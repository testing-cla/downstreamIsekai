#include "isekai/host/falcon/gen3/connection_state_manager.h"

#include "isekai/common/status_util.h"
#include "isekai/host/falcon/gen3/falcon_utils.h"

namespace isekai {

Gen3ConnectionStateManager::Gen3ConnectionStateManager(
    FalconModelInterface* falcon)
    : Gen2ConnectionStateManager(falcon) {}

absl::Status Gen3ConnectionStateManager::InitializeConnectionState(
    const ConnectionState::ConnectionMetadata& connection_metadata) {
  return Gen2ConnectionStateManager::InitializeConnectionState(
      connection_metadata);
}

// Initializes the per-connection states for the various transaction layer
// components.
absl::Status Gen3ConnectionStateManager::InitializeTransactionLayerComponents(
    const ConnectionState::ConnectionMetadata& connection_metadata) {
  return Gen2ConnectionStateManager::InitializeTransactionLayerComponents(
      connection_metadata);
}

}  // namespace isekai
