#include "isekai/host/falcon/falcon_protocol_packet_type_based_connection_scheduler.h"

#include <cstdint>
#include <memory>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "internal/testing.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/simple_environment.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_model.h"
#include "isekai/host/falcon/falcon_testing_helpers.h"
#include "isekai/host/falcon/falcon_utils.h"
#include "isekai/host/rnic/connection_manager.h"

namespace isekai {
namespace {

// This defines all the objects needed for setup and testing. The test fixture
// is parameterized on the Falcon version.
class FalconTransactionTypeBasedConnectionSchedulerTest
    : public FalconTestingHelpers::FalconTestSetup,
      public ::testing::TestWithParam<int /* version */> {
 protected:
  int GetFalconVersion() { return GetParam(); }
};

INSTANTIATE_TEST_SUITE_P(
    TransactionTypeBasedConnectionSchedulerTest,
    FalconTransactionTypeBasedConnectionSchedulerTest,
    /*version=*/testing::Values(1, 2, 3),
    [](const testing::TestParamInfo<
        FalconTransactionTypeBasedConnectionSchedulerTest::ParamType>& info) {
      const int version = static_cast<int>(info.param);
      return absl::StrCat("Gen", version);
    });

using ::testing::InSequence;
using ::testing::NiceMock;

constexpr int kRequestLength = 32;

uint32_t next_psn = 0;
std::unique_ptr<TransactionMetadata> CreateTransaction(falcon::PacketType itype,
                                                       TransactionType ctype,
                                                       uint32_t rsn,
                                                       uint32_t scid) {
  auto packet_metadata = std::make_unique<PacketMetadata>();
  packet_metadata->psn = next_psn++;
  packet_metadata->type = itype;
  packet_metadata->direction = PacketDirection::kOutgoing;
  packet_metadata->active_packet = std::make_unique<Packet>();
  packet_metadata->active_packet->rdma.request_length = kRequestLength;
  packet_metadata->active_packet->metadata.scid = scid;

  auto transaction = std::make_unique<TransactionMetadata>();
  transaction->rsn = rsn;
  transaction->type = ctype;
  transaction->request_length = 50;
  transaction->location = TransactionLocation::kInitiator;
  if (ctype == TransactionType::kPull) {
    transaction->state = TransactionState::kPullReqUlpRx;
  } else if (ctype == TransactionType::kPushUnsolicited) {
    transaction->state = TransactionState::kPushUnsolicitedReqUlpRx;
  }
  transaction->packets[packet_metadata->type] = std::move(packet_metadata);
  if (ctype == TransactionType::kPushUnsolicited) {
    auto phantom_request = std::make_unique<PacketMetadata>();
    phantom_request->type = falcon::PacketType::kInvalid;
    transaction->packets[phantom_request->type] = std::move(phantom_request);
  }

  return transaction;
}

void CreateAndAddTransaction(
    ConnectionStateManager* const connection_state_manager_, uint32_t id,
    falcon::PacketType itype, TransactionType ctype, uint32_t rsn) {
  auto transaction = CreateTransaction(itype, ctype, rsn, id);

  // Adds the above metadata to connection state.
  absl::StatusOr<ConnectionState*> connection_state =
      connection_state_manager_->PerformDirectLookup(id);
  EXPECT_TRUE(connection_state.ok());
  connection_state.value()
      ->transactions[{rsn, TransactionLocation::kInitiator}] =
      std::move(transaction);

  // Sets the retransmission timer such that no retransmissions occur.
  const absl::Duration kRetransmissionTimer = absl::Microseconds(20);
  connection_state.value()->congestion_control_metadata.retransmit_timeout =
      kRetransmissionTimer;
}

TEST_P(FalconTransactionTypeBasedConnectionSchedulerTest, WorkScheduler) {
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(GetFalconVersion());
  config.mutable_connection_scheduler_policies()
      ->set_inter_packet_type_scheduling_policy(
          FalconConfig::WEIGHTED_ROUND_ROBIN);
  config.mutable_connection_scheduler_policies()
      ->set_intra_packet_type_scheduling_policy(FalconConfig::ROUND_ROBIN);

  InitFalcon(config);
  // Initializes the connection state.

  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get(),
                                                         /*scid=*/1);
  FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                  connection_metadata);

