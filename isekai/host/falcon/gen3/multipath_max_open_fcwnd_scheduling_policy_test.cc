#include "isekai/host/falcon/gen3/multipath_max_open_fcwnd_scheduling_policy.h"

#include <cstdint>

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
#include "isekai/host/falcon/rue/bits.h"
#include "isekai/host/falcon/rue/fixed.h"
#include "isekai/host/rnic/gen3/multipath_connection_manager.h"

namespace isekai {

constexpr int kDegreeOfMultipathing = 5;
constexpr int kSrcBid = 16 /* b010000 */;
constexpr int kDstBid = 32 /* b100000 */;
constexpr falcon::PacketType kPacketType =
    falcon::PacketType::kPushUnsolicitedData;

// We simulate the scheduled out of a packet by increasing the
// next_packet_sequence_number for correct open fcwnd calculation.
void ScheduleOutPackets(ConnectionState* connection_state) {
  connection_state->tx_reliability_metadata.data_window_metadata
      .next_packet_sequence_number++;
}

TEST(Gen3MultipathMaxOpenFcwndPolicyTest, MaxOpenFcwndMultipathSelection) {
  SimpleEnvironment env;
  FalconConfig config = DefaultConfigGenerator::DefaultFalconConfig(3);
  config.set_gen3_multipathing_enabled(true);
  config.mutable_connection_scheduler_policies()
      ->set_multipath_scheduling_policy(FalconConfig::MAX_OPEN_FCWND_FIRST);
  config.mutable_connection_scheduler_policies()
      ->set_multipath_scheduling_batch_size(1);
  ConnectionState::ConnectionMetadata connection_bundle_metadata;
  Gen3FalconModel falcon(config, &env, /*stats_collector=*/nullptr,
                         MultipathConnectionManager::GetConnectionManager(),
                         "falcon-host",
                         /* number of hosts */ 4);
  ConnectionStateManager* connection_manager = falcon.get_state_manager();

  connection_bundle_metadata.scid = kSrcBid;
  connection_bundle_metadata.dcid = kDstBid;
  connection_bundle_metadata.degree_of_multipathing = kDegreeOfMultipathing;
  connection_bundle_metadata.connection_type = ConnectionState::
      ConnectionMetadata::ConnectionStateType::Gen3ExploratoryMultipath;
  EXPECT_OK(connection_manager->InitializeConnectionState(
      connection_bundle_metadata));

  // Enqueues a push packet into the scheduler to make a default
  // 'current_candidate_packet_type' in the scheduler. This ensures that we can
  // test the behavior of max open fcwnd scheduling policy without encountering
  // errors, as the policy relies on querying the packet type. Note that this
  // test case specifically targets the max open fcwnd scheduling policy, while
  // scheduling and batching components will be tested separately in the
  // relevant component's test cases.
  FalconTestingHelpers::SetupTransactionAndEnqueuePacketToScheduler(
      /*connection_state=*/kSrcBid,
      /*transaction_type=*/TransactionType::kPushUnsolicited,
      /*transaction_location=*/TransactionLocation::kInitiator,
      /*transaction_state=*/TransactionState::kPushUnsolicitedReqUlpRx,
      /*packet_metadata_type=*/kPacketType,
      /*rsn=*/0,
      /*psn=*/0, falcon);

  // The code above serves as the preparation code. The actual test starts from
  // here, involving the testing of the scheduling of 45 packets with the max
  // open fcwnd scheduling policy. We vary the connections' open fcwnd during
  // the test.
  Gen3MultipathMaxOpenFcwndPolicy policy(&falcon);

  for (auto index = 0; index < 5; ++index) {
    uint32_t cid = MakeCidFromCbidAndIndex(kSrcBid, index);
    CHECK_OK(policy.InitConnection(cid, kSrcBid));
  }

  uint32_t cid0 = MakeCidFromCbidAndIndex(kSrcBid, 0);
  uint32_t cid1 = MakeCidFromCbidAndIndex(kSrcBid, 1);
  uint32_t cid2 = MakeCidFromCbidAndIndex(kSrcBid, 2);
  uint32_t cid3 = MakeCidFromCbidAndIndex(kSrcBid, 3);
  uint32_t cid4 = MakeCidFromCbidAndIndex(kSrcBid, 4);

  ASSERT_OK_AND_ASSIGN(auto connection_state0,
                       connection_manager->PerformDirectLookup(cid0));
  ASSERT_OK_AND_ASSIGN(auto connection_state1,
                       connection_manager->PerformDirectLookup(cid1));
  ASSERT_OK_AND_ASSIGN(auto connection_state2,
                       connection_manager->PerformDirectLookup(cid2));
  ASSERT_OK_AND_ASSIGN(auto connection_state3,
                       connection_manager->PerformDirectLookup(cid3));
  ASSERT_OK_AND_ASSIGN(auto connection_state4,
                       connection_manager->PerformDirectLookup(cid4));

  // Sets the fabric congestion windows of the connection states.
  connection_state0->congestion_control_metadata.fabric_congestion_window =
      falcon_rue::FloatToFixed<double, uint32_t>(5.0,
                                                 falcon_rue::kFractionalBits);
  connection_state1->congestion_control_metadata.fabric_congestion_window =
      falcon_rue::FloatToFixed<double, uint32_t>(5.0,
                                                 falcon_rue::kFractionalBits);
  connection_state2->congestion_control_metadata.fabric_congestion_window =
      falcon_rue::FloatToFixed<double, uint32_t>(10.0,
                                                 falcon_rue::kFractionalBits);
  connection_state3->congestion_control_metadata.fabric_congestion_window =
      falcon_rue::FloatToFixed<double, uint32_t>(50.0,
                                                 falcon_rue::kFractionalBits);
  connection_state4->congestion_control_metadata.fabric_congestion_window =
      falcon_rue::FloatToFixed<double, uint32_t>(0.0,
                                                 falcon_rue::kFractionalBits);

  // The first 40 packets will go to cid3.
  for (int i = 0; i < 40; ++i) {
    EXPECT_EQ(policy.SelectConnection(kSrcBid).value(), cid3);
    ScheduleOutPackets(connection_state3);
  }

  // After that, the 41th packets will still go to cid3 because its open fcwnd
  // is 10.
  EXPECT_EQ(policy.SelectConnection(kSrcBid).value(), cid3);
  ScheduleOutPackets(connection_state3);

  // The 42th and 43th packets will go to cid2 because it has the largest open
  // fcwnd (value=10 and value=9).
  EXPECT_EQ(policy.SelectConnection(kSrcBid).value(), cid2);
  ScheduleOutPackets(connection_state2);
  EXPECT_EQ(policy.SelectConnection(kSrcBid).value(), cid2);
  ScheduleOutPackets(connection_state2);

  // The 44th packet will go to cid3 because cid3 has the largest open fcwnd
  // again (value=9).
  EXPECT_EQ(policy.SelectConnection(kSrcBid).value(), cid3);
  ScheduleOutPackets(connection_state3);
}

}  // namespace isekai
