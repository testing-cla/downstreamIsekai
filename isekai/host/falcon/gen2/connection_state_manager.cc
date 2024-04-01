#include "isekai/host/falcon/gen2/connection_state_manager.h"

#include <cstdint>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_protocol_connection_state_manager.h"
#include "isekai/host/falcon/gen2/falcon_types.h"
#include "isekai/host/falcon/gen2/falcon_utils.h"

namespace isekai {

Gen2ConnectionStateManager::Gen2ConnectionStateManager(
    FalconModelInterface* falcon)
    : ProtocolConnectionStateManager(falcon) {}

// Creates an ACK coalescing entry for each flow in the connection.
void Gen2ConnectionStateManager::InitializeAckCoalescingEntryForConnection(
    const ConnectionState::ConnectionMetadata& connection_metadata) {
  uint32_t multipathing_num_flows =
      GetGen2MultipathingNumFlows(connection_metadata);
  for (uint8_t flow_id = 0; flow_id < multipathing_num_flows; flow_id++) {
    Gen2AckCoalescingKey ack_coalescing_key(connection_metadata.scid, flow_id);
    falcon_->get_ack_coalescing_engine()->CreateAckCoalescingEntry(
        &ack_coalescing_key, connection_metadata);
  }
}

absl::Status Gen2ConnectionStateManager::CreateConnectionState(
    const ConnectionState::ConnectionMetadata& connection_metadata) {
  RETURN_IF_ERROR(ProtocolConnectionStateManager::CreateConnectionState(
      connection_metadata));
  // In Gen2, we also need to resize the flow label and flow weight vectors to
  // the number of Gen2 flows for this connection.
  ConnectionState::CongestionControlMetadata& congestion_control_metadata =
      connection_contexts_[connection_metadata.scid]
          ->congestion_control_metadata;
  uint32_t multipathing_num_flows =
      GetGen2MultipathingNumFlows(connection_metadata);
  if (multipathing_num_flows != 1 && multipathing_num_flows != 4) {
    return absl::InvalidArgumentError(
        "A Gen2 connection can only be configured with 1 flow (single path) or "
        "4 flows (multipath).");
  }
  congestion_control_metadata.gen2_flow_labels.resize(multipathing_num_flows);
  congestion_control_metadata.gen2_flow_weights.resize(multipathing_num_flows);
  congestion_control_metadata.gen2_num_acked.resize(multipathing_num_flows);
  congestion_control_metadata.gen2_last_rue_event_time.resize(
      multipathing_num_flows);
  congestion_control_metadata.gen2_outstanding_rue_event.resize(
      multipathing_num_flows);
  return absl::OkStatus();
}

}  // namespace isekai
