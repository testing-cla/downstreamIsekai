#include "isekai/host/falcon/falcon_protocol_intra_connection_round_robin_policy.h"

#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "internal/testing.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"

namespace isekai {
namespace {

TEST(ProtocolIntraConnectionRoundRobinPolicyTest, RoundRobinWorkOrder) {
  IntraConnectionRoundRobinPolicy<PacketTypeQueue> policy;
  EXPECT_OK(policy.InitConnection(1));
  policy.MarkQueueActive(1, PacketTypeQueue::kPullAndOrderedPushRequest);
  policy.MarkQueueActive(1, PacketTypeQueue::kUnorderedPushRequest);
  policy.MarkQueueActive(1, PacketTypeQueue::kPushData);
  policy.MarkQueueActive(1, PacketTypeQueue::kPushGrant);
  policy.MarkQueueActive(1, PacketTypeQueue::kPullData);

  EXPECT_EQ(policy.SelectQueue(1).value(),
            PacketTypeQueue::kPullAndOrderedPushRequest);
  EXPECT_EQ(policy.SelectQueue(1).value(),
            PacketTypeQueue::kUnorderedPushRequest);
  EXPECT_EQ(policy.SelectQueue(1).value(), PacketTypeQueue::kPushData);
  EXPECT_EQ(policy.SelectQueue(1).value(), PacketTypeQueue::kPushGrant);
  EXPECT_EQ(policy.SelectQueue(1).value(), PacketTypeQueue::kPullData);

  policy.MarkQueueInactive(1, PacketTypeQueue::kPullAndOrderedPushRequest);

  // kPullAndOrderedPushRequest should be removed.
  EXPECT_EQ(policy.SelectQueue(1).value(),
            PacketTypeQueue::kUnorderedPushRequest);
  EXPECT_EQ(policy.SelectQueue(1).value(), PacketTypeQueue::kPushData);
  EXPECT_EQ(policy.SelectQueue(1).value(), PacketTypeQueue::kPushGrant);
  EXPECT_EQ(policy.SelectQueue(1).value(), PacketTypeQueue::kPullData);

  policy.MarkQueueInactive(1, PacketTypeQueue::kPushGrant);

  // kPushGrant should be removed.
  EXPECT_EQ(policy.SelectQueue(1).value(),
            PacketTypeQueue::kUnorderedPushRequest);
  EXPECT_EQ(policy.SelectQueue(1).value(), PacketTypeQueue::kPushData);
  EXPECT_EQ(policy.SelectQueue(1).value(), PacketTypeQueue::kPullData);
}

TEST(ProtocolIntraConnectionRoundRobinPolicyTest, RoundRobinWindowWorkOrder) {
  IntraConnectionRoundRobinPolicy<WindowTypeQueue> policy;
  EXPECT_OK(policy.InitConnection(1));
  policy.MarkQueueActive(1, WindowTypeQueue::kRequest);
  policy.MarkQueueActive(1, WindowTypeQueue::kData);

  EXPECT_EQ(policy.SelectQueue(1).value(), WindowTypeQueue::kRequest);
  EXPECT_EQ(policy.SelectQueue(1).value(), WindowTypeQueue::kData);

  policy.MarkQueueInactive(1, WindowTypeQueue::kRequest);
  EXPECT_EQ(policy.SelectQueue(1).value(), WindowTypeQueue::kData);

  policy.MarkQueueActive(1, WindowTypeQueue::kRequest);
  EXPECT_EQ(policy.SelectQueue(1).value(), WindowTypeQueue::kData);
  EXPECT_EQ(policy.SelectQueue(1).value(), WindowTypeQueue::kRequest);
}

}  // namespace
}  // namespace isekai
