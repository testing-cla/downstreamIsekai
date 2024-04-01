#include "isekai/host/falcon/falcon_protocol_inter_packet_type_weighted_round_robin_policy.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "isekai/host/falcon/falcon.h"

namespace isekai {

namespace {

class FakeConnectionScheduler : public Scheduler {
 public:
  FakeConnectionScheduler() {
    // Initialize the scheduler queue related counters (used for scheduling).
    std::vector<PacketTypeQueue> queue_types(
        {PacketTypeQueue::kPullAndOrderedPushRequest,
         PacketTypeQueue::kUnorderedPushRequest, PacketTypeQueue::kPushData,
         PacketTypeQueue::kPushGrant, PacketTypeQueue::kPullData});

    for (const auto& queue : queue_types) {
      transaction_queue_wise_pkt_count_[queue] = 0;
    }
  }
  // Initializes the packet count corresponding to a given queue type.
  void InitQueuePacketCount(PacketTypeQueue queue_type, uint64_t count) {
    transaction_queue_wise_pkt_count_[queue_type] = count;
  }
  // Returns the number of outstanding packets corresponding to a transaction
  // queue type.
  uint64_t GetQueueTypeBasedOutstandingPacketCount(
      PacketTypeQueue queue_type) override {
    return transaction_queue_wise_pkt_count_[queue_type];
  }
  // Initializes the various intra-connection scheduler queues.
  absl::Status InitConnectionSchedulerQueues(uint32_t scid) {
    return absl::OkStatus();
  }
  // Add a transaction to the relevant queue for transmitting over the network
  absl::Status EnqueuePacket(uint32_t scid, uint32_t rsn,
                             falcon::PacketType type) {
    return absl::OkStatus();
  }
  // Removes a transaction to the relevant queue for transmitting over the
  // network
  absl::Status DequeuePacket(uint32_t scid, uint32_t rsn,
                             falcon::PacketType type) {
    return absl::OkStatus();
  }
  // Returns true if the connection scheduler has outstanding work.
  bool HasWork() { return true; }
  // Performs one unit of work from the connection scheduler.
  bool ScheduleWork() { return true; }
  // Return the queue length of an inter-connection queue.
  uint32_t GetConnectionQueueLength(uint32_t scid) { return 0; }

