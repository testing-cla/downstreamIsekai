#ifndef ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTRA_CONNECTION_ROUND_ROBIN_POLICY_H_
#define ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTRA_CONNECTION_ROUND_ROBIN_POLICY_H_

#include <cstdint>
#include <list>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "glog/logging.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"

namespace isekai {

template <typename T>
class IntraConnectionRoundRobinPolicy
    : public IntraConnectionSchedulingPolicy<T> {
 public:
  IntraConnectionRoundRobinPolicy<T>() {}
  // Adds the connection ID to the internal list of connection IDs.
  absl::Status InitConnection(uint32_t scid) override;
  // Selects the next intra connection queue for scheduling.
  absl::StatusOr<T> SelectQueue(uint32_t scid) override;
  // Marks a queue as active, making it eligible for scheduling.
  void MarkQueueActive(uint32_t scid, T queue_type) override;
  // Marks a queue as inactive, making it ineligible for scheduling.
  void MarkQueueInactive(uint32_t scid, T queue_type) override;
  // Returns whether the poliy for the connection has available work.
  bool HasWork(uint32_t scid) override;

 private:
  // Per-connection policy state.
  struct ConnectionPolicyState {
    // Indicates whether a queue type has enqueued transactions.
    absl::flat_hash_map<T, bool> is_active;
    // List of active and inactive queues (in round robin order).
    std::list<T> active_queues;
    std::list<T> inactive_queues;
  };

  absl::flat_hash_map<uint32_t, ConnectionPolicyState> policy_state_;
};

template <typename T>
absl::StatusOr<T> IntraConnectionRoundRobinPolicy<T>::SelectQueue(
    uint32_t scid) {
  auto it = policy_state_.find(scid);
  CHECK(it != policy_state_.end()) << "Connection does not exist.";

  ConnectionPolicyState& state = it->second;
  if (state.active_queues.empty()) {
    return absl::UnavailableError(
        "No outstanding work from the intra connection queues currently.");
  }
  // Return the queue at the front of active connections.
  T queue_type = state.active_queues.front();
  // Remove it from the head, append it at the back and update policy state.
  state.active_queues.pop_front();
  state.active_queues.push_back(queue_type);
  return queue_type;
}

template <typename T>
void IntraConnectionRoundRobinPolicy<T>::MarkQueueActive(uint32_t scid,
                                                         T queue_type) {
  auto it = policy_state_.find(scid);
  CHECK(it != policy_state_.end()) << "Connection does not exist.";
  ConnectionPolicyState& state = it->second;

  auto active_status = state.is_active.find(queue_type);
  CHECK(active_status != state.is_active.end());
  if (active_status->second) {
    return;
  } else {
    active_status->second = true;
    state.inactive_queues.remove(queue_type);
    state.active_queues.push_back(queue_type);
  }
}

template <typename T>
void IntraConnectionRoundRobinPolicy<T>::MarkQueueInactive(uint32_t scid,
                                                           T queue_type) {
  auto it = policy_state_.find(scid);
  CHECK(it != policy_state_.end()) << "Connection does not exist.";
  ConnectionPolicyState& state = it->second;

  auto active_status = state.is_active.find(queue_type);
  CHECK(active_status != state.is_active.end());
  if (!active_status->second) {
    return;
  } else {
    active_status->second = false;
    state.active_queues.remove(queue_type);
    state.inactive_queues.push_back(queue_type);
  }
}

template <typename T>
bool IntraConnectionRoundRobinPolicy<T>::HasWork(uint32_t scid) {
  auto it = policy_state_.find(scid);
  CHECK(it != policy_state_.end()) << "Connection does not exist.";

  ConnectionPolicyState& state = it->second;
  return !state.active_queues.empty();
}

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTRA_CONNECTION_ROUND_ROBIN_POLICY_H_
