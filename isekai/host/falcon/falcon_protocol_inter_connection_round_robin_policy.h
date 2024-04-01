#ifndef ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTER_CONNECTION_ROUND_ROBIN_POLICY_H_
#define ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTER_CONNECTION_ROUND_ROBIN_POLICY_H_

#include <cstdint>
#include <list>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"

namespace isekai {

class InterConnectionRoundRobinPolicy : public InterConnectionSchedulingPolicy {
 public:
  InterConnectionRoundRobinPolicy();
  // Adds the connection ID to the internal list of connection IDs.
  absl::Status InitConnection(uint32_t scid) override;
  // Selects a particular connection that needs to be scheduled across various
  // available connections.
  absl::StatusOr<uint32_t> SelectConnection() override;
  // Marks a connection as active/inactive.
  void MarkConnectionActive(uint32_t scid) override;
  void MarkConnectionInactive(uint32_t scid) override;
  // Returns whether the policy has work available.
  bool HasWork() override { return !active_connections_.empty(); }

 private:
  // Per-connection policy state.
  struct ConnectionPolicyState {
    // Whether the connection has any transaction enqueued into it.
    bool is_active;
    // Iterator pointing to this connection in either the active or inactive
    // list. We store this to enable constant time removal from either list.
    std::list<uint32_t>::iterator iterator;
  };

  // Represents the list of connections that are being handled by this policy,
  // along with round robin policy state.
  absl::flat_hash_map<uint32_t, ConnectionPolicyState> connection_state_;

  // List of active and inactive connection in round-robin order. An active
  // connection is defined as one having an enqueued transaction.
  std::list<uint32_t> active_connections_;
  std::list<uint32_t> inactive_connections_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTER_CONNECTION_ROUND_ROBIN_POLICY_H_
