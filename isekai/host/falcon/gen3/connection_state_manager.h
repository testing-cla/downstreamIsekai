#ifndef ISEKAI_HOST_FALCON_GEN3_CONNECTION_STATE_MANAGER_H_
#define ISEKAI_HOST_FALCON_GEN3_CONNECTION_STATE_MANAGER_H_

#include "isekai/host/falcon/gen2/connection_state_manager.h"

namespace isekai {

class Gen3ConnectionStateManager : public Gen2ConnectionStateManager {
 public:
  explicit Gen3ConnectionStateManager(FalconModelInterface* falcon);
  absl::Status InitializeConnectionState(
      const ConnectionState::ConnectionMetadata& connection_metadata) override;
  // Initializes the per-connection states for the various transaction layer
  // components.
  absl::Status InitializeTransactionLayerComponents(
      const ConnectionState::ConnectionMetadata& connection_metadata) override;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_CONNECTION_STATE_MANAGER_H_
