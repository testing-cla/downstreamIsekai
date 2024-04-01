#include "isekai/host/falcon/falcon_inter_host_rx_round_robin_policy.h"

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

// Constructor.
InterHostRxSchedulingRoundRobinPolicy::InterHostRxSchedulingRoundRobinPolicy() {
}

// Initializes host state and queue for the host. By default, host state is
// marked inactive since there are no outstanding packets and the xoff is set to
// false since the RDMA Rx buffer will be empty.
absl::Status InterHostRxSchedulingRoundRobinPolicy::InitHost(
    uint8_t bifurcation_id) {
  if (host_state_.contains(bifurcation_id)) {
    LOG(FATAL) << "Duplicate host ID. Queues already exist.";
  }

  // Initialize host state, and add bifurcation_id to list of inactve hosts.
  inactive_hosts_.push_back(bifurcation_id);
  HostPolicyState host_state{
      .is_active = false,
      .is_xoff = false,
      .iterator = std::prev(inactive_hosts_.end()),
  };
  host_state_[bifurcation_id] = host_state;
  return absl::OkStatus();
}

// Selects the next host that needs to be scheduled across various available
// hosts.
absl::StatusOr<uint8_t> InterHostRxSchedulingRoundRobinPolicy::SelectHost() {
  if (active_hosts_.empty()) {
    return absl::UnavailableError(
        "No outstanding work from the hosts currently.");
  }

  // Return the bifurcation_id at the front of active hosts.
  uint8_t bifurcation_id = active_hosts_.front();

  // Remove it from the head, append it to the back and update host state.
  active_hosts_.pop_front();
  active_hosts_.push_back(bifurcation_id);
  host_state_[bifurcation_id].iterator = std::prev(active_hosts_.end());
  return bifurcation_id;
}

// Marks a host as active. The corresponding host will be made eligible for
// selection in the next arbitration.
void InterHostRxSchedulingRoundRobinPolicy::MarkHostActive(
    uint8_t bifurcation_id) {
  auto it = host_state_.find(bifurcation_id);
  CHECK(it != host_state_.end())
      << "Host " << static_cast<uint32_t>(bifurcation_id) << " does not exist.";

  HostPolicyState& state = it->second;
  if (state.is_active) {
    return;
  } else {
    state.is_active = true;
    if (!state.is_xoff) {
      inactive_hosts_.erase(state.iterator);
      active_hosts_.push_back(bifurcation_id);
      state.iterator = std::prev(active_hosts_.end());
    }
  }
}

// Marks a host as inactive when there are no outstanding packets for this host.
// The policy will not consider the corresponding host in next arbitration.
void InterHostRxSchedulingRoundRobinPolicy::MarkHostInactive(
    uint8_t bifurcation_id) {
  auto it = host_state_.find(bifurcation_id);
  CHECK(it != host_state_.end())
      << "Host " << static_cast<uint32_t>(bifurcation_id) << " does not exist.";

  HostPolicyState& state = it->second;
  if (!state.is_active) {
    return;
  } else {
    state.is_active = false;
    if (!state.is_xoff) {
      active_hosts_.erase(state.iterator);
      inactive_hosts_.push_back(bifurcation_id);
      state.iterator = std::prev(inactive_hosts_.end());
    }
  }
}

// Xoffs the correspinding host Rx queue. The policy will not consider the
// corresponding host in next arbitration.
void InterHostRxSchedulingRoundRobinPolicy::XoffHost(uint8_t bifurcation_id) {
  auto it = host_state_.find(bifurcation_id);
  CHECK(it != host_state_.end())
      << "Host " << static_cast<uint32_t>(bifurcation_id) << " does not exist.";

  HostPolicyState& state = it->second;
  if (state.is_xoff) {
    return;
  } else {
    state.is_xoff = true;
    if (state.is_active) {
      active_hosts_.erase(state.iterator);
      inactive_hosts_.push_back(bifurcation_id);
      state.iterator = std::prev(inactive_hosts_.end());
    }
  }
}

// Xons the corresponding host Rx queue. The corresponding host will be made
// eligible for selection in the next arbitration.
void InterHostRxSchedulingRoundRobinPolicy::XonHost(uint8_t bifurcation_id) {
  auto it = host_state_.find(bifurcation_id);
  CHECK(it != host_state_.end())
      << "Host " << static_cast<uint32_t>(bifurcation_id) << " does not exist.";

  HostPolicyState& state = it->second;
  if (!state.is_xoff) {
    return;
  } else {
    state.is_xoff = false;
    if (state.is_active) {
      inactive_hosts_.erase(state.iterator);
      active_hosts_.push_back(bifurcation_id);
      state.iterator = std::prev(active_hosts_.end());
    }
  }
}

// Checks if there is any outstanding work.
bool InterHostRxSchedulingRoundRobinPolicy::HasWork() {
  return !active_hosts_.empty();
}

}  // namespace isekai
