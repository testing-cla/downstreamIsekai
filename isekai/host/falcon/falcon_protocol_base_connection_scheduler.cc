#include "isekai/host/falcon/falcon_protocol_base_connection_scheduler.h"

#include <cstdint>
#include <memory>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/strings/substitute.h"
#include "absl/time/time.h"
#include "glog/logging.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/falcon_utils.h"

namespace isekai {
namespace {
// Flag: enable_connection_scheduler_max_delayed_packet_stats
// Maximum queuing delay time for each packet type ($0).
constexpr std::string_view kStatVectorMaxQueueingDelayByType =
    "falcon.max_queueing_delay_by_type.$0";
// Scheduled packet count by type ($1) during the queuing of the above
// max-delayed packet in each type ($0).
constexpr std::string_view
    kStatVectorSerializedPacketCountByTypeForMaxQueuedPacket =
        "falcon.serialized_packet_count_by_type_for_max_queued_packet.$0.$1";
// Queue length per scheduler queue type ($1) when the queuing of the above
// max-delayed packet in each type ($0) is enqueued.
constexpr std::string_view kStatVectorEnqueueTimeQueueLengthForMaxQueuedPacket =
    "falcon.enqueue_time_queue_length_for_max_queued_packet.$0.$1";
}  // namespace

ProtocolBaseConnectionScheduler::ProtocolBaseConnectionScheduler(
    FalconModelInterface* falcon)
    : falcon_(falcon) {
  // Initialize the scheduler queue related counters (used for scheduling).
  std::vector<PacketTypeQueue> queue_types(
      {PacketTypeQueue::kPullAndOrderedPushRequest,
       PacketTypeQueue::kUnorderedPushRequest, PacketTypeQueue::kPushData,
       PacketTypeQueue::kPushGrant, PacketTypeQueue::kPullData});

  for (const auto& queue : queue_types) {
    packet_queue_wise_pkt_count_[queue] = 0;
  }

  // Initialize the per-packet type tx counters
  std::vector<falcon::PacketType> packet_types({
      falcon::PacketType::kInvalid,
      falcon::PacketType::kPullRequest,
      falcon::PacketType::kPushRequest,
      falcon::PacketType::kPushGrant,
      falcon::PacketType::kPullData,
      falcon::PacketType::kPushSolicitedData,
      falcon::PacketType::kPushUnsolicitedData,
  });
  for (const auto& packet_type : packet_types) {
    packet_type_wise_tx_count_[packet_type] = 0;
    packet_type_wise_max_queueing_delay_[packet_type] = absl::ZeroDuration();
  }
  collect_max_delay_stats_ =
      falcon_->get_stats_manager()
          ->GetStatsConfig()
          .enable_connection_scheduler_max_delayed_packet_stats();
}

// Initializes the intra connection scheduling queues.
absl::Status ProtocolBaseConnectionScheduler::InitConnectionSchedulerQueues(
    uint32_t scid) {
  auto [unused_iterator, was_inserted] =
      connection_queues_.insert({scid, ConnectionSchedulerQueues()});
  if (!was_inserted) {
    return absl::AlreadyExistsError(
        "Duplicate source connection ID. Queues already exist.");
  }
  return absl::OkStatus();
}

// Add a packet to the relevant queue for transmitting over the network.
PacketTypeQueue ProtocolBaseConnectionScheduler::AddPacketToQueue(
    uint32_t scid, uint32_t rsn, falcon::PacketType type) {
  VLOG(2) << "[" << falcon_->get_host_id() << ": "
          << falcon_->get_environment()->ElapsedTime() << "][" << scid << ", "
          << rsn << ", " << static_cast<int>(type) << "] "
          << "Enqueues in the connection scheduler.";
  falcon_->get_stats_manager()->UpdateSchedulerCounters(
      SchedulerTypes::kConnection, false);

  // Decide the connection queue to which this packet needs to be added to
  // based on its type and whether the connection is ordered or not.
  ConnectionStateManager* const state_manager = falcon_->get_state_manager();
  CHECK_OK_THEN_ASSIGN(ConnectionState* const connection_state,
                       state_manager->PerformDirectLookup(scid));
  // Fetching transaction and packet metadata to record enqueue timestamps.
  CHECK_OK_THEN_ASSIGN(
      auto transaction,
      connection_state->GetTransaction(TransactionKey(
          rsn, GetTransactionLocation(type, /*incoming=*/false))));
  CHECK_OK_THEN_ASSIGN(auto packet_metadata,
                       transaction->GetPacketMetadata(type));
  bool is_ordered_connection =
      connection_state->connection_metadata.ordered_mode ==
              OrderingMode::kOrdered
          ? true
          : false;
  PacketTypeQueue queue_type;

  switch (type) {
    case falcon::PacketType::kPullRequest:
      queue_type = PacketTypeQueue::kPullAndOrderedPushRequest;
      break;
    case falcon::PacketType::kPushRequest:
      if (is_ordered_connection) {
        queue_type = PacketTypeQueue::kPullAndOrderedPushRequest;
      } else {
        queue_type = PacketTypeQueue::kUnorderedPushRequest;
      }
      break;
    case falcon::PacketType::kPullData:
      queue_type = PacketTypeQueue::kPullData;
      break;
    case falcon::PacketType::kPushGrant:
      queue_type = PacketTypeQueue::kPushGrant;
      break;
    case falcon::PacketType::kPushSolicitedData: {
      queue_type = PacketTypeQueue::kPushData;
      if (is_ordered_connection) {
        packet_metadata->queuing_delay_metadata.tx_eligible_time =
            falcon_->get_environment()->ElapsedTime();
        packet_metadata->is_transmission_eligible = true;
        // Take snapshot of per-packet type TX counters
        packet_metadata->queuing_delay_metadata
            .enqueue_packet_type_wise_tx_count_snapshot =
            packet_type_wise_tx_count_;
        // Take snapshot of number of packets in each packet-type queue
        packet_metadata->queuing_delay_metadata
            .enqueue_packet_queue_wise_pkt_count_snapshot =
            packet_queue_wise_pkt_count_;
        // Update the count corresponding to this queue as the push solicited
        // data is eligible for transmission.
        UpdateOutstandingPacketCount(queue_type, /*is_dequed=*/false);
      }
    } break;
    case falcon::PacketType::kPushUnsolicitedData:
      queue_type = PacketTypeQueue::kPushData;
      break;
    case falcon::PacketType::kNack:
      LOG(FATAL) << "NACKs should not go through connection scheduler.";
      break;
    case falcon::PacketType::kResync:
      LOG(FATAL) << "Resyncs should not go through connection scheduler.";
      break;
    default:
      LOG(FATAL) << "Not a FALCON packet type enqueued.";
      break;
  }

  // Update stats, enqueue in relevant queue for all types leaving ordered push
  // solicited data. For this type, the packet is enqueued along with the push
  // request.
  if (!is_ordered_connection ||
      type != falcon::PacketType::kPushSolicitedData) {
    // Update intra connection scheduler queue statistics.
    falcon_->get_stats_manager()->UpdateIntraConnectionSchedulerCounters(
        scid, queue_type, false);
    packet_metadata->queuing_delay_metadata.enqueue_time =
        falcon_->get_environment()->ElapsedTime();
    packet_metadata->queuing_delay_metadata
        .enqueue_packet_type_wise_tx_count_snapshot =
        packet_type_wise_tx_count_;
    packet_metadata->queuing_delay_metadata
        .enqueue_packet_queue_wise_pkt_count_snapshot =
        packet_queue_wise_pkt_count_;

    // Create work id that is going to enqueued in the appropriate queue.
    WorkId work_id(rsn, type);
    connection_queues_[scid].Enqueue(queue_type, work_id);
    if (!(is_ordered_connection &&
          type == falcon::PacketType::kPushUnsolicitedData)) {
      // Update counter for all packets leaving ordered push unsolicited data as
      // the data is not eligible until the corresponding phantom request is
      // serviced.
      UpdateOutstandingPacketCount(queue_type, /*is_dequed=*/false);
    }
  }

  // Enqueues the corresponding phantom request for the push unsolicited data,
  // and lets the request queue active.
  if (is_ordered_connection &&
      type == falcon::PacketType::kPushUnsolicitedData) {
    WorkId phantom_request_work_id(rsn, falcon::PacketType::kInvalid);
    queue_type = PacketTypeQueue::kPullAndOrderedPushRequest;
    // Update the counter corresponding to this queue as a phantom request is
    // added which is eligible for service.
    UpdateOutstandingPacketCount(queue_type, /*is_dequed=*/false);
    CHECK_OK_THEN_ASSIGN(
        auto phantom_request_packet_metadata,
        transaction->GetPacketMetadata(falcon::PacketType::kInvalid));
    phantom_request_packet_metadata->queuing_delay_metadata.enqueue_time =
        falcon_->get_environment()->ElapsedTime();
    phantom_request_packet_metadata->queuing_delay_metadata
        .enqueue_packet_type_wise_tx_count_snapshot =
        packet_type_wise_tx_count_;
    phantom_request_packet_metadata->queuing_delay_metadata
        .enqueue_packet_queue_wise_pkt_count_snapshot =
        packet_queue_wise_pkt_count_;
    connection_queues_[scid].Enqueue(queue_type, phantom_request_work_id);
  }

  // In case of ordered push solicited request, we must also enqueue the push
  // solicited data in the relevant queue at request insertion time itself to
  // ensure ordering between solicited data and unsolicited data.
  if (is_ordered_connection && type == falcon::PacketType::kPushRequest) {
    WorkId data_work_id(rsn, falcon::PacketType::kPushSolicitedData);
    // This queue is not marked active currently as this solicited data
    // becomes eligible only when we receive the grant.
    auto push_data_queue = PacketTypeQueue::kPushData;
    falcon_->get_stats_manager()->UpdateIntraConnectionSchedulerCounters(
        scid, push_data_queue, false);
    CHECK_OK_THEN_ASSIGN(
        auto push_data_packet_metadata,
        transaction->GetPacketMetadata(falcon::PacketType::kPushSolicitedData));
    push_data_packet_metadata->queuing_delay_metadata.enqueue_time =
        falcon_->get_environment()->ElapsedTime();
    push_data_packet_metadata->queuing_delay_metadata
        .enqueue_packet_type_wise_tx_count_snapshot =
        packet_type_wise_tx_count_;
    push_data_packet_metadata->queuing_delay_metadata
        .enqueue_packet_queue_wise_pkt_count_snapshot =
        packet_queue_wise_pkt_count_;

    connection_queues_[scid].Enqueue(push_data_queue, data_work_id);
  }

  return queue_type;
}

void ProtocolBaseConnectionScheduler::UpdateQueuingDelayStatistics(
    PacketMetadata::PacketQueuingDelayMetadata* queuing_delay_metadata,
    uint32_t scid, bool is_ordered_connection, falcon::PacketType packet_type) {
  absl::Duration queueing_delay = falcon_->get_environment()->ElapsedTime() -
                                  queuing_delay_metadata->enqueue_time;
  absl::Duration tx_eligible_queueing_delay =
      falcon_->get_environment()->ElapsedTime() -
      queuing_delay_metadata->tx_eligible_time;
  StatisticCollectionInterface* stats_collector =
      falcon_->get_stats_collector();
  if (collect_max_delay_stats_ && stats_collector) {
    // We choose for data packets the delay since they became TX-eligible.
    absl::Duration chosen_queueing_delay =
        packet_type == falcon::PacketType::kPushSolicitedData ||
                packet_type == falcon::PacketType::kPushUnsolicitedData
            ? tx_eligible_queueing_delay
            : queueing_delay;
    // If the queuing delay of this packet is the max seen for this type:
    if (chosen_queueing_delay >
        packet_type_wise_max_queueing_delay_[packet_type]) {
      // Update the max queueing delay counter for this type;
      packet_type_wise_max_queueing_delay_[packet_type] = chosen_queueing_delay;
      CHECK_OK(falcon_->get_stats_collector()->UpdateStatistic(
          absl::Substitute(kStatVectorMaxQueueingDelayByType,
                           TypeToString(packet_type)),
          absl::ToDoubleMicroseconds(
              packet_type_wise_max_queueing_delay_[packet_type]),
          StatisticsCollectionConfig::TIME_SERIES_STAT));
      // For each packet type:
      for (const auto& type_count_snapshot :
           queuing_delay_metadata->enqueue_packet_type_wise_tx_count_snapshot) {
        // Record difference between current per-packet type TX counter and the
        // snapshot when this packet was enqueued;
        CHECK_OK(falcon_->get_stats_collector()->UpdateStatistic(
            absl::Substitute(
                kStatVectorSerializedPacketCountByTypeForMaxQueuedPacket,
                TypeToString(packet_type),
                TypeToString(type_count_snapshot.first)),
            packet_type_wise_tx_count_[type_count_snapshot.first] -
                type_count_snapshot.second,
            StatisticsCollectionConfig::TIME_SERIES_STAT));
      }
      // Also, record the queue-wise counter snapshot taken when this packet was
      // enqueued.
      for (const auto& type_count_snapshot :
           queuing_delay_metadata
               ->enqueue_packet_queue_wise_pkt_count_snapshot) {
        CHECK_OK(falcon_->get_stats_collector()->UpdateStatistic(
            absl::Substitute(
                kStatVectorEnqueueTimeQueueLengthForMaxQueuedPacket,
                TypeToString(packet_type),
                TypeToString(type_count_snapshot.first)),
            type_count_snapshot.second,
            StatisticsCollectionConfig::TIME_SERIES_STAT));
      }
    }
  }
  // Record the difference between now and the enqueue of this packet as a data
  // point of queuing delay for this type of packets.
  falcon_->get_histogram_collector()->Add(
      scid, packet_type, is_ordered_connection, false,
      absl::ToDoubleNanoseconds(queueing_delay));
  if (packet_type == falcon::PacketType::kPushSolicitedData) {
    // Solicited data is enqueued when the request is enqueued but is not yet
    // TX-eligible. We also record The time since it became TX-eligible on
    // grant.
    falcon_->get_histogram_collector()->Add(
        scid, packet_type, is_ordered_connection, true,
        absl::ToDoubleNanoseconds(tx_eligible_queueing_delay));
  }
}

// Return the total queue length of an inter-connection queue.
uint32_t ProtocolBaseConnectionScheduler::GetConnectionQueueLength(
    uint32_t scid) {
  uint32_t unscheduled_queue_length = 0;
  unscheduled_queue_length += connection_queues_[scid].GetSize(
      PacketTypeQueue::kPullAndOrderedPushRequest);
  unscheduled_queue_length +=
      connection_queues_[scid].GetSize(PacketTypeQueue::kUnorderedPushRequest);
  unscheduled_queue_length +=
      connection_queues_[scid].GetSize(PacketTypeQueue::kPushData);
  unscheduled_queue_length +=
      connection_queues_[scid].GetSize(PacketTypeQueue::kPushGrant);
  unscheduled_queue_length +=
      connection_queues_[scid].GetSize(PacketTypeQueue::kPullData);
  return unscheduled_queue_length;
}

// Checks if a packet meets its congestion window and admission control.
bool ProtocolBaseConnectionScheduler::CanSend(uint32_t scid, uint32_t rsn,
                                              falcon::PacketType type) {
  ConnectionStateManager* const conn_state_manager =
      falcon_->get_state_manager();
  CHECK_OK_THEN_ASSIGN(auto connection_state,
                       conn_state_manager->PerformDirectLookup(scid));
  CHECK_OK_THEN_ASSIGN(auto transaction,
                       connection_state->GetTransaction(TransactionKey(
                           rsn, GetTransactionLocation(type,
                                                       /*incoming=*/false))));
  CHECK_OK_THEN_ASSIGN(auto packet_metadata,
                       transaction->GetPacketMetadata(type));

  bool meets_cc_criteria =
      falcon_->get_packet_reliability_manager()
          ->MeetsInitialTransmissionCCTxGatingCriteria(scid, type);
  bool meets_admission_control_criteria =
      falcon_->get_admission_control_manager()->MeetsAdmissionControlCriteria(
          scid, rsn, type);
  bool meets_oow_criteria =
      falcon_->get_packet_reliability_manager()
          ->MeetsInitialTransmissionOowTxGatingCriteria(scid, type);

  falcon_->get_stats_manager()->UpdateCwndPauseCounters(scid,
                                                        !meets_cc_criteria);

  return packet_metadata->is_transmission_eligible &&
         (meets_cc_criteria && meets_admission_control_criteria &&
          meets_oow_criteria);
}

// Checks if a packet meets its op rate criteria.
bool ProtocolBaseConnectionScheduler::MeetsOpRateCriteria(uint32_t scid) {
  ConnectionStateManager* const conn_state_manager =
      falcon_->get_state_manager();
  CHECK_OK_THEN_ASSIGN(auto connection_state,
                       conn_state_manager->PerformDirectLookup(scid));

  return connection_state->last_packet_send_time + kPerConnectionInterOpGap <=
         falcon_->get_environment()->ElapsedTime();
}

// Schedules the given packet by handing it off to the downstream blocks.
void ProtocolBaseConnectionScheduler::SchedulePacket(uint32_t scid,
                                                     const WorkId& work_id) {
  VLOG(2) << "[" << falcon_->get_host_id() << ": "
          << falcon_->get_environment()->ElapsedTime() << "][" << scid << ", "
          << work_id.rsn << ", " << static_cast<int>(work_id.type) << "] "
          << "Packet in connection scheduler picked by arbiter.";
  falcon_->get_stats_manager()->UpdateSchedulerCounters(
      SchedulerTypes::kConnection, true);

  ConnectionStateManager* const state_manager = falcon_->get_state_manager();
  CHECK_OK_THEN_ASSIGN(auto connection_state,
                       state_manager->PerformDirectLookup(scid));
  bool is_ordered_connection =
      connection_state->connection_metadata.ordered_mode ==
              OrderingMode::kOrdered
          ? true
          : false;
  const bool is_incoming_packet = false;
  CHECK_OK_THEN_ASSIGN(
      auto transaction,
      connection_state->GetTransaction(TransactionKey(
          work_id.rsn,
          GetTransactionLocation(work_id.type, is_incoming_packet))));
  CHECK_OK_THEN_ASSIGN(auto packet_metadata,
                       transaction->GetPacketMetadata(work_id.type));

  packet_metadata->schedule_status = absl::OkStatus();

  UpdateQueuingDelayStatistics(&packet_metadata->queuing_delay_metadata, scid,
                               is_ordered_connection, work_id.type);
  RecordRsnDiffMeasure(scid, work_id);

  falcon_->get_stats_manager()->UpdateInitialTxRsnSeries(scid, work_id.rsn,
                                                         work_id.type);

  connection_state->last_packet_send_time =
      falcon_->get_environment()->ElapsedTime();

  CHECK_OK(falcon_->get_packet_reliability_manager()->TransmitPacket(
      scid, work_id.rsn, work_id.type));
}

void ProtocolBaseConnectionScheduler::HandlePhantomRequest(
    uint32_t scid, const WorkId& work_id,
    PacketTypeQueue candidate_queue_type) {
  // Marks the corresponding push unsolicited data as eligible to send.
  CHECK_OK_THEN_ASSIGN(auto connection_state,
                       falcon_->get_state_manager()->PerformDirectLookup(scid));
  CHECK_OK_THEN_ASSIGN(
      auto transaction,
      connection_state->GetTransaction(TransactionKey(
          work_id.rsn, GetTransactionLocation(
                           falcon::PacketType::kPushUnsolicitedData, false))));
  CHECK_OK_THEN_ASSIGN(
      auto push_unsolicited_packet,
      transaction->GetPacketMetadata(falcon::PacketType::kPushUnsolicitedData));
  push_unsolicited_packet->is_transmission_eligible = true;
  auto push_data_queue = PacketTypeQueue::kPushData;
  // Updating counter corresponding to push data queue as its eligible for
  // service.
  UpdateOutstandingPacketCount(push_data_queue, /*is_dequed = */ false);
  push_unsolicited_packet->queuing_delay_metadata.tx_eligible_time =
      falcon_->get_environment()->ElapsedTime();

  // Pops the phantom request and cleans up the request queue.
  CHECK_OK_THEN_ASSIGN(auto phantom_packet,
                       transaction->GetPacketMetadata(work_id.type));
  phantom_packet->schedule_status = absl::OkStatus();
  falcon_->get_histogram_collector()->Add(
      scid, work_id.type, true, false,
      absl::ToDoubleNanoseconds(
          falcon_->get_environment()->ElapsedTime() -
          phantom_packet->queuing_delay_metadata.enqueue_time));
  connection_queues_[scid].Pop(candidate_queue_type);
  // Reflect the handling of the phantom request by updating the outstanding
  // packet count corresponding to the relevant queue.
  UpdateOutstandingPacketCount(candidate_queue_type, /*is_dequed = */ true);
}

uint64_t
ProtocolBaseConnectionScheduler::GetQueueTypeBasedOutstandingPacketCount(
    PacketTypeQueue queue_type) {
  return packet_queue_wise_pkt_count_[queue_type];
}

void ProtocolBaseConnectionScheduler::UpdateOutstandingPacketCount(
    PacketTypeQueue queue_type, bool is_dequed) {
  if (is_dequed) {
    packet_queue_wise_pkt_count_[queue_type] -= 1;
  } else {
    packet_queue_wise_pkt_count_[queue_type] += 1;
  }
}

void ProtocolBaseConnectionScheduler::RecordRsnDiffMeasure(
    uint32_t scid, const WorkId& work_id) {
  if (work_id.type == falcon::PacketType::kPullRequest) {
    if (sent_out_push_unsolicited_data &&
        (work_id.rsn > last_push_unsolicited_data_rsn)) {
      auto rsn_diff_measure = work_id.rsn - last_push_unsolicited_data_rsn;
      falcon_->get_stats_manager()->UpdateMaxRsnDistance(scid,
                                                         rsn_diff_measure);
    }
  } else if (work_id.type == falcon::PacketType::kPushUnsolicitedData) {
    sent_out_push_unsolicited_data = true;
    last_push_unsolicited_data_rsn = work_id.rsn;
  }
}

}  // namespace isekai
