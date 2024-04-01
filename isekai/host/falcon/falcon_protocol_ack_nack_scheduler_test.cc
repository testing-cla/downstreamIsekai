#include "isekai/host/falcon/falcon_protocol_ack_nack_scheduler.h"

#include <cstdint>
#include <memory>
#include <utility>

#include "absl/log/check.h"
#include "gtest/gtest.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/simple_environment.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_model.h"
#include "isekai/host/falcon/falcon_testing_helpers.h"
#include "isekai/host/rnic/connection_manager.h"

namespace isekai {
namespace {

class FakeTrafficShaper : public TrafficShaperInterface {
 public:
  void ConnectPacketBuilder(PacketBuilderInterface* packet_builder) override {}
  void TransferTxPacket(std::unique_ptr<Packet> packet) override {
    tx_packet_ = std::move(packet);
    tx_packet_count_++;
  }
  uint32_t get_tx_packet_count() const { return tx_packet_count_; }
  uint32_t get_tx_packet_psn() const { return tx_packet_->falcon.psn; }

 private:
  uint32_t tx_packet_count_ = 0;
  std::unique_ptr<Packet> tx_packet_;
};

TEST(ProtocolAckNackSchedulerTest, TestScheduler) {
  SimpleEnvironment env;
  FalconConfig config = DefaultConfigGenerator::DefaultFalconConfig(1);
  FakeTrafficShaper shaper;
  FalconModel falcon(config, &env, /*stats_collector=*/nullptr,
                     ConnectionManager::GetConnectionManager(), "falcon-host",
                     /* number of hosts */ 4);
  falcon.ConnectShaper(&shaper);
  // Make sure the connection for the ACK packet is properly initialized.
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(&falcon);
  FalconTestingHelpers::InitializeConnectionState(&falcon, connection_metadata);

  ProtocolAckNackScheduler scheduler(&falcon);

  for (int i = 0; i < 100; i++) {
    auto packet = std::make_unique<Packet>();
    // Set the ACK packet's scid.
    packet->metadata.scid = connection_metadata.scid;
    packet->falcon.psn = i;
    if (i % 3 == 0) {
      packet->packet_type = falcon::PacketType::kNack;
    } else {
      packet->packet_type = falcon::PacketType::kAck;
    }
    CHECK_OK(scheduler.EnqueuePacket(std::move(packet)));
  }

  int cnt = 0;
  while (scheduler.HasWork()) {
    scheduler.ScheduleWork();
    EXPECT_EQ(shaper.get_tx_packet_count(), cnt + 1);
    EXPECT_EQ(shaper.get_tx_packet_psn(), cnt);
    cnt++;
  }
}

}  // namespace
}  // namespace isekai
