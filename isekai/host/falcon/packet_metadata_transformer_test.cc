#include <cstdint>
#include <memory>
#include <tuple>
#include <vector>

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
#include "isekai/host/falcon/falcon_types.h"

namespace isekai {

namespace {

using ::testing::_;

constexpr uint32_t kFalconVersion1 = 1;

class Gen1PacketMetadataTransformerTest
    : public FalconTestingHelpers::FalconTestSetup,
      public ::testing::Test {};

TEST_F(Gen1PacketMetadataTransformerTest, CheckRoutingListInsertedInAPacket) {
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(kFalconVersion1);
  InitFalcon(config);

  // Uses fake traffic shaper instead of the default mock traffic shaper.
  FalconTestingHelpers::FakeTrafficShaper fake_shaper;
  falcon_->ConnectShaper(&fake_shaper);

  // Setups dummy packet with necessary information.
  Packet* packet;
  ConnectionState* connection_state;
  std::tie(packet, connection_state) = FalconTestingHelpers::SetupTransaction(
      falcon_.get(), TransactionType::kPushUnsolicited,
      TransactionLocation::kInitiator,
      TransactionState::kPushUnsolicitedReqUlpRx,
      falcon::PacketType::kPushUnsolicitedData, /*scid=*/1, /*rsn=*/0);

  // Sets static routing port list in a connection state.
  std::vector<uint32_t> expected_ports_list = {7, 6, 4, 3, 1};
  connection_state->connection_metadata.static_routing_port_lists = {
      expected_ports_list};

  // Transmits the packet.
  EXPECT_OK(reliability_manager_->TransmitPacket(
      /*scid=*/1, /*rsn*/ 0, falcon::PacketType::kPushUnsolicitedData));

  // Checks if the packet has the static route list information.
  env_.RunFor(absl::Microseconds(1));
  EXPECT_EQ(fake_shaper.packet_list.size(), 1);
  EXPECT_EQ(fake_shaper.packet_list[0].metadata.static_route.current_port_index,
            0);
  EXPECT_EQ(fake_shaper.packet_list[0].metadata.static_route.port_list,
            expected_ports_list);
}

TEST_F(Gen1PacketMetadataTransformerTest,
       CheckRoutingListInsertedInANackPacket) {
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(kFalconVersion1);
  // Sets the parametrized test values in FalconConfig and initialize Falcon
  // test setup.
  InitFalcon(config);

  // Initializes the connection state with the static routing list.
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get());
  std::vector<uint32_t> expected_ports_list = {7, 6, 4, 3, 1};
  connection_metadata.static_routing_port_lists = {expected_ports_list};
  FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                  connection_metadata);

  // Transmits a Nack packet.
  std::unique_ptr<AckCoalescingKey> ack_coalescing_key =
      CreateAckCoalescingKey(connection_metadata.scid);
  ASSERT_OK(ack_coalescing_engine_->TransmitNACK(
      *ack_coalescing_key, 1, true, falcon::NackCode::kRxWindowError, nullptr));

  // Checks if the packet has the static route list information.
  EXPECT_CALL(shaper_, TransferTxPacket(_))
      .WillOnce([&](std::unique_ptr<Packet> p) {
        EXPECT_EQ(p->metadata.static_route.current_port_index, 0);
        EXPECT_EQ(p->metadata.static_route.port_list, expected_ports_list);
      });
  env_.Run();
}

TEST_F(Gen1PacketMetadataTransformerTest,
       CheckRoutingListInsertedInAnAckPacket) {
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(kFalconVersion1);
  // Sets the parametrized test values in FalconConfig and initialize Falcon
  // test setup.
  InitFalcon(config);

  // Initializes the connection state with the static routing list.
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get());
  std::vector<uint32_t> expected_ports_list = {7, 6, 4, 3, 1};
  connection_metadata.static_routing_port_lists = {expected_ports_list};
  FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                  connection_metadata);

  // Transmits an Ack packet.
  std::unique_ptr<AckCoalescingKey> ack_coalescing_key =
      CreateAckCoalescingKey(connection_metadata.scid);
  ack_coalescing_engine_->TransmitACK(*ack_coalescing_key, false, false);

  // Checks if the packet has the static route list information.
  EXPECT_CALL(shaper_, TransferTxPacket(_))
      .WillOnce([&](std::unique_ptr<Packet> p) {
        EXPECT_EQ(p->metadata.static_route.current_port_index, 0);
        EXPECT_EQ(p->metadata.static_route.port_list, expected_ports_list);
      });
  env_.Run();
}

}  // namespace

}  // namespace isekai
