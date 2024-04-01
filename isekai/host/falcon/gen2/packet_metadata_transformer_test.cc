#include <cstdint>
#include <memory>
#include <vector>

#include "absl/log/check.h"
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
#include "isekai/host/falcon/falcon_testing_helpers.h"
#include "isekai/host/falcon/falcon_types.h"

namespace isekai {

namespace {

using ::testing::_;
using ::testing::InSequence;

constexpr uint32_t kFalconVersion2 = 2;
constexpr uint8_t kNumFlowsPerMultipathConnection = 4;

class Gen2PacketMetadataTransformerTest
    : public FalconTestingHelpers::FalconTestSetup,
      public ::testing::Test {};

TEST_F(Gen2PacketMetadataTransformerTest, CheckRoutingListInsertedInPackets) {
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(kFalconVersion2);
  InitFalcon(config);

  // Sets the expected static port lists.
  std::vector<std::vector<uint32_t>> expected_ports_lists = {
      {7, 6, 4, 3, 1}, {1, 7, 5, 34, 16}, {2, 2, 2, 2, 2}, {4, 4, 4, 4, 4}};

  // Initialize the connection state.
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get());
  connection_metadata.degree_of_multipathing = kNumFlowsPerMultipathConnection;
  connection_metadata.static_routing_port_lists = expected_ports_lists;
  ConnectionState* connection_state =
      FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                      connection_metadata);

  // Checks if the packets contain the correct static port list.
  {
    InSequence seq;
    for (int i = 0; i < kNumFlowsPerMultipathConnection; ++i) {
      EXPECT_CALL(shaper_, TransferTxPacket(_))
          .WillOnce([expected_ports_lists, i](std::unique_ptr<Packet> p) {
            EXPECT_EQ(p->metadata.static_route.current_port_index, 0);
            EXPECT_EQ(p->metadata.static_route.port_list,
                      expected_ports_lists[i]);
          });
    }
  }

  // Transmits one pull request for each path in Gen2.
  for (int i = 0; i < kNumFlowsPerMultipathConnection; ++i) {
    // Initializes a pull request transaction.
    auto packet1 = FalconTestingHelpers::SetupTransactionWithConnectionState(
        /*connection_state=*/connection_state,
        /*transaction_type=*/TransactionType::kPull,
        /*transaction_location=*/TransactionLocation::kInitiator,
        /*transaction_state=*/TransactionState::kPullReqUlpRx,
        /*packet_metadata_type=*/falcon::PacketType::kPullRequest,
        /*rsn=*/i,
        /*psn=*/1,
        /*request_length=*/1,
        /*rdma_opcode=*/Packet::Rdma::Opcode::kReadRequest);
    packet1->metadata.scid = connection_metadata.scid;

    // Transmits the pull request packet.
    EXPECT_OK(reliability_manager_->TransmitPacket(
        /*scid=*/connection_metadata.scid, /*rsn*/ i,
        falcon::PacketType::kPullRequest));
  }

  env_.RunFor(absl::Microseconds(10));
}

TEST_F(Gen2PacketMetadataTransformerTest,
       CheckRoutingListInsertedINackPackets) {
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(kFalconVersion2);
  // Sets the parametrized test values in FalconConfig and initialize Falcon
  // test setup.
  InitFalcon(config);

  // Sets the expected static port lists.
  std::vector<std::vector<uint32_t>> expected_ports_lists = {
      {7, 6, 4, 3, 1}, {1, 7, 5, 34, 16}, {2, 2, 2, 2, 2}, {4, 4, 4, 4, 4}};

  // Initialize the connection state.
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get());
  connection_metadata.degree_of_multipathing = kNumFlowsPerMultipathConnection;
  connection_metadata.static_routing_port_lists = expected_ports_lists;
  FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                  connection_metadata);

  // Checks if the packets contain the correct static port list.
  {
    InSequence seq;
    for (int i = 0; i < kNumFlowsPerMultipathConnection; ++i) {
      EXPECT_CALL(shaper_, TransferTxPacket(_))
          .WillOnce([expected_ports_lists, i](std::unique_ptr<Packet> p) {
            EXPECT_EQ(p->metadata.static_route.current_port_index, 0);
            EXPECT_EQ(p->metadata.static_route.port_list,
                      expected_ports_lists[i]);
          });
    }
  }

  // Transmits one Nack for each path in Gen2.
  for (int i = 0; i < kNumFlowsPerMultipathConnection; ++i) {
    std::unique_ptr<AckCoalescingKey> ack_coalescing_key =
        CreateAckCoalescingKey(connection_metadata.scid, i);

    // Creates a dumb packet with flow label to set the flow label of the
    // AckCoalescingEntry corresponding to the AckCoalescingKey.
    Packet packet_with_flow_label;
    packet_with_flow_label.metadata.flow_label = i;
    CHECK_OK(ack_coalescing_engine_->UpdateCongestionControlMetadataToReflect(
        *ack_coalescing_key, &packet_with_flow_label));

    ASSERT_OK(ack_coalescing_engine_->TransmitNACK(
        *ack_coalescing_key, 0, true, falcon::NackCode::kRxWindowError,
        nullptr));
  }

  env_.Run();
}

TEST_F(Gen2PacketMetadataTransformerTest,
       CheckRoutingListInsertedInAckPackets) {
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(kFalconVersion2);

  // Sets the parametrized test values in FalconConfig and initialize Falcon
  // test setup.
  InitFalcon(config);

  // Sets the expected static port lists.
  std::vector<std::vector<uint32_t>> expected_ports_lists = {
      {7, 6, 4, 3, 1}, {1, 7, 5, 34, 16}, {2, 2, 2, 2, 2}, {4, 4, 4, 4, 4}};

  // Initializes the connection state.
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get());
  connection_metadata.degree_of_multipathing = kNumFlowsPerMultipathConnection;
  connection_metadata.static_routing_port_lists = expected_ports_lists;
  FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                  connection_metadata);

  // Checks if the packets contain the correct static port list.
  {
    InSequence seq;
    for (int i = 0; i < kNumFlowsPerMultipathConnection; ++i) {
      EXPECT_CALL(shaper_, TransferTxPacket(_))
          .WillOnce([expected_ports_lists, i](std::unique_ptr<Packet> p) {
            EXPECT_EQ(p->metadata.static_route.current_port_index, 0);
            EXPECT_EQ(p->metadata.static_route.port_list,
                      expected_ports_lists[i]);
          });
    }
  }

  // Transmits one ACK for each path in Gen2.
  for (int i = 0; i < kNumFlowsPerMultipathConnection; ++i) {
    std::unique_ptr<AckCoalescingKey> ack_coalescing_key =
        CreateAckCoalescingKey(connection_metadata.scid, i);

    // Creates a dumb packet with flow label to set the flow label of the
    // AckCoalescingEntry corresponding to the AckCoalescingKey.
    Packet packet_with_flow_label;
    packet_with_flow_label.metadata.flow_label = i;
    CHECK_OK(ack_coalescing_engine_->UpdateCongestionControlMetadataToReflect(
        *ack_coalescing_key, &packet_with_flow_label));

    ack_coalescing_engine_->TransmitACK(*ack_coalescing_key, false, false);
  }

  env_.Run();
}

}  // namespace

}  // namespace isekai
