#include "isekai/host/falcon/falcon_connection_state.h"

#include <cstdint>
#include <memory>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/simple_environment.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_model.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/gen3/falcon_utils.h"
#include "isekai/host/rnic/connection_manager.h"

namespace isekai {
namespace {

// Helper methods for creating Read/Write requests.
std::unique_ptr<Packet> CreateWriteRequestInitiator() {
  auto packet = std::make_unique<Packet>();
  packet->rdma.opcode = Packet::Rdma::Opcode::kWriteOnly;
  return packet;
}

// Helper methods for creating Read/Write requests.
std::unique_ptr<Packet> CreateWriteRequestTarget(uint32_t rsn) {
  auto packet = std::make_unique<Packet>();
  packet->falcon.rsn = rsn;
  packet->packet_type = falcon::PacketType::kPushUnsolicitedData;
  return packet;
}

TEST(ConnectionStateTest, AccessTransactionsInAConnectionState) {
  ConnectionState::ConnectionMetadata connection_metadata;
  ConnectionState connection_state(connection_metadata);

  auto transaction = std::make_unique<TransactionMetadata>();
  connection_state.transactions[{1, TransactionLocation::kInitiator}] =
      std::move(transaction);

  EXPECT_TRUE(
      connection_state.GetTransaction({1, TransactionLocation::kInitiator})
          .ok());
  EXPECT_FALSE(
      connection_state.GetTransaction({2, TransactionLocation::kInitiator})
          .ok());
}

}  // namespace
}  // namespace isekai
