#include "isekai/host/falcon/falcon_protocol_inter_connection_round_robin_policy.h"

#include <cstdint>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "internal/testing.h"

namespace isekai {
namespace {

TEST(ProtocolInterConnectionRoundRobinPolicyTest, RoundRobinWorkFound) {
  InterConnectionRoundRobinPolicy policy;
  // Inserts two connections that are available for scheduling.
  EXPECT_OK(policy.InitConnection(1));
  EXPECT_OK(policy.InitConnection(2));
  policy.MarkConnectionActive(1);
  policy.MarkConnectionActive(2);
  // For the first time, we expect connection 1 to be picked.
  EXPECT_EQ(policy.SelectConnection().value(), 1);
  // For the second time, we expect connection 2 to be picked.
  EXPECT_EQ(policy.SelectConnection().value(), 2);
  // We update the policy to record that connection 2 is not inactive.
  policy.MarkConnectionInactive(2);
  // For the next time, we expect connection 1 to be picked.
  EXPECT_EQ(policy.SelectConnection().value(), 1);
}

TEST(ProtocolInterConnectionRoundRobinPolicyTest, RoundRobinNoWorkFound) {
  InterConnectionRoundRobinPolicy policy;
  // Inserts two connections that are available for scheduling.
  EXPECT_OK(policy.InitConnection(1));
  EXPECT_OK(policy.InitConnection(2));
  EXPECT_EQ(policy.SelectConnection().status().code(),
            absl::StatusCode::kUnavailable);
  policy.MarkConnectionActive(1);
  policy.MarkConnectionActive(2);
  // For the first time, we expect connection 1 to be picked.
  EXPECT_EQ(policy.SelectConnection().value(), 1);
  // For the second time, we expect connection 2 to be picked.
  EXPECT_EQ(policy.SelectConnection().value(), 2);
  policy.MarkConnectionInactive(1);
  policy.MarkConnectionInactive(2);
  EXPECT_EQ(policy.SelectConnection().status().code(),
            absl::StatusCode::kUnavailable);
}

TEST(ProtocolInterConnectionRoundRobinPolicyTest, RoundRobinOrder) {
  std::vector<uint64_t> test_order;
  InterConnectionRoundRobinPolicy policy;
  EXPECT_OK(policy.InitConnection(1));
  EXPECT_OK(policy.InitConnection(2));
  EXPECT_OK(policy.InitConnection(3));

  policy.MarkConnectionActive(1);
  EXPECT_EQ(policy.SelectConnection().value(), 1);
  EXPECT_EQ(policy.SelectConnection().value(), 1);
  policy.MarkConnectionActive(2);
  EXPECT_EQ(
      policy.SelectConnection().value(),
      1);  // 1 again, since it was already at the head of the active queue.
  policy.MarkConnectionActive(3);
  EXPECT_EQ(policy.SelectConnection().value(), 2);
  policy.MarkConnectionInactive(1);
  EXPECT_EQ(policy.SelectConnection().value(), 3);
  EXPECT_EQ(policy.SelectConnection().value(), 2);
  EXPECT_EQ(policy.SelectConnection().value(), 3);
  policy.MarkConnectionInactive(3);
  EXPECT_EQ(policy.SelectConnection().value(), 2);
}

}  // namespace
}  // namespace isekai
