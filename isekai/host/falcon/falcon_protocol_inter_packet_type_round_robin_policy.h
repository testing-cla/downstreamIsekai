#ifndef ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTER_PACKET_TYPE_ROUND_ROBIN_POLICY_H_
#define ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTER_PACKET_TYPE_ROUND_ROBIN_POLICY_H_

#include <list>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"

namespace isekai {

class InterPacketTypeRoundRobinPolicy : public InterPacketSchedulingPolicy {
 public:
  explicit InterPacketTypeRoundRobinPolicy();
  // Selects a particular packet type based queue type for scheduling.
  absl::StatusOr<PacketTypeQueue> SelectPacketTypeBasedQueueType() override;
  // Marks a queue as active or inactive.
  void MarkPacketTypeBasedQueueTypeActive(
      PacketTypeQueue packet_queue_type) override;
  void MarkPacketTypeBasedQueueTypeInactive(
      PacketTypeQueue packet_queue_type) override;
  // Returns whether the policy has work to do.
  bool HasWork() const override;
  // Given that the policy is stateless, no external event impacts the policy
  // state.
  void UpdatePolicyState(bool are_all_packet_types_blocked) override {}

 private:
  // Stores the per-packet type active/inactive status.
  absl::flat_hash_map<PacketTypeQueue, bool> is_active_;
  // List of active and inactive queues (in round robin order);
  std::list<PacketTypeQueue> active_queues_;
  std::list<PacketTypeQueue> inactive_queues_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTER_PACKET_TYPE_ROUND_ROBIN_POLICY_H_
