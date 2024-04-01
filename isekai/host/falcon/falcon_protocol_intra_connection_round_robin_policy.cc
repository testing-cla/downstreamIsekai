#include "isekai/host/falcon/falcon_protocol_intra_connection_round_robin_policy.h"

#include <cstdint>
#include <list>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"

namespace isekai {

template <>
absl::Status IntraConnectionRoundRobinPolicy<PacketTypeQueue>::InitConnection(
    uint32_t scid) {
  if (policy_state_.contains(scid)) {
    return absl::AlreadyExistsError(
        "Duplicate source connection ID. Queues already exist.");
  }
  std::vector<PacketTypeQueue> queue_types(
      {PacketTypeQueue::kPullAndOrderedPushRequest,
       PacketTypeQueue::kUnorderedPushRequest, PacketTypeQueue::kPushData,
       PacketTypeQueue::kPushGrant, PacketTypeQueue::kPullData});

  // Create connection state and initialize all queues.
  ConnectionPolicyState state;
  for (const auto& queue : queue_types) {
    state.is_active[queue] = false;
    state.inactive_queues.push_back(queue);
  }
  policy_state_[scid] = state;
  return absl::OkStatus();
}

template <>
absl::Status IntraConnectionRoundRobinPolicy<WindowTypeQueue>::InitConnection(
    uint32_t scid) {
  std::vector<WindowTypeQueue> queue_types(
      {WindowTypeQueue::kRequest, WindowTypeQueue::kData});

  // Create connection state and initialize all queues.
  ConnectionPolicyState state;
  for (const auto& queue : queue_types) {
    state.is_active[queue] = false;
    state.inactive_queues.push_back(queue);
  }
  policy_state_[scid] = state;
  return absl::OkStatus();
}

}  // namespace isekai