  // Initializes metadata corresponding to the transaction.
  auto transaction = CreateTransaction(falcon::PacketType::kPullRequest,
                                       TransactionType::kPull,
                                       /*rsn=*/1, /*scid=*/1);

  // Adds the above transaction to the appropriate connection state.
  absl::StatusOr<ConnectionState*> connection_state =
      connection_state_manager_->PerformDirectLookup(/*scid*/ 1);
  EXPECT_TRUE(connection_state.ok());
  connection_state.value()
      ->transactions[{transaction->rsn, TransactionLocation::kInitiator}] =
      std::move(transaction);

  // Sets the retransmission timer such that no retransmissions occur.
  const absl::Duration kRetransmissionTimer = absl::Microseconds(20);
  connection_state.value()->congestion_control_metadata.retransmit_timeout =
      kRetransmissionTimer;

  EXPECT_TRUE(connection_scheduler_
                  ->EnqueuePacket(
                      /*scid=*/1, /*rsn=*/1, falcon::PacketType::kPullRequest)
                  .ok());

  EXPECT_TRUE(
      env_.ScheduleEvent(absl::Nanoseconds(11), [&]() { env_.Stop(); }).ok());
  env_.Run();

  // Checks if the transaction was actually scheduled.
  EXPECT_TRUE(connection_state.value()
                  ->GetTransaction({1, TransactionLocation::kInitiator})
                  .value()
                  ->GetPacketMetadata(falcon::PacketType::kPullRequest)
                  .value()
                  ->schedule_status.ok());
}

