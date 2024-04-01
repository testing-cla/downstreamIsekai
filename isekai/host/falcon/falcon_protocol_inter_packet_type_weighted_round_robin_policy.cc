#include "isekai/host/falcon/falcon_protocol_inter_packet_type_weighted_round_robin_policy.h"

namespace isekai {

namespace {
constexpr double kWeightNormalizer = 64.0;
constexpr std::array<PacketTypeQueue, 5> queue_types = {
    PacketTypeQueue::kPullAndOrderedPushRequest,
    PacketTypeQueue::kUnorderedPushRequest, PacketTypeQueue::kPushData,
    PacketTypeQueue::kPushGrant, PacketTypeQueue::kPullData};
}  // namespace

InterPacketTypeWeightedRoundRobinPolicy::
    InterPacketTypeWeightedRoundRobinPolicy(Scheduler* scheduler)
    : scheduler_(*scheduler) {
  for (const auto& queue : queue_types) {
    policy_state_[queue] = PacketTypePolicyState(
        /* is_active = */ false, /* available_credits = */ 0);
  }

  total_available_credits_ = 0;
}

// Selects the packet type based queue in a weighted round robin manner, where
// the weights correpond to the credits calculated at the beginning of an
// arbitration round.
absl::StatusOr<PacketTypeQueue>
InterPacketTypeWeightedRoundRobinPolicy::SelectPacketTypeBasedQueueType() {
  if (total_available_credits_ == 0) {
    return absl::UnavailableError("No available credits to schedule packets.");
  } else if (active_queues_.empty()) {
    return absl::UnavailableError("No active packet types to schedule.");
  }

  auto packet_queue_type = active_queues_.front();
  auto& packet_queue_state = policy_state_[packet_queue_type];

  if (packet_queue_state.available_credits > 0) {
    total_available_credits_ -= 1;
    packet_queue_state.available_credits -= 1;
    // Remove it from the queue at the front of active queue list and append it
    // to the back. Transfer to inactive list is done explicitly during credit
    // calculation.
    active_queues_.pop_front();
    active_queues_.push_back(packet_queue_type);
    return packet_queue_type;
  } else {
    return absl::UnavailableError(
        "No available credits for this packet type. Looped over all "
        "active packets.");
  }
}

// Marks a given packet type active when the scheduler receives work. This in
// turn triggers the start of an arbitration round if the current credits are 0.
void InterPacketTypeWeightedRoundRobinPolicy::
    MarkPacketTypeBasedQueueTypeActive(PacketTypeQueue packet_queue_type) {
  if (total_available_credits_ == 0) {
    ComputePerPacketCredits();
  }
}

// Marks a given packet type inactive. This occurs when a given packet type does
// not have any credits.
void InterPacketTypeWeightedRoundRobinPolicy::
    MarkPacketTypeBasedQueueTypeInactive(PacketTypeQueue packet_queue_type) {
  auto& packet_state = policy_state_[packet_queue_type];
  if (!packet_state.is_active) {
    return;
  } else {
    total_available_credits_ -= packet_state.available_credits;
    packet_state.is_active = false;
    packet_state.available_credits = 0;
    active_queues_.remove(packet_queue_type);
    inactive_queues_.push_back(packet_queue_type);
  }
}

// Triggers the beginning of a new arbitration round in case all the credits
// have been exhausted or all the packets are current blocked.
void InterPacketTypeWeightedRoundRobinPolicy::UpdatePolicyState(
    bool are_all_packet_types_blocked) {
  // Remove inactive packet type queues, if present.
  for (const auto& queue : queue_types) {
    if (policy_state_[queue].available_credits == 0) {
      MarkPacketTypeBasedQueueTypeInactive(queue);
    }
  }
  // If all the credits have been exhausted or all packets are blocked,
  // calculate per packet type credits for the next round of arbitration.
  if (are_all_packet_types_blocked || total_available_credits_ == 0) {
    ComputePerPacketCredits();
  }
}

// Calculates the credits corresponding to the various scheduler queues based on
// the number of outstanding packets and the weight normalizer. Queues with no
// credits are marked as inactive.
void InterPacketTypeWeightedRoundRobinPolicy::ComputePerPacketCredits() {
  total_available_credits_ = 0;
  for (const auto& queue : queue_types) {
    policy_state_[queue].available_credits = 0;
    // Calculate the credits corresponding to the queue.
    double credits = scheduler_.GetQueueTypeBasedOutstandingPacketCount(queue) /
                     kWeightNormalizer;
    // If no credits, mark the queue as inactive.
    if (credits == 0) {
      if (policy_state_[queue].is_active) {
        MarkPacketTypeBasedQueueTypeInactive(queue);
      }
    } else {
      // Assign credits and activate the queue in case inactive.
      credits = std::ceil(credits);
      policy_state_[queue].available_credits = credits;
      total_available_credits_ += credits;
      if (!policy_state_[queue].is_active) {
        inactive_queues_.remove(queue);
        active_queues_.push_back(queue);
        policy_state_[queue].is_active = true;
      }
    }
  }
}

// Determines if the policy has outstanding work based on the available credits.
bool InterPacketTypeWeightedRoundRobinPolicy::HasWork() const {
  return (total_available_credits_ > 0);
}

bool InterPacketTypeWeightedRoundRobinPolicy::GetPacketTypeStateForTesting(
    PacketTypeQueue queue_type) {
  return policy_state_[queue_type].is_active;
}

uint64_t
InterPacketTypeWeightedRoundRobinPolicy::GetTotalAvailableCreditsForTesting() {
  return total_available_credits_;
}

}  // namespace isekai
