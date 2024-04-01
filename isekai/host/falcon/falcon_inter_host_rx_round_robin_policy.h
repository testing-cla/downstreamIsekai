#ifndef ISEKAI_HOST_FALCON_FALCON_PER_HOST_RX_ROUND_ROBIN_POLICY_H_
#define ISEKAI_HOST_FALCON_FALCON_PER_HOST_RX_ROUND_ROBIN_POLICY_H_

#include <cstdint>
#include <list>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"

namespace isekai {

class InterHostRxSchedulingRoundRobinPolicy
    : public InterHostRxSchedulingPolicy {
 public:
  InterHostRxSchedulingRoundRobinPolicy();
  // Initializes the host in the context of the policy.

  absl::Status InitHost(uint8_t bifurcation_id) override;

  // Selects the next host that needs to be scheduled across various
  // available hosts.
  absl::StatusOr<uint8_t> SelectHost() override;
  // Marks a host Rx queue as active or inactive.
  void MarkHostActive(uint8_t bifurcation_id) override;
  void MarkHostInactive(uint8_t bifurcation_id) override;
  // Xoff and xon a particular host Rx queue.
  void XoffHost(uint8_t bifurcation_id) override;
  void XonHost(uint8_t bifurcation_id) override;
  // Returns whether the policy has schedulable work (has active host).
  bool HasWork() override;

 private:
  // Per-Host policy state.
  struct HostPolicyState {
    // Whether there are transactions enqueued in the host queue to be sent to
    // RDMA.
    bool is_active;
    // Whether the host is XoFF-ed by ULP due to bottleneck in ULP causing the
    // Rx buffer to get exhausted.
    bool is_xoff;
    // Iterator pointing to this host in either the active or inactive
    // list. We store this to enable constant time removal from either list.
    std::list<uint8_t>::iterator iterator;
  };

  // Represents the list of hosts that are being handled by this policy,
  // along with the round robin policy state.
  absl::flat_hash_map<uint8_t, HostPolicyState> host_state_;

  // List of active and inactive host in round-robin order. An active
  // host is defined as one having an enqueued transaction.
  std::list<uint8_t> active_hosts_;
  std::list<uint8_t> inactive_hosts_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_PER_HOST_RX_ROUND_ROBIN_POLICY_H_