TEST_P(FalconTransactionTypeBasedConnectionSchedulerTest,
       TestMultipleConnections) {
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(GetFalconVersion());
  config.set_falcon_tick_time_ns(10);
  config.mutable_connection_scheduler_policies()
      ->set_inter_packet_type_scheduling_policy(
          FalconConfig::WEIGHTED_ROUND_ROBIN);
  config.mutable_connection_scheduler_policies()
      ->set_intra_packet_type_scheduling_policy(FalconConfig::ROUND_ROBIN);

  InitFalcon(config);

  // Initializes connection state for multiple SCIDs. We'll keep SCID = 3 empty.
  ConnectionState::ConnectionMetadata connection_metadata1 =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get(),
                                                         /*scid=*/1);
  FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                  connection_metadata1);
  ConnectionState::ConnectionMetadata connection_metadata2 =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get(),
                                                         /*scid=*/2);
  FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                  connection_metadata2);
  ConnectionState::ConnectionMetadata connection_metadata3 =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get(),
                                                         /*scid=*/3);
  FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                  connection_metadata3);

  // Creates a pull request with scid = 1
  CreateAndAddTransaction(connection_state_manager_, /*id=*/1,
                          falcon::PacketType::kPullRequest,
                          TransactionType::kPull,
                          /*rsn=*/1);
  // Creates another pull request with scid = 1
  CreateAndAddTransaction(connection_state_manager_, /*id=*/1,
                          falcon::PacketType::kPullRequest,
                          TransactionType::kPull,
                          /*rsn=*/2);

  // Creates a pull request with scid = 2
  CreateAndAddTransaction(connection_state_manager_, /*id=*/2,
                          falcon::PacketType::kPullRequest,
                          TransactionType::kPull,
                          /*rsn=*/1);
  // Creates a push request with scid = 2
  CreateAndAddTransaction(connection_state_manager_, /*id=*/2,
                          falcon::PacketType::kPushUnsolicitedData,
                          TransactionType::kPushUnsolicited,
                          /*rsn=*/2);

  // Enqueues all the transactions together.
  EXPECT_TRUE(
      env_.ScheduleEvent(absl::Nanoseconds(1),
                         [&]() {
                           EXPECT_TRUE(connection_scheduler_
                                           ->EnqueuePacket(
                                               /*scid=*/1, /*rsn=*/1,
                                               falcon::PacketType::kPullRequest)
                                           .ok());
                           EXPECT_TRUE(connection_scheduler_
                                           ->EnqueuePacket(
                                               /*scid=*/1, /*rsn=*/2,
                                               falcon::PacketType::kPullRequest)
                                           .ok());
                           EXPECT_TRUE(connection_scheduler_
                                           ->EnqueuePacket(
                                               /*scid=*/2, /*rsn=*/1,
                                               falcon::PacketType::kPullRequest)
                                           .ok());
                           EXPECT_TRUE(
                               connection_scheduler_
                                   ->EnqueuePacket(
                                       /*scid=*/2, /*rsn=*/2,
                                       falcon::PacketType::kPushUnsolicitedData)
                                   .ok());
                         })
          .ok());

  {
    InSequence seq;
    EXPECT_CALL(shaper_, TransferTxPacket(testing::_))
        .WillOnce([&](std::unique_ptr<Packet> p) {
          EXPECT_EQ(p->packet_type, falcon::PacketType::kPullRequest);
        });
    EXPECT_CALL(shaper_, TransferTxPacket(testing::_))
        .WillOnce([&](std::unique_ptr<Packet> p) {
          EXPECT_EQ(p->packet_type, falcon::PacketType::kPullRequest);
        });
    EXPECT_CALL(shaper_, TransferTxPacket(testing::_))
        .WillOnce([&](std::unique_ptr<Packet> p) {
          EXPECT_EQ(p->packet_type, falcon::PacketType::kPullRequest);
        });
    EXPECT_CALL(shaper_, TransferTxPacket(testing::_))
        .WillOnce([&](std::unique_ptr<Packet> p) {
          EXPECT_EQ(p->packet_type, falcon::PacketType::kPushUnsolicitedData);
        });
  }

  EXPECT_TRUE(
      env_.ScheduleEvent(absl::Nanoseconds(60), [&]() { env_.Stop(); }).ok());
  env_.Run();
}

TEST_P(FalconTransactionTypeBasedConnectionSchedulerTest,
       PhantomRequesZeroCost) {
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(GetFalconVersion());
  constexpr int kFalconCycleTime = 30;
  config.set_falcon_tick_time_ns(kFalconCycleTime);

  InitFalcon(config);
  // Initialize the connection state.

  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get(),
                                                         /*scid=*/1);
  FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                  connection_metadata);

  absl::StatusOr<ConnectionState*> connection_state_or =
      connection_state_manager_->PerformDirectLookup(/*scid*/ 1);
  EXPECT_OK(connection_state_or);
  auto connection_state = connection_state_or.value();

  // Set the retransmission timer such that no retransmissions occur.
  const absl::Duration kRetransmissionTimer = absl::Microseconds(20);
  connection_state->congestion_control_metadata.retransmit_timeout =
      kRetransmissionTimer;

  // Initialize metadata corresponding to the transaction.
  auto pull_transaction1 = CreateTransaction(falcon::PacketType::kPullRequest,
                                             TransactionType::kPull,
                                             /*rsn=*/1, /*scid=*/1);
  auto push_transaction2 =
      CreateTransaction(falcon::PacketType::kPushUnsolicitedData,
                        TransactionType::kPushUnsolicited,
                        /*rsn=*/2, /*scid=*/1);
  auto pull_transaction3 = CreateTransaction(falcon::PacketType::kPullRequest,
                                             TransactionType::kPull,
                                             /*rsn=*/3, /*scid=*/1);

  // Add the above transaction to the appropriate connection state.
  connection_state->transactions[{pull_transaction1->rsn,
                                  TransactionLocation::kInitiator}] =
      std::move(pull_transaction1);
  connection_state->transactions[{push_transaction2->rsn,
                                  TransactionLocation::kInitiator}] =
      std::move(push_transaction2);
  connection_state->transactions[{pull_transaction3->rsn,
                                  TransactionLocation::kInitiator}] =
      std::move(pull_transaction3);

  EXPECT_OK(connection_scheduler_->EnqueuePacket(
      /*scid=*/1, /*rsn=*/1, falcon::PacketType::kPullRequest));
  EXPECT_OK(connection_scheduler_->EnqueuePacket(
      /*scid=*/1, /*rsn=*/2, falcon::PacketType::kPushUnsolicitedData));
  EXPECT_OK(connection_scheduler_->EnqueuePacket(
      /*scid=*/1, /*rsn=*/3, falcon::PacketType::kPullRequest));

  // Given that the Phantom request does not consume any cycles, then we can
  // expect three packets to be scheduled in three cycles.
  env_.RunUntil(absl::Nanoseconds(kFalconCycleTime * 3));
  auto counters = stats_manager_->GetConnectionCounters(/*cid=*/1);
  EXPECT_EQ(2, counters.initiator_tx_pull_request);
  EXPECT_EQ(1, counters.initiator_tx_push_unsolicited_data);
}

