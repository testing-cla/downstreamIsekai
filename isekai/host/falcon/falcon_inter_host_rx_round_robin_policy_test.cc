#include "isekai/host/falcon/falcon_inter_host_rx_round_robin_policy.h"

#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "internal/testing.h"
#include "isekai/common/status_util.h"

namespace isekai {

namespace {

// Helper function to Xoff/Xon the policy.
void SetHostXoff(InterHostRxSchedulingRoundRobinPolicy& policy, uint8_t host_id,
                 bool xoff) {
  if (xoff) {
    policy.XoffHost(host_id);
  } else {
    policy.XonHost(host_id);
  }
}

// Helper function to mark the host active or inactive in the policy.
void SetHostActive(InterHostRxSchedulingRoundRobinPolicy& policy,
                   uint8_t host_id, bool active) {
  if (active) {
    policy.MarkHostActive(host_id);
  } else {
    policy.MarkHostInactive(host_id);
  }
}

// Trying these configurations for Single host:
// +───────+─────────+
// | Xoff  | Active  |
// +───────+─────────+
// | True  | True    |
// | True  | False   |
// | False | True    |
// | False | False   |
// +───────+─────────+
TEST(FalconInterHostRxRoundRobinPolicy,
     TestSingleHostAllXoffAndActiveCombination) {
  absl::StatusOr<uint8_t> selected_host;
  InterHostRxSchedulingRoundRobinPolicy policy;

  // Expect no work as no queue is active in the beginning.
  EXPECT_EQ(policy.SelectHost().status().code(),
            absl::StatusCode::kUnavailable);
  EXPECT_EQ(policy.HasWork(), false);

  // Add a new host.
  EXPECT_OK(policy.InitHost(0));
  // Initially host is in inactive state.
  EXPECT_EQ(policy.HasWork(), false);
  // Expect no work as no outstanding data is there to send even though the
  // host. is in Xon state.
  EXPECT_EQ(policy.SelectHost().status().code(),
            absl::StatusCode::kUnavailable);
  // Case 1.
  policy.MarkHostActive(0);
  policy.XoffHost(0);
  // Expect no work as host 0 is Xoffed.
  EXPECT_EQ(policy.SelectHost().status().code(),
            absl::StatusCode::kUnavailable);
  // Case 2.
  policy.MarkHostInactive(0);
  policy.XoffHost(0);
  // Expect no work as host 0 is Xoffed and inactive.
  EXPECT_EQ(policy.SelectHost().status().code(),
            absl::StatusCode::kUnavailable);
  // Case 3.
  policy.MarkHostActive(0);
  policy.XonHost(0);
  ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
  EXPECT_EQ(selected_host.value(), 0);
  // Case 4.
  policy.MarkHostInactive(0);
  policy.XonHost(0);
  // Expect no work as host 0 in inactive.
  EXPECT_EQ(policy.SelectHost().status().code(),
            absl::StatusCode::kUnavailable);
}

// Two hosts configured with these values:
// +────+──────────+────────────+──────────+────────────+────────+
// |    | Xoff (0) | Active (0) | Xoff (1) | Active (1) | Result |
// +────+──────────+────────────+──────────+────────────+────────+
// | 0  | True     | True       | True     | True       | None   |
// | 1  | True     | True       | True     | False      | None   |
// | 2  | True     | True       | False    | True       | 1      |
// | 3  | True     | True       | False    | False      | None   |
// | 4  | True     | False      | True     | True       | None   |
// | 5  | True     | False      | True     | False      | None   |
// | 6  | True     | False      | False    | True       | 1      |
// | 7  | True     | False      | False    | False      | None   |
// | 8  | False    | True       | True     | True       | 0      |
// | 9  | False    | True       | True     | False      | 0      |
// | 10 | False    | True       | False    | True       | 0, 1   |
// | 11 | False    | True       | False    | False      | 0      |
// | 12 | False    | False      | True     | True       | None   |
// | 13 | False    | False      | True     | False      | None   |
// | 14 | False    | False      | False    | True       | 1      |
// | 15 | False    | False      | False    | False      | None   |
// +────+──────────+────────────+──────────+────────────+────────+
TEST(FalconInterHostRxRoundRobinPolicy, TestTwoHostWithActiveAndXoff) {
  std::vector<bool> possible_values = {true, false};

  InterHostRxSchedulingRoundRobinPolicy policy;

  // Expect no work as no queue is active in the beginning.
  EXPECT_EQ(policy.SelectHost().status().code(),
            absl::StatusCode::kUnavailable);
  EXPECT_EQ(policy.HasWork(), false);

  // Add a new host.
  EXPECT_OK(policy.InitHost(0));
  // Initially host is in inactive state.
  EXPECT_EQ(policy.HasWork(), false);
  // Add a new host.
  EXPECT_OK(policy.InitHost(1));
  // Initially host is in inactive state.
  EXPECT_EQ(policy.HasWork(), false);

  uint32_t iteration = 0;
  for (auto h1_xoff : possible_values) {
    for (auto h1_active : possible_values) {
      for (auto h2_xoff : possible_values) {
        for (auto h2_active : possible_values) {
          SetHostXoff(policy, 0, h1_xoff);
          SetHostActive(policy, 0, h1_active);
          SetHostXoff(policy, 1, h2_xoff);
          SetHostActive(policy, 1, h2_active);

          switch (iteration) {
            case 2:
            case 6:
            case 14: {
              // Round 1.
              absl::StatusOr<uint8_t> selected_host;
              ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
              EXPECT_EQ(selected_host.value(), 1);
              // Round 2.
              ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
              EXPECT_EQ(selected_host.value(), 1);
            } break;
            case 8:
            case 9:
            case 11: {
              absl::StatusOr<uint8_t> selected_host;
              // Round 1.
              ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
              EXPECT_EQ(selected_host.value(), 0);
              // Round 2.
              ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
              EXPECT_EQ(selected_host.value(), 0);
            } break;
            case 10: {
              absl::StatusOr<uint8_t> selected_host;
              // Round 1.
              ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
              EXPECT_EQ(selected_host.value(), 0);
              // Round 2.
              ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
              EXPECT_EQ(selected_host.value(), 1);
              // Round 3.
              ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
              EXPECT_EQ(selected_host.value(), 0);
              // Round 4.
              ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
              EXPECT_EQ(selected_host.value(), 1);
            } break;
            default: {
              EXPECT_EQ(policy.HasWork(), false);
            } break;
          }
          iteration++;
        }
      }
    }
  }
}

TEST(FalconInterHostRxRoundRobinPolicy, TestMultipleHostWithActiveAndXoff) {
  std::list<uint8_t> host_order = {1, 3, 2, 0};

  InterHostRxSchedulingRoundRobinPolicy policy;

  // Expect no work as no queue is active in the beginning.
  EXPECT_EQ(policy.SelectHost().status().code(),
            absl::StatusCode::kUnavailable);
  EXPECT_EQ(policy.HasWork(), false);

  for (auto host_id : host_order) {
    EXPECT_OK(policy.InitHost(host_id));
  }

  for (auto host_id : host_order) {
    policy.MarkHostActive(host_id);
  }
  // Current seqeuence 1 3 2 0.

  absl::StatusOr<uint8_t> selected_host;
  // Round 1.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }

  // Check two times, we should get same sequence.
  // Round 2.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }

  // Mark first two host (i.e., 1, 3) inactive
  for (int i = 0; i < 2; i++) {
    policy.MarkHostInactive(host_order.front());
    host_order.pop_front();
  }
  // Current sequence: 2, 0

  // Round 3 and 4.
  for (int i = 0; i < 2; i++) {
    for (auto host_id : host_order) {
      ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
      EXPECT_EQ(selected_host.value(), host_id);
    }
  }

  // Round 4.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }

  policy.MarkHostActive(1);
  host_order.push_back(1);
  policy.MarkHostActive(3);
  host_order.push_back(3);
  // Current sequence 2 0 1 3.

  // Round 5.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }

  // Xoff 0.
  auto it = std::find(host_order.begin(), host_order.end(), 0);
  EXPECT_NE(it, host_order.end());
  policy.XoffHost(0);
  host_order.erase(it);
  // Current sequence 2 1 3.

  // Round 6.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }

  // Xoff 1.
  it = std::find(host_order.begin(), host_order.end(), 1);
  EXPECT_NE(it, host_order.end());
  policy.XoffHost(1);
  host_order.erase(it);
  // Current sequence 2 3.

  // Round 7.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }

  // Xon 1.
  policy.XonHost(1);
  host_order.push_back(1);
  // Current sequence 2 3 1.

  // Round 8.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }

  // Round 9.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }

  policy.MarkHostActive(1);
  // Round 10.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }

  policy.MarkHostActive(0);
  // Round 11.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }

  // Xon 1.
  policy.XonHost(0);
  host_order.push_back(0);
  // Current sequence 2 3 1 0.

  // Round 12.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }

  // Deactivate 3.
  it = std::find(host_order.begin(), host_order.end(), 3);
  EXPECT_NE(it, host_order.end());
  policy.MarkHostInactive(3);
  host_order.erase(it);
  // Current sequence 2 1 0.
  // Round 13.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }

  policy.XonHost(3);
  // Round 14.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }

  policy.XoffHost(3);
  policy.MarkHostActive(3);
  // Round 14.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }
  policy.XonHost(3);
  host_order.push_back(3);
  // Round 15.
  for (auto host_id : host_order) {
    ASSERT_OK_THEN_ASSIGN(selected_host, policy.SelectHost());
    EXPECT_EQ(selected_host.value(), host_id);
  }
}

}  // namespace
}  // namespace isekai
