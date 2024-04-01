#include "isekai/host/falcon/gen3/multipath_round_robin_scheduling_policy.h"

#include "absl/log/check.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace isekai {
namespace {

constexpr uint32_t kCbid1 = 16; /* b010000 */
constexpr uint32_t kCbid2 = 32; /* b100000 */

TEST(Gen3MultipathRoundRobinPolicyTest, SelectWithoutInit) {
  Gen3MultipathRoundRobinPolicy policy;
  ASSERT_DEATH(CHECK_OK(policy.SelectConnection(kCbid1)), "");
}

TEST(Gen3MultipathRoundRobinPolicyTest, InitDuplicatedConnections) {
  Gen3MultipathRoundRobinPolicy policy;
  EXPECT_EQ(policy.InitConnection(1, kCbid1).code(), absl::StatusCode::kOk);
  ASSERT_DEATH(CHECK_OK(policy.InitConnection(1, kCbid1)), "");
}

TEST(Gen3MultipathRoundRobinPolicyTest, RoundRobinSelection) {
  Gen3MultipathRoundRobinPolicy policy;

  for (auto cid = 1; cid <= 3; ++cid) {
    CHECK_OK(policy.InitConnection(cid, kCbid1));
  }

  EXPECT_EQ(policy.SelectConnection(kCbid1).value(), 1);
  EXPECT_EQ(policy.SelectConnection(kCbid1).value(), 2);
  EXPECT_EQ(policy.SelectConnection(kCbid1).value(), 3);
  EXPECT_EQ(policy.SelectConnection(kCbid1).value(), 1);
  EXPECT_EQ(policy.SelectConnection(kCbid1).value(), 2);
  EXPECT_EQ(policy.SelectConnection(kCbid1).value(), 3);

  for (auto cid = 1; cid <= 2; ++cid) {
    CHECK_OK(policy.InitConnection(cid, kCbid2));
  }

  EXPECT_EQ(policy.SelectConnection(kCbid2).value(), 1);
  EXPECT_EQ(policy.SelectConnection(kCbid2).value(), 2);
  EXPECT_EQ(policy.SelectConnection(kCbid2).value(), 1);
  EXPECT_EQ(policy.SelectConnection(kCbid2).value(), 2);
}

}  // namespace
}  // namespace isekai