TEST_P(FalconTransactionTypeBasedConnectionSchedulerTest,
       TestOpRateWorkScheduler) {
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(GetFalconVersion());
  constexpr int kFalconCycleTime = 5;
  config.set_falcon_tick_time_ns(kFalconCycleTime);
  config.mutable_rue()->set_initial_fcwnd(128);
  config.mutable_rue()->set_initial_ncwnd(128);

  InitFalcon(config);
  // Initialize the connection state.

  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get(),
                                                         /*scid=*/1);
  FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                  connection_metadata);

  constexpr int kManyOps = 128;
  constexpr int kExpectedOps = 40;
  for (int i = 0; i < kManyOps; ++i) {
    // Initialize metadata corresponding to the transaction.
    auto transaction = CreateTransaction(falcon::PacketType::kPullRequest,
                                         TransactionType::kPull,
                                         /*rsn=*/i, /*scid=*/1);

    // Add the above transaction to the appropriate connection state.
    absl::StatusOr<ConnectionState*> connection_state =
        connection_state_manager_->PerformDirectLookup(/*scid*/ 1);
    EXPECT_OK(connection_state);
    connection_state.value()
        ->transactions[{transaction->rsn, TransactionLocation::kInitiator}] =
        std::move(transaction);

    // Set the retransmission timer such that no retransmissions occur.
    const absl::Duration kRetransmissionTimer = absl::Microseconds(20);
    connection_state.value()->congestion_control_metadata.retransmit_timeout =
        kRetransmissionTimer;

    EXPECT_OK(connection_scheduler_->EnqueuePacket(
        /*scid=*/1, /*rsn=*/i, falcon::PacketType::kPullRequest));
  }

  constexpr int kOneMicrosecondInNanoseconds = 1e3 - 1;
  env_.RunUntil(absl::Nanoseconds(kOneMicrosecondInNanoseconds));
  auto counters = stats_manager_->GetConnectionCounters(/*cid=*/1);
  // In one microsecond, ONLY 40 ops will be sent.
  EXPECT_EQ(kExpectedOps, counters.initiator_tx_pull_request);

  env_.RunUntil(absl::Nanoseconds(4 * kOneMicrosecondInNanoseconds));
  // After a few microsecond, the rest of 128 ops will be sent.
  counters = stats_manager_->GetConnectionCounters(/*cid=*/1);
  EXPECT_EQ(kManyOps, counters.initiator_tx_pull_request);
}

}  // namespace
}  // namespace isekai
