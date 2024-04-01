#include "isekai/host/falcon/gen2/connection_state_manager.h"

#include <cstdint>
#include <memory>

#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "internal/testing.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/simple_environment.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_testing_helpers.h"
#include "isekai/host/falcon/gen2/falcon_model.h"
#include "isekai/host/rnic/connection_manager.h"

namespace isekai {

namespace {

constexpr uint32_t kCid1 = 123;
constexpr int kFalconVersion = 2;

// Tests that the connection state manager only accepts 1 or 4 flows as degree
// of multipathing for a connection.
TEST(Gen2ConnectionStateManager, ConnectionStateDegreeOfMultipathing) {
  // Setup FalconModel.
  SimpleEnvironment env;
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(kFalconVersion);
  auto falcon = std::make_unique<Gen2FalconModel>(
      config, &env, /*stats_collector=*/nullptr,
      ConnectionManager::GetConnectionManager(), "falcon-host",
      /* number of hosts */ 4);

  // Initialize a connection with a valid degree_of_multipathing (1 or 4).
  ConnectionState::ConnectionMetadata metadata_allowed_1 =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon.get(), kCid1);
  metadata_allowed_1.degree_of_multipathing = 1;
  EXPECT_OK(falcon->get_state_manager()->InitializeConnectionState(
      metadata_allowed_1));
  ConnectionState::ConnectionMetadata metadata_allowed_4 =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon.get(),
                                                         kCid1 + 1);
  metadata_allowed_4.degree_of_multipathing = 4;
  EXPECT_OK(falcon->get_state_manager()->InitializeConnectionState(
      metadata_allowed_4));

  // Initialize a connection with an invalid degree_of_multipathing (2).
  ConnectionState::ConnectionMetadata metadata_disallowed =
      FalconTestingHelpers::InitializeConnectionMetadata(falcon.get(),
                                                         kCid1 + 2);
  metadata_disallowed.degree_of_multipathing = 2;
  EXPECT_EQ(falcon->get_state_manager()
                ->InitializeConnectionState(metadata_disallowed)
                .code(),
            absl::StatusCode::kInvalidArgument);
}

}  // namespace

}  // namespace isekai
