#include "isekai/host/falcon/falcon_protocol_packet_type_based_connection_scheduler.h"

#include <optional>

#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_protocol_inter_packet_type_round_robin_policy.h"
#include "isekai/host/falcon/falcon_protocol_inter_packet_type_weighted_round_robin_policy.h"
#include "isekai/host/falcon/falcon_protocol_intra_packet_type_round_robin_policy.h"
#include "isekai/host/falcon/falcon_utils.h"

namespace isekai {

ProtocolPacketTypeBasedConnectionScheduler::
    ProtocolPacketTypeBasedConnectionScheduler(FalconModelInterface* falcon)
    : ProtocolBaseConnectionScheduler(falcon) {
  switch (falcon_->get_config()
              ->connection_scheduler_policies()
              .inter_packet_type_scheduling_policy()) {
    case FalconConfig::ROUND_ROBIN:
      inter_packet_policy_ =
          std::make_unique<InterPacketTypeRoundRobinPolicy>();
      break;
    case FalconConfig::WEIGHTED_ROUND_ROBIN:
      inter_packet_policy_ =
          std::make_unique<InterPacketTypeWeightedRoundRobinPolicy>(this);
      break;
    default:
      LOG(FATAL) << "Unsupported scheduling policy provided.";
  }
  switch (falcon_->get_config()
              ->connection_scheduler_policies()
              .intra_packet_type_scheduling_policy()) {
    case FalconConfig::ROUND_ROBIN:
      intra_packet_policy_ =
          std::make_unique<IntraPacketTypeRoundRobinPolicy>();
      break;
    default:
      LOG(FATAL) << "Unsupported scheduling policy provided.";
  }
}

// Initializes the intra connection scheduling queues.
absl::Status
ProtocolPacketTypeBasedConnectionScheduler::InitConnectionSchedulerQueues(
    uint32_t scid) {
  CHECK_OK(
      ProtocolBaseConnectionScheduler::InitConnectionSchedulerQueues(scid));
  intra_packet_policy_->InitConnection(scid);
  return absl::OkStatus();
}

// Add a packet to the relevant queue for transmitting over the network.
absl::Status ProtocolPacketTypeBasedConnectionScheduler::EnqueuePacket(
    uint32_t scid, uint32_t rsn, falcon::PacketType type) {
  auto queue_type = AddPacketToQueue(scid, rsn, type);

  inter_packet_policy_->MarkPacketTypeBasedQueueTypeActive(queue_type);
  intra_packet_policy_->MarkConnectionActive(scid, queue_type);

  // Schedule the work scheduler in case this is the only outstanding work item.
  falcon_->get_arbiter()->ScheduleSchedulerArbiter();
  return absl::OkStatus();
}

// Performs one unit of work from the connection scheduler.
bool ProtocolPacketTypeBasedConnectionScheduler::ScheduleWork() {
  PacketTypeQueue starting_packet_type_queue;
  bool is_starting_packet_type_queue_initialized = false;
  while (true) {
    absl::StatusOr<PacketTypeQueue> candidate_packet_type =
        PickCandidatePacketType(starting_packet_type_queue,
                                is_starting_packet_type_queue_initialized);
    if (!candidate_packet_type.ok()) {
      return false;
    }
    // Go through the various connections within a packet type and attempt
    // to schedule a packet.
    std::optional<uint32_t> starting_connection_id = std::nullopt;
    while (true) {
      absl::StatusOr<uint32_t> candidate_connection_id =
          PickCandidateConnection(candidate_packet_type,
                                  starting_connection_id);
      // Move onto another packet type in case all the connections have no
      // work corresponding to this packet or we have looped through all
      // connections and have found no work.
      if (!candidate_connection_id.ok()) {
        break;
      }

      // Schedule the selected packet (if allowed per congestion control
      // and the admission control checks) and remove it from the queue. Else
      // move onto the next queue.
      const WorkId& work_id =
          connection_queues_[candidate_connection_id.value()].Peek(
              candidate_packet_type.value());

      if (work_id.type == falcon::PacketType::kInvalid) {
        HandlePhantomRequest(candidate_connection_id.value(), work_id,
                             candidate_packet_type.value());
        inter_packet_policy_->UpdatePolicyState(false);
        // Call schedule work again to make phantom request cost 0; See
        // b/266962284 for details.
        ScheduleWork();
        return true;
      }

      uint32_t scid = candidate_connection_id.value();

      if (MeetsOpRateCriteria(scid)) {
        bool is_packet_scheduled =
            AttemptPacketScheduling(scid, work_id, candidate_packet_type);
        if (is_packet_scheduled) {
          return true;
        }
      }
    }
  }
}

// Schedules the packet type based on the selected packet type scheduling
// policy.
absl::StatusOr<PacketTypeQueue>
ProtocolPacketTypeBasedConnectionScheduler::PickCandidatePacketType(
    PacketTypeQueue& starting_packet_type_queue,
    bool& is_starting_packet_type_queue_initialized) {
  // Pick a packet type to service in this cycle.
  absl::StatusOr<PacketTypeQueue> candidate_packet_type_queue =
      inter_packet_policy_->SelectPacketTypeBasedQueueType();
  // Exit the scheduling loop after updating the policy state in case we have
  // looped through all packet types in this cycle and have found no
  // work.
  if ((!candidate_packet_type_queue.ok()) ||
      (is_starting_packet_type_queue_initialized &&
       candidate_packet_type_queue.value() == starting_packet_type_queue)) {
    inter_packet_policy_->UpdatePolicyState(true);
    return absl::UnavailableError("No packet type found.");
  }

  if (candidate_packet_type_queue.ok() &&
      !is_starting_packet_type_queue_initialized) {
    is_starting_packet_type_queue_initialized = true;
    starting_packet_type_queue = candidate_packet_type_queue.value();
  }
  return candidate_packet_type_queue;
}

// Schedules the connection based on the packet type and the connection
// scheduling policy.
absl::StatusOr<uint32_t>
ProtocolPacketTypeBasedConnectionScheduler::PickCandidateConnection(
    absl::StatusOr<PacketTypeQueue>& candidate_packet_type_queue,
    std::optional<uint32_t>& starting_connection_id) {
  absl::StatusOr<uint32_t> candidate_connection_id = SelectCandidateConnection(
      candidate_packet_type_queue.value(), starting_connection_id);
  // Move onto another packet type in case all the connections have no
  // work corresponding to this packet or we have looped through all
  // connections and have found no work.
  if (!candidate_connection_id.ok()) {
    return absl::UnavailableError("No connection found.");
  }

  if (candidate_connection_id.ok() && !starting_connection_id.has_value()) {
    starting_connection_id = candidate_connection_id.value();
  }
  return candidate_connection_id;
}

// Schedules the packet with the selected packet type and connection.
bool ProtocolPacketTypeBasedConnectionScheduler::AttemptPacketScheduling(
    uint32_t scid, const WorkId& work_id,
    absl::StatusOr<PacketTypeQueue> candidate_packet_type_queue) {
  if (CanSend(scid, work_id.rsn, work_id.type)) {
    SchedulePacket(scid, candidate_packet_type_queue.value(), work_id);
    return true;
  }
  return false;
}

// Selects a candidate connection to transmit an eligible packet.
absl::StatusOr<uint32_t>
ProtocolPacketTypeBasedConnectionScheduler::SelectCandidateConnection(
    PacketTypeQueue queue_type,
    std::optional<uint32_t> starting_connection_id) {
  CHECK_OK_THEN_ASSIGN(uint32_t candidate_connection_id,
                       intra_packet_policy_->SelectConnection(queue_type));
  // No work available in case all the connections have no work corresponding to
  // this packet or we have looped through all connections and have found
  // no eligible work.
  if (starting_connection_id.has_value() &&
      candidate_connection_id == starting_connection_id) {
    return absl::UnavailableError(
        "No outstanding work from the connections currently.");
  }
  return candidate_connection_id;
}

void ProtocolPacketTypeBasedConnectionScheduler::SchedulePacket(
    uint32_t scid, PacketTypeQueue queue_type, const WorkId& work_id) {
  CHECK_OK(
      falcon_->get_admission_control_manager()->ReserveAdmissionControlResource(
          scid, work_id.rsn, work_id.type));

  // Update intra connection scheduler queue statistics.
  falcon_->get_stats_manager()->UpdateIntraConnectionSchedulerCounters(
      scid, queue_type, true);
  UpdateOutstandingPacketCount(queue_type, true);
  ProtocolBaseConnectionScheduler::SchedulePacket(scid, work_id);

  UpdatePolicyStates(scid, queue_type);
}

void ProtocolPacketTypeBasedConnectionScheduler::UpdatePolicyStates(
    uint32_t scid, PacketTypeQueue queue_type) {
  connection_queues_[scid].Pop(queue_type);

  // If this connection has no more work corresponding to the selected
  // packet type, mark the queue as inactive.
  if (connection_queues_[scid].IsEmpty(queue_type)) {
    intra_packet_policy_->MarkConnectionInactive(scid, queue_type);
    // Further, if the packet type has no more work across all
    // connections, mark it as inactive.
    if (!intra_packet_policy_->HasWork(queue_type)) {
      inter_packet_policy_->MarkPacketTypeBasedQueueTypeInactive(queue_type);
    }
  }
  inter_packet_policy_->UpdatePolicyState(false);
}

void ProtocolPacketTypeBasedConnectionScheduler::HandlePhantomRequest(
    uint32_t scid, const WorkId& work_id,
    PacketTypeQueue candidate_queue_type) {
  ProtocolBaseConnectionScheduler::HandlePhantomRequest(scid, work_id,
                                                        candidate_queue_type);

  // Marks the corresponding data queue as active for schedule packet.
  inter_packet_policy_->MarkPacketTypeBasedQueueTypeActive(
      PacketTypeQueue::kPushData);
  intra_packet_policy_->MarkConnectionActive(scid, PacketTypeQueue::kPushData);

  // Reconstruct the packet type RR order, i.e., request queue should be behind
  // data queue.
  inter_packet_policy_->MarkPacketTypeBasedQueueTypeInactive(
      candidate_queue_type);
  // Mark the corresponding request queue inactive if no work remaining.
  if (connection_queues_[scid].IsEmpty(candidate_queue_type)) {
    intra_packet_policy_->MarkConnectionInactive(scid, candidate_queue_type);
  }
  if (intra_packet_policy_->HasWork(candidate_queue_type)) {
    inter_packet_policy_->MarkPacketTypeBasedQueueTypeActive(
        candidate_queue_type);
  }
}

// Returns true if the connection scheduler has outstanding work.
bool ProtocolPacketTypeBasedConnectionScheduler::HasWork() {
  return falcon_->CanSendPacket() && inter_packet_policy_->HasWork();
}

}  // namespace isekai
