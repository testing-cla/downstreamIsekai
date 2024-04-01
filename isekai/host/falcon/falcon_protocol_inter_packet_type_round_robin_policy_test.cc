#include "isekai/host/falcon/falcon_protocol_inter_packet_type_round_robin_policy.h"

#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"

namespace isekai {

namespace {

TEST(ProtocolInterPacketTypeRoundRobinPolicy, PacketSelectionOrder) {
  InterPacketTypeRoundRobinPolicy policy;

  // Expect no work as no queue is active in the beginning.
  EXPECT_EQ(policy.SelectPacketTypeBasedQueueType().status().code(),
            absl::StatusCode::kUnavailable);

  // Mark push data queue active.
  policy.MarkPacketTypeBasedQueueTypeActive(PacketTypeQueue::kPushData);

  // Picks push data queue.
  ASSERT_OK_THEN_ASSIGN(auto queue_type1,
                        policy.SelectPacketTypeBasedQueueType());
  EXPECT_EQ(queue_type1, PacketTypeQueue::kPushData);

  // Picks push data queue again as it is the only active one.
  ASSERT_OK_THEN_ASSIGN(auto queue_type2,
                        policy.SelectPacketTypeBasedQueueType());
  EXPECT_EQ(queue_type2, PacketTypeQueue::kPushData);

  // Mark push grant and pull data  queues active, and push data queue inactive.
  policy.MarkPacketTypeBasedQueueTypeActive(PacketTypeQueue::kPushGrant);
  policy.MarkPacketTypeBasedQueueTypeActive(PacketTypeQueue::kPullData);
  policy.MarkPacketTypeBasedQueueTypeInactive(PacketTypeQueue::kPushData);

  // Picks push grant and pull data in round robin order.
  ASSERT_OK_THEN_ASSIGN(auto queue_type3,
                        policy.SelectPacketTypeBasedQueueType());
  EXPECT_EQ(queue_type3, PacketTypeQueue::kPushGrant);

  ASSERT_OK_THEN_ASSIGN(auto queue_type4,
                        policy.SelectPacketTypeBasedQueueType());
  EXPECT_EQ(queue_type4, PacketTypeQueue::kPullData);

  ASSERT_OK_THEN_ASSIGN(auto queue_type5,
                        policy.SelectPacketTypeBasedQueueType());
  EXPECT_EQ(queue_type5, PacketTypeQueue::kPushGrant);

  ASSERT_OK_THEN_ASSIGN(auto queue_type6,
                        policy.SelectPacketTypeBasedQueueType());
  EXPECT_EQ(queue_type6, PacketTypeQueue::kPullData);
}

TEST(ProtocolInterPacketTypeRoundRobinPolicy, HasWorkCriteria) {
  InterPacketTypeRoundRobinPolicy policy;

  // No work is expected as there are no queues that are active.
  EXPECT_FALSE(policy.HasWork());

  policy.MarkPacketTypeBasedQueueTypeActive(PacketTypeQueue::kPushGrant);

  // Work is expected as there is an active queue.
  EXPECT_TRUE(policy.HasWork());
}

}  // namespace
}  // namespace isekai
