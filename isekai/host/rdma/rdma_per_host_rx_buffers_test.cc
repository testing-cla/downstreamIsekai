#include "isekai/host/rdma/rdma_per_host_rx_buffers.h"

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "internal/testing.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/simple_environment.h"
#include "isekai/host/rnic/rnic.h"

namespace isekai {

namespace {

using ::testing::_;
using ::testing::NiceMock;

class MockFalcon : public FalconInterface {
 public:
  MOCK_METHOD(int, GetVersion, (), (const override));
  MOCK_METHOD(void, InitiateTransaction, (std::unique_ptr<Packet> packet),
              (override));
  MOCK_METHOD(void, TransferRxPacket, (std::unique_ptr<Packet> packet),
              (override));
  MOCK_METHOD(void, AckTransaction,
              (uint32_t scid, uint32_t rsn, Packet::Syndrome ack_code,
               absl::Duration rnr_timeout,
               std::unique_ptr<OpaqueCookie> cookie),
              (override));
  MOCK_METHOD(uint32_t, SetupNewQp,
              (uint32_t scid, QpId qp_id, QpType type, OrderingMode mode),
              (override));
  MOCK_METHOD(absl::Status, EstablishConnection,
              (uint32_t scid, uint32_t dcid, uint8_t source_bifurcation_id,
               uint8_t destination_bifurcation_id,
               absl::string_view dst_ip_address, OrderingMode ordering_mode,
               const FalconConnectionOptions& connection_options),
              (override));
  MOCK_METHOD(const FalconConfig*, get_config, (), (const override));
  MOCK_METHOD(Environment*, get_environment, (), (const override));
  MOCK_METHOD(std::string_view, get_host_id, (), (const override));
  MOCK_METHOD(StatisticCollectionInterface*, get_stats_collector, (),
              (const override));
  MOCK_METHOD(FalconHistogramCollector*, get_histogram_collector, (),
              (const override));
  MOCK_METHOD(void, SetXoffByPacketBuilder, (bool xoff), (override));
  MOCK_METHOD(bool, CanSendPacket, (), (const override));
  MOCK_METHOD(void, SetXoffByRdma, (uint8_t bifurcation_id, bool xoff),
              (override));
  MOCK_METHOD(void, UpdateRxBytes,
              (std::unique_ptr<Packet> packet, uint32_t pkt_size_bytes),
              (override));
  MOCK_METHOD(void, UpdateTxBytes,
              (std::unique_ptr<Packet> packet, uint32_t pkt_size_bytes),
              (override));
};

TEST(RdmaRxBuffers, TestPushAndWakeup) {
  SimpleEnvironment env;

  RdmaRxBufferConfig rdma_rx_buffer_config;
  rdma_rx_buffer_config.add_buffer_size_bytes(1000);

  NiceMock<MockFalcon> falcon;

  isekai::RNicConfig rnic_config =
      isekai::DefaultConfigGenerator::DefaultRNicConfig();
  auto pcie_config = rnic_config.mutable_host_interface_config()->Mutable(0);
  pcie_config->mutable_write_queue_config()->set_bandwidth_bps(8000);

  auto hifs =
      std::make_unique<std::vector<std::unique_ptr<isekai::MemoryInterface>>>();
  for (const auto& host_intf_config : rnic_config.host_interface_config()) {
    auto hif =
        std::make_unique<isekai::MemoryInterface>(host_intf_config, &env);
    hifs->push_back(std::move(hif));
  }

  auto packet = std::make_unique<Packet>();
  packet->falcon.payload_length = 2028;  // RDMA Header +  payload
  RdmaPerHostRxBuffers rx_buffers(rdma_rx_buffer_config, hifs.get(), &falcon);

  rx_buffers.Push(
      0, /* PCIe completion callback */
      [&]() { EXPECT_EQ(env.ElapsedTime(), absl::Seconds(2)); },
      /* packet */ std::move(packet), /* RDMA to PCIe handover success */
      [&] { EXPECT_EQ(env.ElapsedTime(), absl::Seconds(0)); });

  packet = std::make_unique<Packet>();
  packet->falcon.payload_length = 2028;
  rx_buffers.Push(
      0, /* PCIe completion callback */
      [&] { EXPECT_EQ(env.ElapsedTime(), absl::Seconds(4)); },
      /* packet */ std::move(packet), /* RDMA to PCIe handover success */
      [&] { EXPECT_EQ(env.ElapsedTime(), absl::Seconds(0)); });

  EXPECT_CALL(falcon, SetXoffByRdma(_, _))
      .WillOnce([&](uint8_t bifurcation_id, bool xoff) {
        EXPECT_EQ(bifurcation_id, 0);
        EXPECT_EQ(xoff, true);
      });

  packet = std::make_unique<Packet>();
  packet->falcon.payload_length = 2028;
  rx_buffers.Push(
      0, /* PCIe completion callback */
      [&] { EXPECT_EQ(env.ElapsedTime(), absl::Seconds(6)); },
      /* packet */ std::move(packet), /* RDMA to PCIe handover success */
      [&] { EXPECT_EQ(env.ElapsedTime(), absl::Seconds(2)); });

  EXPECT_CALL(falcon, SetXoffByRdma(_, _))
      .WillOnce([&](uint8_t bifurcation_id, bool xoff) {
        EXPECT_EQ(bifurcation_id, 0);
        EXPECT_EQ(xoff, true);
      });

  packet = std::make_unique<Packet>();
  packet->falcon.payload_length = 2028;
  rx_buffers.Push(
      0, /* PCIe completion callback */
      [&] { EXPECT_EQ(env.ElapsedTime(), absl::Seconds(8)); },
      /* packet */ std::move(packet), /* RDMA to PCIe handover success */
      [&] { EXPECT_EQ(env.ElapsedTime(), absl::Seconds(4)); });

  EXPECT_CALL(falcon, SetXoffByRdma(_, _))
      .WillRepeatedly([&](uint8_t bifurcation_id, bool xoff) {
        EXPECT_EQ(bifurcation_id, 0);
        EXPECT_EQ(xoff, false);
      });
  env.Run();
}

}  // namespace
}  // namespace isekai
