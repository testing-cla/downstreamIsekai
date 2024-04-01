#include "isekai/host/falcon/falcon_protocol_intra_packet_type_round_robin_policy.h"

#include "glog/logging.h"

namespace isekai {

namespace {
constexpr std::array<PacketTypeQueue, 5> queue_types = {
    PacketTypeQueue::kPullAndOrderedPushRequest,
    PacketTypeQueue::kUnorderedPushRequest, PacketTypeQueue::kPushData,
    PacketTypeQueue::kPushGrant, PacketTypeQueue::kPullData};
}

IntraPacketTypeRoundRobinPolicy::IntraPacketTypeRoundRobinPolicy() {
  for (const auto& queue : queue_types) {
    policy_state_[queue] = PacketTypePolicyState();
  }
}

// Initializes the per connection queues corresponding to the various packet
// types and metadata in the context of the policy (e.g., marking the connection
// inactive).
void IntraPacketTypeRoundRobinPolicy::InitConnection(uint32_t scid) {
  for (const auto& queue : queue_types) {
    CHECK(!policy_state_[queue].is_active.contains(scid))
        << "Duplicate source connection ID. Queues already exist.";

    policy_state_[queue].is_active[scid] = false;
    policy_state_[queue].inactive_connections.push_back(scid);
    policy_state_[queue].iterators[scid] =
        std::prev(policy_state_[queue].inactive_connections.end());
  }
}

// Selects connection in a round robin manner from active connection list by
// choosing the head and moving it to the tail of the list.
absl::StatusOr<uint32_t> IntraPacketTypeRoundRobinPolicy::SelectConnection(
    PacketTypeQueue packet_queue_type) {
  auto& state = policy_state_[packet_queue_type];
  if (state.active_connections.empty()) {
    return absl::UnavailableError(
        "No outstanding work from the connections currently.");
  }

  // Return the scid at the front of active connections.
  uint32_t connection_id = state.active_connections.front();

  // Remove it from the head, append it to the back and update connection state.
  state.active_connections.pop_front();
  state.active_connections.push_back(connection_id);
  state.iterators[connection_id] = std::prev(state.active_connections.end());
  return connection_id;
}

// Marks a connection, corresponding to the given packet type, active by
// updating the relative policy metadata and adding the connection to the active
// connection pool.
void IntraPacketTypeRoundRobinPolicy::MarkConnectionActive(
    uint32_t scid, PacketTypeQueue packet_queue_type) {
  auto& state = policy_state_[packet_queue_type];

  auto active_status = state.is_active.find(scid);
  CHECK(active_status != state.is_active.end());
  if (active_status->second) {
    return;
  }
  active_status->second = true;
  state.inactive_connections.erase(state.iterators[scid]);
  state.active_connections.push_back(scid);
  state.iterators[scid] = std::prev(state.active_connections.end());
}

// Marks a connection, corresponding to the given packet type, inactive by
// updating the relevant policy metadata and moving the connection to the
// inactive pool.
void IntraPacketTypeRoundRobinPolicy::MarkConnectionInactive(
    uint32_t scid, PacketTypeQueue packet_queue_type) {
  auto& state = policy_state_[packet_queue_type];

  auto active_status = state.is_active.find(scid);
  CHECK(active_status != state.is_active.end());
  if (!active_status->second) {
    return;
  }
  active_status->second = false;
  state.active_connections.erase(state.iterators[scid]);
  state.inactive_connections.push_back(scid);
  state.iterators[scid] = std::prev(state.inactive_connections.end());
}

// Determines if a particular packet queue type has outstanding work based on
// whether it has any active connections.
bool IntraPacketTypeRoundRobinPolicy::HasWork(
    PacketTypeQueue packet_queue_type) {
  return !policy_state_[packet_queue_type].active_connections.empty();
}

}  // namespace isekai
