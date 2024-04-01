#include "isekai/host/falcon/falcon_protocol_retransmission_scheduler.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "glog/logging.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/environment.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"
#include "isekai/host/falcon/falcon_protocol_inter_connection_round_robin_policy.h"
#include "isekai/host/falcon/falcon_protocol_intra_connection_round_robin_policy.h"
#include "isekai/host/falcon/falcon_utils.h"

namespace isekai {

ProtocolRetransmissionScheduler::ProtocolRetransmissionScheduler(
    FalconModelInterface* falcon)
    : falcon_(falcon) {
  switch (falcon_->get_config()
              ->inter_connection_retransmission_scheduling_policy()) {
    case FalconConfig::ROUND_ROBIN:
      inter_connection_policy_ =
          std::make_unique<InterConnectionRoundRobinPolicy>();
      break;
    default:
      LOG(FATAL) << "Unsupported scheduling policy provided.";
  }
  switch (falcon_->get_config()
              ->inter_connection_retransmission_scheduling_policy()) {
    case FalconConfig::ROUND_ROBIN:
      intra_connection_policy_ =
          std::make_unique<IntraConnectionRoundRobinPolicy<WindowTypeQueue>>();
      break;
    default:
      LOG(FATAL) << "Unsupported scheduling policy provided.";
  }
}

// Initializes the intra connection scheduling queues.
absl::Status ProtocolRetransmissionScheduler::InitConnectionSchedulerQueues(
    uint32_t scid) {
  auto [unused_iterator, was_inserted] =
      connection_queues_.insert({scid, RetransmissionSchedulerQueues()});
  if (!was_inserted) {
    return absl::AlreadyExistsError(
        "Duplicate source connection ID. Queues already exist.");
  }
  CHECK_OK(inter_connection_policy_->InitConnection(scid));
  CHECK_OK(intra_connection_policy_->InitConnection(scid));
  return absl::OkStatus();
}

// Add a packet to the relevant queue for transmitting over the network.
absl::Status ProtocolRetransmissionScheduler::EnqueuePacket(
    uint32_t scid, uint32_t rsn, falcon::PacketType type) {
  VLOG(2) << "[" << falcon_->get_host_id() << ": "
          << falcon_->get_environment()->ElapsedTime() << "][" << scid << ", "
          << rsn << ", " << static_cast<int>(type) << "] "
          << "Enqueues in the retransmission scheduler.";
  falcon_->get_stats_manager()->UpdateSchedulerCounters(
      SchedulerTypes::kRetransmission, false);

  // Decide the connection queue to which this packet needs to be added to based
  // on its type and whether the connection is ordered or not.
  ConnectionStateManager* const state_manager = falcon_->get_state_manager();
  ASSIGN_OR_RETURN(ConnectionState* const connection_state,
                   state_manager->PerformDirectLookup(scid));
  ASSIGN_OR_RETURN(TransactionMetadata* const transaction,
                   connection_state->GetTransaction(
                       TransactionKey(rsn, GetTransactionLocation(
                                               /*type=*/type,
                                               /*incoming=*/false))));
  ASSIGN_OR_RETURN(PacketMetadata* const packet_metadata,
                   transaction->GetPacketMetadata(type));

  WindowTypeQueue queue_type;
  switch (type) {
    case falcon::PacketType::kPullRequest:
    case falcon::PacketType::kPushRequest:
      queue_type = WindowTypeQueue::kRequest;
      break;
    case falcon::PacketType::kPullData:
    case falcon::PacketType::kPushGrant:
    case falcon::PacketType::kPushSolicitedData:
    case falcon::PacketType::kPushUnsolicitedData:
      queue_type = WindowTypeQueue::kData;
      break;
    case falcon::PacketType::kAck:
      LOG(FATAL) << "ACKs should not go through retransmission scheduler.";
      break;
    case falcon::PacketType::kNack:
      LOG(FATAL) << "NACKs should not go through retransmission scheduler.";
      break;
    case falcon::PacketType::kResync:
      LOG(FATAL) << "Resyncs should not go through retransmission scheduler.";
      break;
    case falcon::PacketType::kBack:
      LOG(FATAL) << "BACKs should not go through retransmission scheduler.";
      break;
    case falcon::PacketType::kEack:
      LOG(FATAL) << "EACKs should not go through retransmission scheduler.";
      break;
    case falcon::PacketType::kInvalid:
      LOG(FATAL)
          << "Invalid (phantoms) should not through retransmission scheduler.";
      break;
  }
  // Create work id that is going to enqueued in the appropriate queue.
  RetransmissionWorkId work_id(rsn, packet_metadata->psn, type);
  auto inserted = connection_queues_[scid].Enqueue(queue_type, work_id);
  if (!inserted) {
    return absl::AlreadyExistsError(
        "Previous retransmission of packet still in retransmission scheduler "
        "queue.");
  } else {
    inter_connection_policy_->MarkConnectionActive(scid);
    intra_connection_policy_->MarkQueueActive(scid, queue_type);
    // Schedule the work scheduler in case this is the only outstanding work
    // item.
    falcon_->get_arbiter()->ScheduleSchedulerArbiter();
    return absl::OkStatus();
  }
}

absl::Status ProtocolRetransmissionScheduler::DequeuePacket(
    uint32_t scid, uint32_t rsn, falcon::PacketType type) {
  VLOG(2) << "[" << falcon_->get_host_id() << ": "
          << falcon_->get_environment()->ElapsedTime() << "][" << scid << ", "
          << rsn << ", " << static_cast<int>(type) << "] "
          << "Dequeues in the retransmission scheduler.";
  // Decide the connection queue from which this packet needs to be added to
  // based on its type and whether the connection is ordered or not.
  ConnectionStateManager* const state_manager = falcon_->get_state_manager();
  ASSIGN_OR_RETURN(ConnectionState* const connection_state,
                   state_manager->PerformDirectLookup(scid));
  ASSIGN_OR_RETURN(TransactionMetadata* const transaction,
                   connection_state->GetTransaction(
                       TransactionKey(rsn, GetTransactionLocation(
                                               /*type=*/type,
                                               /*incoming=*/false))));
  ASSIGN_OR_RETURN(PacketMetadata* const packet_metadata,
                   transaction->GetPacketMetadata(type));

  WindowTypeQueue queue_type;
  switch (type) {
    case falcon::PacketType::kPullRequest:
    case falcon::PacketType::kPushRequest:
      queue_type = WindowTypeQueue::kRequest;
      break;
    case falcon::PacketType::kPullData:
    case falcon::PacketType::kPushGrant:
    case falcon::PacketType::kPushSolicitedData:
    case falcon::PacketType::kPushUnsolicitedData:
      queue_type = WindowTypeQueue::kData;
      break;
    case falcon::PacketType::kAck:
      LOG(FATAL) << "ACKs should not go through retransmission scheduler.";
      break;
    case falcon::PacketType::kNack:
      LOG(FATAL) << "NACKs should not go through retransmission scheduler.";
      break;
    case falcon::PacketType::kBack:
      LOG(FATAL) << "BACKs should not go through retransmission scheduler.";
      break;
    case falcon::PacketType::kEack:
      LOG(FATAL) << "EACKs should not go through retransmission scheduler.";
      break;
    case falcon::PacketType::kResync:
      LOG(FATAL) << "Resyncs should not go through retransmission scheduler.";
      break;
    case falcon::PacketType::kInvalid:
      LOG(FATAL) << "Invalid (phantom) request should not go through "
                    "retransmission scheduler.";
      break;
  }
  // Create work id that is going to be dequeued from the appropriate queue.
  RetransmissionWorkId work_id(rsn, packet_metadata->psn, type);
  connection_queues_[scid].Dequeue(queue_type, work_id);
  // If this queue has no more work, mark it as inactive.
  if (connection_queues_[scid].IsEmpty(queue_type)) {
    intra_connection_policy_->MarkQueueInactive(scid, queue_type);
    // Further, if this connection has no more work, mark it as inactive.
    if (!intra_connection_policy_->HasWork(scid)) {
      inter_connection_policy_->MarkConnectionInactive(scid);
    }
  }
  return absl::OkStatus();
}

// Performs one unit of work from the retransmission scheduler.
bool ProtocolRetransmissionScheduler::ScheduleWork() {
  std::optional<uint32_t> starting_connection_id = std::nullopt;
  while (true) {
    // Pick a candidate connection from the pool of connections to schedule in
    // this cycle.
    CHECK_OK_THEN_ASSIGN(uint32_t candidate_connection_id,
                         inter_connection_policy_->SelectConnection());

    if (!starting_connection_id.has_value()) {
      starting_connection_id = candidate_connection_id;
    } else if (candidate_connection_id == starting_connection_id) {
      // Exit the scheduling logic in case no connections are available or we
      // have looped through all connections in this cycle and have found no
      // work.
      return false;
    }

    // Go through the intra connection queues of the candidate connection and
    // attempt to schedule a packet.
    WindowTypeQueue starting_queue_type;
    bool is_starting_queue_initialized = false;
    CHECK_OK_THEN_ASSIGN(auto connection_state,
                         falcon_->get_state_manager()->PerformDirectLookup(
                             candidate_connection_id));
    while (true) {
      // Pick the intra connection queue type of the candidate connection.
      absl::StatusOr<WindowTypeQueue> candidate_queue_type =
          intra_connection_policy_->SelectQueue(candidate_connection_id);
      // Move onto another connection in case none of the queues have work or we
      // have looped through all queues and have found no work.
      if (is_starting_queue_initialized &&
          candidate_queue_type.value() == starting_queue_type) {
        break;
      }
      if (!is_starting_queue_initialized) {
        starting_queue_type = candidate_queue_type.value();
        is_starting_queue_initialized = true;
      }
      // Schedule the selected packet and remove it from the queue, if it meets
      // the congestion control criteria.
      RetransmissionWorkId work_id =
          connection_queues_[candidate_connection_id].Peek(
              candidate_queue_type.value());
      bool meets_op_rate_criteria =
          connection_state->last_packet_send_time + kPerConnectionInterOpGap <=
          falcon_->get_environment()->ElapsedTime();
      if (meets_op_rate_criteria &&
          falcon_->get_packet_reliability_manager()
              ->MeetsRetransmissionCCTxGatingCriteria(
                  candidate_connection_id, work_id.psn, work_id.type)) {
        SchedulePacket(candidate_connection_id, work_id);
        connection_queues_[candidate_connection_id].Dequeue(
            candidate_queue_type.value(), work_id);
        // If this queue has no more work, mark it as inactive.
        if (connection_queues_[candidate_connection_id].IsEmpty(
                candidate_queue_type.value())) {
          intra_connection_policy_->MarkQueueInactive(
              candidate_connection_id, candidate_queue_type.value());
          // Further, if this connection has no more work, mark it as inactive.
          if (!intra_connection_policy_->HasWork(candidate_connection_id)) {
            inter_connection_policy_->MarkConnectionInactive(
                candidate_connection_id);
          }
        }
        return true;
      }
    }
  }
}

// Returns true if the retransmission scheduler has outstanding work.
bool ProtocolRetransmissionScheduler::HasWork() {
  return falcon_->CanSendPacket() && inter_connection_policy_->HasWork();
}

// Schedules the given packet by handing it off to the downstream blocks.
void ProtocolRetransmissionScheduler::SchedulePacket(
    uint32_t scid, const RetransmissionWorkId& work_id) {
  VLOG(2) << "[" << falcon_->get_host_id() << ": "
          << falcon_->get_environment()->ElapsedTime() << "][" << scid << ", "
          << work_id.rsn << ", " << static_cast<int>(work_id.type) << "] "
          << "Packet in retransmission scheduler picked by arbiter.";
  falcon_->get_stats_manager()->UpdateSchedulerCounters(
      SchedulerTypes::kRetransmission, true);

  ConnectionStateManager* const state_manager = falcon_->get_state_manager();
  CHECK_OK_THEN_ASSIGN(auto connection_state,
                       state_manager->PerformDirectLookup(scid));
  CHECK_OK_THEN_ASSIGN(auto transaction,
                       connection_state->GetTransaction(TransactionKey(
                           work_id.rsn, GetTransactionLocation(
                                            /*type=*/work_id.type,
                                            /*incoming=*/false))));
  CHECK_OK_THEN_ASSIGN(auto packet_metadata,
                       transaction->GetPacketMetadata(work_id.type));

  falcon_->get_stats_manager()->UpdateRetxRsnSeries(
      scid, work_id.rsn, work_id.type, packet_metadata->retransmission_reason);

  packet_metadata->schedule_status = absl::OkStatus();
  connection_state->last_packet_send_time =
      falcon_->get_environment()->ElapsedTime();

  CHECK_OK(falcon_->get_packet_reliability_manager()->TransmitPacket(
      scid, work_id.rsn, work_id.type));
}

}  // namespace isekai