 private:
  absl::flat_hash_map<PacketTypeQueue, uint64_t>
      transaction_queue_wise_pkt_count_;
};

TEST(ProtocolInterPacketTypeWeightedRoundRobinPolicy, PacketSelectionOrder) {
  FakeConnectionScheduler scheduler;
  InterPacketTypeWeightedRoundRobinPolicy policy(&scheduler);

  // Initialize packet count for various queue types. Weights are assigned in
  // multiples of 64.
  scheduler.InitQueuePacketCount(PacketTypeQueue::kPushData, 65);
  policy.MarkPacketTypeBasedQueueTypeActive(PacketTypeQueue::kPushData);

  scheduler.InitQueuePacketCount(PacketTypeQueue::kPushGrant, 64);
  scheduler.InitQueuePacketCount(PacketTypeQueue::kPullData, 129);
  policy.MarkPacketTypeBasedQueueTypeActive(PacketTypeQueue::kPushGrant);
  policy.MarkPacketTypeBasedQueueTypeActive(PacketTypeQueue::kPullData);

  // 1. Push Data is picked up twice in the first arbitration round.
  EXPECT_EQ(policy.SelectPacketTypeBasedQueueType().value(),
            PacketTypeQueue::kPushData);
  policy.UpdatePolicyState(false);
  EXPECT_EQ(policy.SelectPacketTypeBasedQueueType().value(),
            PacketTypeQueue::kPushData);
  policy.UpdatePolicyState(false);

  // 2. Push Data is picked up first in the second arbitration round.
  EXPECT_EQ(policy.SelectPacketTypeBasedQueueType().value(),
            PacketTypeQueue::kPushData);
  policy.UpdatePolicyState(false);

  // 3. Push Grant is picked up next in the second arbitration round.
  EXPECT_EQ(policy.SelectPacketTypeBasedQueueType().value(),
            PacketTypeQueue::kPushGrant);
  policy.UpdatePolicyState(false);

  // 4. Pull Data is picked up next in the second arbitration round.
  EXPECT_EQ(policy.SelectPacketTypeBasedQueueType().value(),
            PacketTypeQueue::kPullData);
  policy.UpdatePolicyState(false);

  // 5. Push Data is picked up first in the third arbitration round.
  EXPECT_EQ(policy.SelectPacketTypeBasedQueueType().value(),
            PacketTypeQueue::kPushData);
  policy.UpdatePolicyState(false);
  // 6. Pull Data is picked up next in the third arbitration round.
  EXPECT_EQ(policy.SelectPacketTypeBasedQueueType().value(),
            PacketTypeQueue::kPullData);
  policy.UpdatePolicyState(false);
  // 7. Pull Data is picked up next in the third arbitration round.
  EXPECT_EQ(policy.SelectPacketTypeBasedQueueType().value(),
            PacketTypeQueue::kPullData);
  policy.UpdatePolicyState(false);
}

TEST(ProtocolInterPacketTypeWeightedRoundRobinPolicy, HasWork) {
  FakeConnectionScheduler scheduler;
  InterPacketTypeWeightedRoundRobinPolicy policy(&scheduler);

  // On start up, no work is expected.
  EXPECT_FALSE(policy.HasWork());

  // Initialize packet count for a queue and mark it active.
  scheduler.InitQueuePacketCount(PacketTypeQueue::kPushData, 65);
  policy.MarkPacketTypeBasedQueueTypeActive(PacketTypeQueue::kPushData);

  // Given that a queue is active, we expect work to be present.
  EXPECT_TRUE(policy.HasWork());
}

TEST(ProtocolInterPacketTypeWeightedRoundRobinPolicy,
     MarkPacketTypeBasedQueueTypeActive) {
  FakeConnectionScheduler scheduler;
  InterPacketTypeWeightedRoundRobinPolicy policy(&scheduler);

  // Attempt to mark a queue active.
  auto queue_type = PacketTypeQueue::kPushData;
  policy.MarkPacketTypeBasedQueueTypeActive(queue_type);

  // Expect false as no outstanding packets corresponding to the above queue.
  EXPECT_FALSE(policy.GetPacketTypeStateForTesting(queue_type));

  // Initialize packet count for a queue and mark it active.
  scheduler.InitQueuePacketCount(PacketTypeQueue::kPushData, 65);
  policy.MarkPacketTypeBasedQueueTypeActive(queue_type);

  // Expect true as no outstanding packets corresponding to the above queue.
  EXPECT_TRUE(policy.GetPacketTypeStateForTesting(queue_type));
}

TEST(ProtocolInterPacketTypeWeightedRoundRobinPolicy,
     MarkPacketTypeBasedQueueTypeInactive) {
  FakeConnectionScheduler scheduler;
  InterPacketTypeWeightedRoundRobinPolicy policy(&scheduler);

  // Initialize packet count for a queue and mark it active.
  auto queue_type = PacketTypeQueue::kPushData;

  scheduler.InitQueuePacketCount(PacketTypeQueue::kPushData, 65);
  policy.MarkPacketTypeBasedQueueTypeActive(queue_type);

  policy.MarkPacketTypeBasedQueueTypeInactive(queue_type);

  // Expect false as queue is marked inactive above.
  EXPECT_FALSE(policy.GetPacketTypeStateForTesting(queue_type));
}

TEST(ProtocolInterPacketTypeWeightedRoundRobinPolicy, UpdatePolicy) {
  FakeConnectionScheduler scheduler;
  InterPacketTypeWeightedRoundRobinPolicy policy(&scheduler);

  // Initialize packet count for a queue.
  auto queue_type = PacketTypeQueue::kPushData;

  scheduler.InitQueuePacketCount(PacketTypeQueue::kPushData, 65);
  policy.MarkPacketTypeBasedQueueTypeActive(queue_type);

  policy.UpdatePolicyState(/* are_all_packet_types_blocked = */ false);

  // Expect true as the policy state is updated.
  EXPECT_TRUE(policy.GetPacketTypeStateForTesting(queue_type));
  EXPECT_EQ(policy.GetTotalAvailableCreditsForTesting(), 2);

  // Reduce the packet count for the queue
  scheduler.InitQueuePacketCount(PacketTypeQueue::kPushData, 64);

  policy.UpdatePolicyState(/* are_all_packet_types_blocked = */ true);

  // Expect true as the policy state is updated.
  EXPECT_TRUE(policy.GetPacketTypeStateForTesting(queue_type));
  EXPECT_EQ(policy.GetTotalAvailableCreditsForTesting(), 1);
}

}  // namespace
}  // namespace isekai
