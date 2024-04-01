#include "isekai/host/falcon/falcon_protocol_inter_connection_round_robin_policy.h"

#include <cstdint>
#include <iterator>
#include <list>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/meta/type_traits.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "glog/logging.h"

namespace isekai {

InterConnectionRoundRobinPolicy::InterConnectionRoundRobinPolicy() {}

absl::Status InterConnectionRoundRobinPolicy::InitConnection(uint32_t scid) {
  if (connection_state_.contains(scid)) {
    return absl::AlreadyExistsError(
        "Duplicate source connection ID. Queues already exist.");
  }

  // Initialize connection state, and add scid to list of inactive connections.
  inactive_connections_.push_back(scid);
  ConnectionPolicyState connection_state{
      .is_active = false,
      .iterator = std::prev(inactive_connections_.end()),
  };
  connection_state_[scid] = connection_state;
  return absl::OkStatus();
}

absl::StatusOr<uint32_t> InterConnectionRoundRobinPolicy::SelectConnection() {
  if (active_connections_.empty()) {
    return absl::UnavailableError(
        "No outstanding work from the connections currently.");
  }

  // Return the scid at the front of active connections.
  uint32_t connection_id = active_connections_.front();

  // Remove it from the head, append it to the back and update connection state.
  active_connections_.pop_front();
  active_connections_.push_back(connection_id);
  connection_state_[connection_id].iterator =
      std::prev(active_connections_.end());
  return connection_id;
}

void InterConnectionRoundRobinPolicy::MarkConnectionActive(uint32_t scid) {
  auto it = connection_state_.find(scid);
  CHECK(it != connection_state_.end()) << "Connection does not exist.";

  ConnectionPolicyState& state = it->second;
  if (state.is_active) {
    return;
  } else {
    state.is_active = true;
    inactive_connections_.erase(state.iterator);
    active_connections_.push_back(scid);
    state.iterator = std::prev(active_connections_.end());
  }
}

void InterConnectionRoundRobinPolicy::MarkConnectionInactive(uint32_t scid) {
  auto it = connection_state_.find(scid);
  CHECK(it != connection_state_.end()) << "Connection does not exist.";

  ConnectionPolicyState& state = it->second;
  if (!state.is_active) {
    return;
  } else {
    state.is_active = false;
    active_connections_.erase(state.iterator);
    inactive_connections_.push_back(scid);
    state.iterator = std::prev(inactive_connections_.end());
  }
}

}  // namespace isekai
