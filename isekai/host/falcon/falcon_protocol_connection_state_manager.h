#ifndef ISEKAI_HOST_FALCON_FALCON_PROTOCOL_CONNECTION_STATE_MANAGER_H_
#define ISEKAI_HOST_FALCON_FALCON_PROTOCOL_CONNECTION_STATE_MANAGER_H_

#include <cstdint>
#include <functional>
#include <memory>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"

namespace isekai {

// Manages the state corresponding to FALCON connections
class ProtocolConnectionStateManager : public ConnectionStateManager {
 public:
  explicit ProtocolConnectionStateManager(FalconModelInterface* falcon);
  // Initializes the state corresponding to a new connection based on the ID and
  // metadata provided.
  absl::Status InitializeConnectionState(
      const ConnectionState::ConnectionMetadata& connection_metadata) override;
  // Returns the metadata associated with a given connection instantly. This API
  // is typically used by the various internal modules of FALCON.
  absl::StatusOr<ConnectionState*> PerformDirectLookup(
      uint32_t source_connection_id) const override;

 protected:
  FalconModelInterface* const falcon_;
  // Creates a connection state context for the connection defined by the
  // input metadata.
  virtual absl::Status CreateConnectionState(
      const ConnectionState::ConnectionMetadata& connection_metadata);
  virtual absl::Status InitializeTransactionLayerComponents(
      const ConnectionState::ConnectionMetadata& connection_metadata);
  // Per-connection state;
  absl::flat_hash_map<uint32_t, std::unique_ptr<ConnectionState>>
      connection_contexts_;

 private:
  // Initializes the per-connection states for the various packet delivery layer
  // components.
  absl::Status InitializePacketDeliveryLayerComponents(
      const ConnectionState::ConnectionMetadata& connection_metadata);
  // Initializes the per-connection states for the various transaction layer
  // components.
  // Handles the ACK coalescing engine related initialization for a connection.
  virtual void InitializeAckCoalescingEntryForConnection(
      const ConnectionState::ConnectionMetadata& connection_metadata);
};
};  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_PROTOCOL_CONNECTION_STATE_MANAGER_H_
