#ifndef ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTRA_PACKET_TYPE_ROUND_ROBIN_POLICY_H_
#define ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTRA_PACKET_TYPE_ROUND_ROBIN_POLICY_H_

#include "isekai/host/falcon/falcon_component_interfaces.h"

namespace isekai {

// Reflects the round robin policy of selecting connections within a packet
// type. This policy is invoked once a packet type is selected for arbitration.
class IntraPacketTypeRoundRobinPolicy : public IntraPacketSchedulingPolicy {
 public:
  IntraPacketTypeRoundRobinPolicy();
  // Adds the connection ID to the internal list of connection IDs.
  void InitConnection(uint32_t scid) override;

  // Given a packet type, selects a particular connection that needs to be
  // scheduled across various available active connections in round robin
  // manner.
  absl::StatusOr<uint32_t> SelectConnection(
      PacketTypeQueue packet_queue_type) override;

  // Marks a connection corresponding to a packet type as active by adding it to
  // the active list. A connection is marked active when it has outstanding
  // packets.
  void MarkConnectionActive(uint32_t scid,
                            PacketTypeQueue packet_queue_type) override;
  // Marks a connection corresponding to a packet type as inactive by adding it
  // to the inactive list. A connection is marked inactive when it has no
  // outstanding packets.
  void MarkConnectionInactive(uint32_t scid,
                              PacketTypeQueue packet_queue_type) override;

  // Return true if the corresponding packet type is active, i.e., when the
  // active list has elements.
  bool HasWork(PacketTypeQueue packet_queue_type) override;

 private:
  // Per packet type policy state.
  struct PacketTypePolicyState {
    // Indicates whether a given connection has enqueued packets.
    absl::flat_hash_map<uint32_t, bool> is_active;
    // Iterator pointing to a connection in either the active or inactive list.
    // We store this to enable constant time removal from either list.
    absl::flat_hash_map<uint32_t, std::list<uint32_t>::iterator> iterators;
    // List of active and inactive connections (in round robin order).
    std::list<uint32_t> active_connections;
    std::list<uint32_t> inactive_connections;
  };

  absl::flat_hash_map<PacketTypeQueue, PacketTypePolicyState> policy_state_;
};
}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTRA_PACKET_TYPE_ROUND_ROBIN_POLICY_H_
