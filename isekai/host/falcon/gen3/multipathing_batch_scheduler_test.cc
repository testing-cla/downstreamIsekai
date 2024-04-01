#include "isekai/host/falcon/gen3/multipathing_batch_scheduler.h"

#include <cstdint>
#include <memory>
#include <utility>

#include "absl/log/check.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/simple_environment.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_testing_helpers.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/gen3/falcon_model.h"
#include "isekai/host/falcon/gen3/falcon_utils.h"
#include "isekai/host/falcon/gen3/multipath_max_open_fcwnd_scheduling_policy.h"
#include "isekai/host/falcon/gen3/multipath_round_robin_scheduling_policy.h"
#include "isekai/host/falcon/rue/bits.h"
#include "isekai/host/falcon/rue/fixed.h"
#include "isekai/host/rnic/gen3/multipath_connection_manager.h"

namespace isekai {

constexpr uint32_t kCbid1 = 16; /* b010000 */
constexpr uint32_t kCbid2 = 32; /* b100000 */
constexpr int kDegreeOfMultipathing = 3;
constexpr falcon::PacketType kPacketType =
    falcon::PacketType::kPushUnsolicitedData;

// Tests the behaviors when connections are selected in round robin with batch
// size 1.
TEST(Gen3MultipathBatchSchedulerTest, RoundRobinSelectionWithBatchOne) {
  std::unique_ptr<MultipathSchedulingPolicy> policy =
      std::make_unique<Gen3MultipathRoundRobinPolicy>();
  Gen3MultipathBatchScheduler batch_scheduler(
      /*batch_size=*/1, std::move(policy));

  for (auto cid = 1; cid <= 3; ++cid) {
    CHECK_OK(batch_scheduler.InitConnection(cid, kCbid1));
  }

  // With max batch size 1, connections should be picked in round
  // robin order, regardless of whether the packet is sent successfully.
  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 1);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));
  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 2);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));
  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 3);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));
  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 1);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));
  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 2);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));
  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 3);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));
  for (auto cid = 1; cid <= 2; ++cid) {
    CHECK_OK(batch_scheduler.InitConnection(cid, kCbid2));
  }

  // With max batch size 1, connections should be picked in round
  // robin order, regardless of whether the packet is sent successfully.
  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid2).value(), 1);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid2, /*packet_sent*/ true));
  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid2).value(), 2);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid2, /*packet_sent*/ true));
  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid2).value(), 1);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid2, /*packet_sent*/ true));
  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid2).value(), 2);
  CHECK_OK(batch_scheduler.ReceiveSchedulingFeedback(kCbid2,
                                                     /*packet_sent*/ true));
}

// Tests the behaviors when connections are selected in round robin with batch
// size 2.
TEST(Gen3MultipathBatchSchedulerTest, RoundRobinSelectionWithBatchTwo) {
  std::unique_ptr<MultipathSchedulingPolicy> policy =
      std::make_unique<Gen3MultipathRoundRobinPolicy>();
  Gen3MultipathBatchScheduler batch_scheduler(
      /*batch_size=*/2, std::move(policy));

  for (auto cid = 1; cid <= 3; ++cid) {
    CHECK_OK(batch_scheduler.InitConnection(cid, kCbid1));
  }

  // With max batch size 2, connections should be picked in round robin order
  // every 2 packets, or will change the path immediately if a selected
  // connection is not sending packet successfully.
  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 1);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));

  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 1);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));

  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 2);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));

  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 2);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));

  // The path will be changed the next time after an unsuccessful send,
  // regardless of the batch size.
  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 3);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ false));

  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 1);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));
}

// Tests the behaviors when connections are selected in round robin with batch
// size 0.
TEST(Gen3MultipathBatchSchedulerTest, RoundRobinSelectionWithBatchZero) {
  std::unique_ptr<MultipathSchedulingPolicy> policy =
      std::make_unique<Gen3MultipathRoundRobinPolicy>();
  Gen3MultipathBatchScheduler batch_scheduler(
      /*batch_size=*/0, std::move(policy));

  for (auto cid = 1; cid <= 3; ++cid) {
    CHECK_OK(batch_scheduler.InitConnection(cid, kCbid1));
  }

  // With max batch size 0 (unlimited batch size), connections should be picked
  // in round robin order. The path will only be changed if a selected
  // connection is not sending packet successfully.
  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 1);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));

  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 1);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));

  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 1);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));

  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 1);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ false));

  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 2);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));

  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 2);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ false));

  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 3);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));

  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 3);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));

  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), 3);
  CHECK_OK(
      batch_scheduler.ReceiveSchedulingFeedback(kCbid1, /*packet_sent*/ true));
}

