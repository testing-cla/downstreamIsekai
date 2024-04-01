#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "internal/testing.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_model.h"
#include "isekai/host/falcon/falcon_testing_helpers.h"

namespace isekai {

namespace {

class ProtocolPacketReliabilityManagerTest
    : public FalconTestingHelpers::FalconTestSetup,
      public ::testing::Test {};

TEST_F(ProtocolPacketReliabilityManagerTest, TransactionSlidingWindowChecks) {
  FalconConfig config = DefaultConfigGenerator::DefaultFalconConfig(1);
  config.set_inter_host_rx_scheduling_tick_ns(0);
  InitFalcon(config);

  // Initialize the connection state.
  uint32_t source_connection_id = 1;
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get(),
                                                         source_connection_id);
  ConnectionState* connection_state =
      FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                      connection_metadata);

  // Create a fake incoming transaction with the required fields setup.
  auto rx_packet = std::make_unique<Packet>();
  rx_packet->packet_type = falcon::PacketType::kPullRequest;
  rx_packet->falcon.dest_cid = 1;
  rx_packet->falcon.rsn = 0;
  // Window range [0, 63].

  // Initialize packet metadata.
  auto packet_metadata = std::make_unique<PacketMetadata>();
  packet_metadata->type = falcon::PacketType::kPullRequest;
  packet_metadata->active_packet = std::make_unique<Packet>();
  // Initialize transaction metadata.
  auto transaction = std::make_unique<TransactionMetadata>(
      0, TransactionType::kPull, TransactionLocation::kTarget);
  transaction->packets[falcon::PacketType::kPullRequest] =
      std::move(packet_metadata);
  // Add the above metadata to the appropriate connection state.
  connection_state
      ->transactions[{transaction->rsn, TransactionLocation::kTarget}] =
      std::move(transaction);

  // Valid PSN (within window)
  rx_packet->falcon.psn = 63;
  EXPECT_OK(reliability_manager_->ReceivePacket(rx_packet.get()));
  // Check if the receive packet context with RSN=0 is created.
  EXPECT_OK(connection_state->rx_reliability_metadata.GetReceivedPacketContext(
      {0, TransactionLocation::kTarget}));
  // Invalid PSN (beyond window)
  rx_packet->falcon.psn = 64;
  EXPECT_EQ(reliability_manager_->ReceivePacket(rx_packet.get()).code(),
            absl::StatusCode::kOutOfRange);
}

TEST_F(ProtocolPacketReliabilityManagerTest,
       WrapAroundTransactionSlidingWindowChecks) {
  FalconConfig config = DefaultConfigGenerator::DefaultFalconConfig(1);
  config.set_inter_host_rx_scheduling_tick_ns(0);
  InitFalcon(config);

  // Initialize the connection state.
  uint32_t source_connection_id = 1;
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get(),
                                                         source_connection_id);
  ConnectionState* connection_state =
      FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                      connection_metadata);

  // Set the RBPSN > PSN (due to wrap  around)
  connection_state->rx_reliability_metadata.request_window_metadata
      .base_packet_sequence_number =
      4294967294;  // Makes window range [2^32 - 2, 61].

  // Create a fake incoming transaction with the required fields setup.
  auto rx_packet = std::make_unique<Packet>();
  rx_packet->packet_type = falcon::PacketType::kPullRequest;
  rx_packet->falcon.dest_cid = 1;
  rx_packet->falcon.rsn = 0;

  // Initialize packet metadata.
  auto packet_metadata = std::make_unique<PacketMetadata>();
  packet_metadata->type = falcon::PacketType::kPullRequest;
  packet_metadata->active_packet = std::make_unique<Packet>();
  // Initialize transaction metadata.
  auto transaction = std::make_unique<TransactionMetadata>(
      0, TransactionType::kPull, TransactionLocation::kTarget);
  transaction->packets[falcon::PacketType::kPullRequest] =
      std::move(packet_metadata);
  // Add the above metadata to the appropriate connection state.
  connection_state
      ->transactions[{transaction->rsn, TransactionLocation::kTarget}] =
      std::move(transaction);

  // Valid PSN (within window)
  rx_packet->falcon.psn = 61;
  EXPECT_OK(reliability_manager_->ReceivePacket(rx_packet.get()));

  // Invalid PSN (beyond window)
  rx_packet->falcon.psn = 62;
  EXPECT_EQ(reliability_manager_->ReceivePacket(rx_packet.get()).code(),
            absl::StatusCode::kOutOfRange);

  // Invalid PSN (before window)
  rx_packet->falcon.psn = 4294967292;
  EXPECT_EQ(reliability_manager_->ReceivePacket(rx_packet.get()).code(),
            absl::StatusCode::kAlreadyExists);
}

}  // namespace

}  // namespace isekai
