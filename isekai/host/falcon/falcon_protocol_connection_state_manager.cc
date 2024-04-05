#include "isekai/host/falcon/falcon_protocol_connection_state_manager.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/functional/bind_front.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/falcon_utils.h"

namespace isekai {

// Manages the state corresponding to the FALCON connections and does not model
// any hardware aspects of a given NIC; instead focuses on the protocol.
ProtocolConnectionStateManager::ProtocolConnectionStateManager(
    FalconModelInterface* falcon)
    : falcon_(falcon) {}

// Initializes the connection state for the given connection ID.
absl::Status ProtocolConnectionStateManager::InitializeConnectionState(
    const ConnectionState::ConnectionMetadata& connection_metadata) {
  // Creates a connection state context for the connection defined by the
  // input metadata.
  RETURN_IF_ERROR(CreateConnectionState(connection_metadata));

  RETURN_IF_ERROR(InitializePacketDeliveryLayerComponents(connection_metadata));
  RETURN_IF_ERROR(InitializeTransactionLayerComponents(connection_metadata));
  return absl::OkStatus();
}

absl::Status ProtocolConnectionStateManager::CreateConnectionState(
    const ConnectionState::ConnectionMetadata& connection_metadata) {
  if (connection_contexts_.find(connection_metadata.scid) !=
      connection_contexts_.end()) {
    return absl::AlreadyExistsError("Duplicate source connection ID.");
  }
  connection_contexts_[connection_metadata.scid] =
      std::make_unique<ConnectionState>(connection_metadata,
                                        falcon_->GetVersion());
  return absl::OkStatus();
}

// Returns a pointer to the connection state of the given connection ID without
// any delay and without triggering any simulator event.
absl::StatusOr<ConnectionState*>
ProtocolConnectionStateManager::PerformDirectLookup(
    uint32_t source_connection_id) const {
  auto it = connection_contexts_.find(source_connection_id);
  if (it != connection_contexts_.end()) {
    return it->second.get();
  }
  return absl::NotFoundError("Connection state not found.");
}

void ProtocolConnectionStateManager::InitializeAckCoalescingEntryForConnection(
    const ConnectionState::ConnectionMetadata& connection_metadata) {
  AckCoalescingKey ack_coalescing_key(connection_metadata.scid);
  falcon_->get_ack_coalescing_engine()->CreateAckCoalescingEntry(
      &ack_coalescing_key, connection_metadata);
}

// Initializes the per-connection states for the various packet delivery layer
// components.
absl::Status
ProtocolConnectionStateManager::InitializePacketDeliveryLayerComponents(
    const ConnectionState::ConnectionMetadata& connection_metadata) {
  // Passes congestion control metadata to the RUE for initialization.
  falcon_->get_rate_update_engine()->InitializeMetadata(
      connection_contexts_[connection_metadata.scid]
          ->congestion_control_metadata);
  // Passes the connection ID to schedulers for initialization.
  CHECK_OK(
      falcon_->get_retransmission_scheduler()->InitConnectionSchedulerQueues(
          connection_metadata.scid));
  CHECK_OK(falcon_->get_connection_scheduler()->InitConnectionSchedulerQueues(
      connection_metadata.scid));
  // Creates the coalescing entries for this connection.
  InitializeAckCoalescingEntryForConnection(connection_metadata);
  // Initializes per-connection fields (if any) in the packet reliability
  // manager.
  falcon_->get_packet_reliability_manager()->InitializeConnection(
      connection_metadata.scid);
  return absl::OkStatus();
}

// Initializes the per-connection states for the various transaction layer
// components.
absl::Status
ProtocolConnectionStateManager::InitializeTransactionLayerComponents(
    const ConnectionState::ConnectionMetadata& connection_metadata) {
  // Initialize the connection resource profile.

  falcon_->get_resource_manager()->InitializeResourceProfile(
      connection_contexts_[connection_metadata.scid]->resource_profile, 0);
  // Passes the connection ID and default sequence numbers to the buffer reorder
  // engine for initialization.
  CHECK_OK(falcon_->get_buffer_reorder_engine()->InitializeConnection(
      connection_metadata.scid, 0, 0, 0, 0, connection_metadata.ordered_mode));

  // scheduler is not yet supported for Gen2 and Gen3.
  if (falcon_->get_config()->scheduler_variant() ==
          FalconConfig::EVENT_BASED_SCHEDULER &&
      falcon_->get_config()->version() == 1) {
    auto fast_connection_scheduler =
        dynamic_cast<FastScheduler*>(falcon_->get_connection_scheduler());
    auto fast_retransmission_scheduler =
        dynamic_cast<FastScheduler*>(falcon_->get_retransmission_scheduler());
    auto scid = connection_metadata.scid;
    auto f = [fast_connection_scheduler, fast_retransmission_scheduler,
              scid](std::string name) {
      VLOG(2) << "Gating Function: " << name << " " << scid;
      fast_connection_scheduler->RecomputeEligibility(scid);
      fast_retransmission_scheduler->RecomputeEligibility(scid);
    };
    ConnectionState& state = *connection_contexts_[scid];

    state.congestion_control_metadata.fabric_congestion_window
        .SetGatingFunction(absl::bind_front(f, "fcwnd"));
    state.congestion_control_metadata.nic_congestion_window.SetGatingFunction(
        absl::bind_front(f, "ncwnd"));

    state.is_op_rate_limited.SetGatingFunction(absl::bind_front(f, "op_rate"));

    state.tx_reliability_metadata.data_window_metadata
        .base_packet_sequence_number.SetGatingFunction(
            absl::bind_front(f, "data_bpsn"));
    state.tx_reliability_metadata.data_window_metadata
        .next_packet_sequence_number.SetGatingFunction(
            absl::bind_front(f, "data_npsn"));
    state.tx_reliability_metadata.data_window_metadata
        .outstanding_requests_counter.SetGatingFunction(
            absl::bind_front(f, "data_orc"));
    state.tx_reliability_metadata.data_window_metadata
        .outstanding_retransmission_requests_counter.SetGatingFunction(
            absl::bind_front(f, "data_orrc"));
    state.tx_reliability_metadata.data_window_metadata.oow_notification_received
        .SetGatingFunction(absl::bind_front(f, "data_oow"));

    state.tx_reliability_metadata.request_window_metadata
        .base_packet_sequence_number.SetGatingFunction(
            absl::bind_front(f, "req_bpsn"));
    state.tx_reliability_metadata.request_window_metadata
        .next_packet_sequence_number.SetGatingFunction(
            absl::bind_front(f, "req_npsn"));
    state.tx_reliability_metadata.request_window_metadata
        .outstanding_requests_counter.SetGatingFunction(
            absl::bind_front(f, "req_orc"));
    state.tx_reliability_metadata.request_window_metadata
        .outstanding_retransmission_requests_counter.SetGatingFunction(
            absl::bind_front(f, "req_orrc"));
    state.tx_reliability_metadata.request_window_metadata
        .oow_notification_received.SetGatingFunction(
            absl::bind_front(f, "req_oow"));
  }

  return absl::OkStatus();
}
};  // namespace isekai
