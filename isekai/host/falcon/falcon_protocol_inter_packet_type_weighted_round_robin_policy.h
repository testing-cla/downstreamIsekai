#ifndef ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTER_PACKET_TYPE_WEIGHTED_ROUND_ROBIN_POLICY_H_
#define ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTER_PACKET_TYPE_WEIGHTED_ROUND_ROBIN_POLICY_H_

#include "isekai/host/falcon/falcon_component_interfaces.h"

namespace isekai {

class InterPacketTypeWeightedRoundRobinPolicy
    : public InterPacketSchedulingPolicy {
 public:
  explicit InterPacketTypeWeightedRoundRobinPolicy(Scheduler* scheduler);
  // Selects a particular packet type based queue type for scheduling.
  absl::StatusOr<PacketTypeQueue> SelectPacketTypeBasedQueueType() override;
  // Marks a queue as active or inactive.
  void MarkPacketTypeBasedQueueTypeActive(
      PacketTypeQueue packet_queue_type) override;
  void MarkPacketTypeBasedQueueTypeInactive(
      PacketTypeQueue packet_queue_type) override;
  // Returns whether the policy has work to do.
  bool HasWork() const override;
  // Used to update the policy state due to an external event (e.g., all
  // connections are not eligible).
  void UpdatePolicyState(bool are_all_packet_types_blocked) override;
  // Function that returns the active/inactive status of a packet queue type.
  bool GetPacketTypeStateForTesting(PacketTypeQueue queue_type);
  uint64_t GetTotalAvailableCreditsForTesting();

 private:
  // Computes the per packet credits for each arbitration round.
  void ComputePerPacketCredits();
  Scheduler& scheduler_;

  // Per packet policy state.
  struct PacketTypePolicyState {
    bool is_active;
    uint64_t available_credits;
    PacketTypePolicyState() {}
    PacketTypePolicyState(bool is_active, uint64_t available_credits)
        : is_active(is_active), available_credits(available_credits) {}
  };
  // Stores the per-packet type policy state.
  absl::flat_hash_map<PacketTypeQueue, PacketTypePolicyState> policy_state_;
  // Reflects the total available credits in the current arbitration round.
  uint64_t total_available_credits_;
  // List of active and inactive queues (in round robin order);
  std::list<PacketTypeQueue> active_queues_;
  std::list<PacketTypeQueue> inactive_queues_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_PROTOCOL_INTER_PACKET_TYPE_WEIGHTED_ROUND_ROBIN_POLICY_H_
