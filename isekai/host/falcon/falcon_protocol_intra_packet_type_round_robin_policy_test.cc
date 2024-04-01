#include "isekai/host/falcon/falcon_protocol_intra_packet_type_round_robin_policy.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace isekai {
namespace {

TEST(ProtocolIntraPacketTypeRoundRobinPolicy, RoundRobinSelectionOrder) {
  IntraPacketTypeRoundRobinPolicy policy;
  for (auto cid = 1; cid <= 3; ++cid) {
    policy.InitConnection(cid);
    policy.MarkConnectionActive(cid, PacketTypeQueue::kPushData);
  }

  // Connections should be picked in round robin order.
  EXPECT_EQ(policy.SelectConnection(PacketTypeQueue::kPushData).value(), 1);
  EXPECT_EQ(policy.SelectConnection(PacketTypeQueue::kPushData).value(), 2);
  EXPECT_EQ(policy.SelectConnection(PacketTypeQueue::kPushData).value(), 3);

  // Mark connection 1 inactive.
  policy.MarkConnectionInactive(1, PacketTypeQueue::kPushData);

  // Connections 2 and 3 should be in round robin order picked.
  for (auto round = 1; round <= 2; ++round) {
    EXPECT_EQ(policy.SelectConnection(PacketTypeQueue::kPushData).value(), 2);
    EXPECT_EQ(policy.SelectConnection(PacketTypeQueue::kPushData).value(), 3);
  }
}

}  // namespace
}  // namespace isekai