// We simulate the scheduled out of a packet by increasing the
// next_packet_sequence_number for correct open fcwnd calculation.
void ScheduleOutPackets(ConnectionState* connection_state) {
  connection_state->tx_reliability_metadata.data_window_metadata
      .next_packet_sequence_number++;
}

// Tests the behaviors when connections are selected in max-open-fcwnd-first
// with batch size 4.
TEST(Gen3MultipathMaxOpenFcwndPolicyBatchTest, MaxOpenFcwndWithBatchSizeFour) {
  SimpleEnvironment env;
  FalconConfig config = DefaultConfigGenerator::DefaultFalconConfig(3);
  config.set_gen3_multipathing_enabled(true);
  config.mutable_connection_scheduler_policies()
      ->set_multipath_scheduling_policy(FalconConfig::MAX_OPEN_FCWND_FIRST);
  config.mutable_connection_scheduler_policies()
      ->set_multipath_scheduling_batch_size(4);
  ConnectionState::ConnectionMetadata connection_bundle_metadata;

  Gen3FalconModel falcon(config, &env, /*stats_collector=*/nullptr,
                         MultipathConnectionManager::GetConnectionManager(),
                         "falcon-host",
                         /* number of hosts */ 4);
  ConnectionStateManager* connection_manager = falcon.get_state_manager();

  connection_bundle_metadata.scid = kCbid1;
  connection_bundle_metadata.dcid = kCbid2;
  connection_bundle_metadata.degree_of_multipathing = kDegreeOfMultipathing;
  connection_bundle_metadata.connection_type = ConnectionState::
      ConnectionMetadata::ConnectionStateType::Gen3ExploratoryMultipath;
  EXPECT_OK(connection_manager->InitializeConnectionState(
      connection_bundle_metadata));

  std::unique_ptr<MultipathSchedulingPolicy> policy =
      std::make_unique<Gen3MultipathMaxOpenFcwndPolicy>(&falcon);
  Gen3MultipathBatchScheduler batch_scheduler(
      /*batch_size=*/4, std::move(policy));

  // Enqueues a push packet into the scheduler to make a default
  // 'current_candidate_packet_type' in the scheduler. This ensures that we can
  // test the behavior of max open fcwnd scheduling policy without encountering
  // errors, as the policy relies on querying the packet type. Note that this
  // test case specifically targets the max open fcwnd scheduling policy, while
  // scheduling and batching components will be tested separately in the
  // relevant component's test cases.
  FalconTestingHelpers::SetupTransactionAndEnqueuePacketToScheduler(
      /*connection_state=*/kCbid1,
      /*transaction_type=*/TransactionType::kPushUnsolicited,
      /*transaction_location=*/TransactionLocation::kInitiator,
      /*transaction_state=*/TransactionState::kPushUnsolicitedReqUlpRx,
      /*packet_metadata_type=*/kPacketType,
      /*rsn=*/0,
      /*psn=*/0, falcon);

  for (auto index = 0; index < 3; ++index) {
    uint32_t cid = MakeCidFromCbidAndIndex(kCbid1, index);
    CHECK_OK(batch_scheduler.InitConnection(cid, kCbid1));
  }

  uint32_t cid0 = MakeCidFromCbidAndIndex(kCbid1, 0);
  uint32_t cid1 = MakeCidFromCbidAndIndex(kCbid1, 1);
  uint32_t cid2 = MakeCidFromCbidAndIndex(kCbid1, 2);

  ASSERT_OK_AND_ASSIGN(auto connection_state0,
                       connection_manager->PerformDirectLookup(cid0));
  ASSERT_OK_AND_ASSIGN(auto connection_state1,
                       connection_manager->PerformDirectLookup(cid1));
  ASSERT_OK_AND_ASSIGN(auto connection_state2,
                       connection_manager->PerformDirectLookup(cid2));

  // Sets the fabric congestion windows of the connection states.
  connection_state0->congestion_control_metadata.fabric_congestion_window =
      falcon_rue::FloatToFixed<double, uint32_t>(6.0,
                                                 falcon_rue::kFractionalBits);
  connection_state1->congestion_control_metadata.fabric_congestion_window =
      falcon_rue::FloatToFixed<double, uint32_t>(5.0,
                                                 falcon_rue::kFractionalBits);
  connection_state2->congestion_control_metadata.fabric_congestion_window =
      falcon_rue::FloatToFixed<double, uint32_t>(50.0,
                                                 falcon_rue::kFractionalBits);

  // The actual test starts from here, we schedule 60 packets with the max
  // open fcwnd scheduling policy via the batch scheduler. We vary the
  // connections' open fcwnd during the test.

  // cid 0's open fcwnd = 6
  // cid 1's open fcwnd = 5
  // cid 2's open fcwnd = 50 (selected)

  // The first 40 packets will go to cid 2.
  for (int i = 0; i < 40; ++i) {
    EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), cid2);
    CHECK_OK(batch_scheduler.ReceiveSchedulingFeedback(kCbid1,
                                                       /*packet_sent=*/true));
    ScheduleOutPackets(connection_state2);
  }

  // cid 0's open fcwnd = 6
  // cid 1's open fcwnd = 5
  // cid 2's open fcwnd = 10 (selected)

  // After that, the 41~44th packets will still go to cid2 because its open
  // fcwnd is 10.
  for (int i = 40; i < 44; ++i) {
    EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), cid2);
    CHECK_OK(batch_scheduler.ReceiveSchedulingFeedback(kCbid1,
                                                       /*packet_sent=*/true));
    ScheduleOutPackets(connection_state2);
  }

  // cid 0's open fcwnd = 6
  // cid 1's open fcwnd = 5
  // cid 2's open fcwnd = 6 (selected)

  for (int i = 44; i < 48; ++i) {
    EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), cid2);
    CHECK_OK(batch_scheduler.ReceiveSchedulingFeedback(kCbid1,
                                                       /*packet_sent=*/true));
    ScheduleOutPackets(connection_state2);
  }

  // cid 0's open fcwnd = 6 (selected)
  // cid 1's open fcwnd = 5
  // cid 2's open fcwnd = 2

  for (int i = 48; i < 52; ++i) {
    EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), cid0);
    CHECK_OK(batch_scheduler.ReceiveSchedulingFeedback(kCbid1,
                                                       /*packet_sent=*/true));
    ScheduleOutPackets(connection_state0);
  }

  // cid 0's open fcwnd = 2
  // cid 1's open fcwnd = 5 (selected)
  // cid 2's open fcwnd = 2

  for (int i = 52; i < 56; ++i) {
    EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), cid1);
    CHECK_OK(batch_scheduler.ReceiveSchedulingFeedback(kCbid1,
                                                       /*packet_sent=*/true));
    ScheduleOutPackets(connection_state1);
  }

  // cid 0's open fcwnd = 2 (selected)
  // cid 1's open fcwnd = 1
  // cid 2's open fcwnd = 2

  for (int i = 56; i < 58; ++i) {
    EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), cid0);
    CHECK_OK(batch_scheduler.ReceiveSchedulingFeedback(kCbid1,
                                                       /*packet_sent=*/true));
    ScheduleOutPackets(connection_state0);
  }

  // Gives negative response to the policy if there are no available fcwnd left.
  // The policy will then be enforced to change to the next connection.
  CHECK_OK(batch_scheduler.ReceiveSchedulingFeedback(kCbid1,
                                                     /*packet_sent=*/false));

  // cid 0's open fcwnd = 0
  // cid 1's open fcwnd = 1
  // cid 2's open fcwnd = 2 (selected)
  for (int i = 58; i < 60; ++i) {
    EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), cid2);
    CHECK_OK(batch_scheduler.ReceiveSchedulingFeedback(kCbid1,
                                                       /*packet_sent=*/true));
    ScheduleOutPackets(connection_state2);
  }

  // Gives negative response to the policy if there are no available fcwnd left.
  // The policy will then be enforced to change to the next connection.
  CHECK_OK(batch_scheduler.ReceiveSchedulingFeedback(kCbid1,
                                                     /*packet_sent=*/false));
  // cid 0's open fcwnd = 0
  // cid 1's open fcwnd = 1 (selected)
  // cid 2's open fcwnd = 0

  EXPECT_EQ(batch_scheduler.SelectConnection(kCbid1).value(), cid1);
  CHECK_OK(batch_scheduler.ReceiveSchedulingFeedback(kCbid1,
                                                     /*packet_sent=*/true));
  ScheduleOutPackets(connection_state1);

  // cid 0's open fcwnd = 0
  // cid 1's open fcwnd = 0
  // cid 2's open fcwnd = 0
}

}  // namespace isekai
