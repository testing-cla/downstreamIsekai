#ifndef ISEKAI_HOST_FALCON_GEN2_CONNECTION_STATE_MANAGER_H_
#define ISEKAI_HOST_FALCON_GEN2_CONNECTION_STATE_MANAGER_H_

#include <cstdint>

#include "absl/status/status.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_protocol_connection_state_manager.h"

namespace isekai {

class Gen2ConnectionStateManager : public ProtocolConnectionStateManager {
 public:
  explicit Gen2ConnectionStateManager(FalconModelInterface* falcon);

 protected:
  // Initializes the CongestionControlMetadata struct of the ConnectionState.
  absl::Status CreateConnectionState(
      const ConnectionState::ConnectionMetadata& connection_metadata) override;

 private:
  // Handles the ACK coalescing engine related initialization for a connection.
  void InitializeAckCoalescingEntryForConnection(
      const ConnectionState::ConnectionMetadata& connection_metadata) override;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_CONNECTION_STATE_MANAGER_H_
