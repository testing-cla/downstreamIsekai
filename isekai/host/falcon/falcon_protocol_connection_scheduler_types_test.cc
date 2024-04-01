#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"

#include "absl/container/btree_set.h"
#include "gtest/gtest.h"

namespace isekai {
namespace {

TEST(ProtocolConnectionSchedulerTypesTest, RetransmissionPriorityEnforcement) {
  RetransmissionWorkId w1(1, 1, falcon::PacketType::kPullRequest);
  RetransmissionWorkId w2(0, 2, falcon::PacketType::kPullRequest);
  absl::btree_set<RetransmissionWorkId> set;
  set.insert(w2);
  set.insert(w1);
  EXPECT_EQ(set.begin()->psn, 1);
  set.erase(set.begin());
  EXPECT_EQ(set.begin()->psn, 2);
}

}  // namespace
}  // namespace isekai
