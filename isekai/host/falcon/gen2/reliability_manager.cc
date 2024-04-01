#include "isekai/host/falcon/gen2/reliability_manager.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/time/time.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/constants.h"
#include "isekai/common/packet.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"
#include "isekai/host/falcon/falcon_protocol_packet_reliability_manager.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/falcon_utils.h"
#include "isekai/host/falcon/gen2/falcon_types.h"
#include "isekai/host/falcon/gen2/falcon_utils.h"
#include "isekai/host/falcon/weighted_round_robin_policy.h"

namespace isekai {

Gen2ReliabilityManager::Gen2ReliabilityManager(FalconModelInterface* falcon)
    : ProtocolPacketReliabilityManager(falcon) {
  const FalconConfig::Gen2ConfigOptions& config =
      falcon_->get_config()->gen2_config_options();
  if (config.has_multipath_config()) {
    const FalconConfig::Gen2ConfigOptions::MultipathConfig& multipath_config =
        config.multipath_config();
    if (multipath_config.has_path_selection_policy()) {
      multipath_config_.path_selection_policy =
          multipath_config.path_selection_policy();
    }
    if (multipath_config.has_single_path_connection_ack_unrolling_delay_ns()) {
      multipath_config_.single_path_connection_ack_unrolling_delay =
          absl::Nanoseconds(
              multipath_config.single_path_connection_ack_unrolling_delay_ns());
    }
    if (multipath_config.has_multipath_connection_ack_unrolling_delay_ns()) {
      multipath_config_.multipath_connection_ack_unrolling_delay =
          absl::Nanoseconds(
              multipath_config.multipath_connection_ack_unrolling_delay_ns());
    }
    if (multipath_config.has_single_path_connection_accept_stale_acks()) {
      multipath_config_.single_path_connection_accept_stale_acks =
          multipath_config.single_path_connection_accept_stale_acks();
    }
    if (multipath_config.has_multipath_connection_accept_stale_acks()) {
      multipath_config_.multipath_connection_accept_stale_acks =
          multipath_config.multipath_connection_accept_stale_acks();
    }
    if (multipath_config.has_batched_packet_scheduling()) {
      multipath_config_.batched_packet_scheduling =
          multipath_config.batched_packet_scheduling();
    }
    if (multipath_config.retx_flow_label()) {
      multipath_config_.retx_flow_label_policy =
          multipath_config.retx_flow_label();
    }
  }
  decrement_orc_on_pull_response_ = falcon->get_config()
                                        ->gen2_config_options()
                                        .decrement_orc_on_pull_response();
}

// Initializes the per-connection state stored in the packet reliability
// manager. In Gen2, there is a per-connection WRR/RR policy that needs to be
// initialized when a connection is set up.
void Gen2ReliabilityManager::InitializeConnection(uint32_t scid) {
  // Get a handle on the connection and transaction state along with the packet.
  ConnectionStateManager* state_manager = falcon_->get_state_manager();
  CHECK_OK_THEN_ASSIGN(ConnectionState* const connection_state,
                       state_manager->PerformDirectLookup(scid));
  // The policy expects a function that will fetch weights at the end of a
  // round-robin round or when a the policy is reset. This function should take
  // in a flow ID as input and return the corresponding flow's weight for the
  // WRR policy as output.
  std::function<int(uint8_t)> fetch_flow_weight;
  switch (multipath_config_.path_selection_policy) {
    case FalconConfig::Gen2ConfigOptions::MultipathConfig::ROUND_ROBIN:
      // nullptr makes the policy a vanilla RR policy.
      fetch_flow_weight = nullptr;
      break;
    case FalconConfig::Gen2ConfigOptions::MultipathConfig::WEIGHTED_ROUND_ROBIN:
      // For WRR, we fetch the up-to-date weights in
      // congestion_control_metadata.
      fetch_flow_weight = [connection_state](uint8_t flow_id) {
        return connection_state->congestion_control_metadata
            .gen2_flow_weights[flow_id];
      };
      break;
  }
  auto wrr_policy = std::make_unique<WeightedRoundRobinPolicy<uint8_t>>(
      fetch_flow_weight, multipath_config_.batched_packet_scheduling);
  uint32_t num_flows =
      connection_state->congestion_control_metadata.gen2_flow_weights.size();
  for (uint8_t flow_id = 0; flow_id < num_flows; flow_id++) {
    CHECK_OK(wrr_policy->InitializeEntity(flow_id));
    wrr_policy->MarkEntityActive(flow_id);
  }
  // Save the WRR policy in the per-connection scheduling policy map.
  path_selection_policies_[scid] = std::move(wrr_policy);
}

// Resets the WRR policy for the given connection, forcing the policy to
// immediately update flow credits using the flow weights in the connection
// state.
void Gen2ReliabilityManager::ResetWrrForConnection(uint32_t scid) {
  auto it = path_selection_policies_.find(scid);
  if (it == path_selection_policies_.end()) {
    LOG(FATAL) << "Connection ID not found in WRR map";
  }
  it->second->ResetAndRestartRound();
}

// For initial retransmissions, uses WRR/RR policy to choose the flow label. For
// retransmissions, uses the same flow ID as original initial transmission; the
// flow label can change from the initial transmission if RUE changed that
// flow's label since the initial transmission.
uint32_t Gen2ReliabilityManager::ChooseOutgoingPacketFlowLabel(
    falcon::PacketType packet_type, uint32_t rsn,
    const ConnectionState* connection_state) {
  // Get a handle on the packet's PacketMetadata.
  CHECK_OK_THEN_ASSIGN(TransactionMetadata* const transaction,
                       connection_state->GetTransaction(
                           TransactionKey(rsn, GetTransactionLocation(
                                                   /*type=*/packet_type,
                                                   /*incoming=*/false))));
  CHECK_OK_THEN_ASSIGN(PacketMetadata* const packet_metadata,
                       transaction->GetPacketMetadata(packet_type));

  if (packet_metadata->transmit_attempts > 1) {
    // The outgoing packet is a retransmission. Use same flow ID as initial
    // transmission.
    const Packet* packet = packet_metadata->active_packet.get();
    return ChooseRetxFlowLabel(packet, connection_state);
  }
  // The outgoing packet is an initial transmission. Use WRR/RR policy to
  // choose the flow label.
  uint32_t scid = connection_state->connection_metadata.scid;
  CHECK_OK_THEN_ASSIGN(uint8_t chosen_flow_id,
                       path_selection_policies_[scid]->GetNextEntity());
  // Return the up-to-date flow label for the chosen flow ID.
  return connection_state->congestion_control_metadata
      .gen2_flow_labels[chosen_flow_id];
}

uint32_t Gen2ReliabilityManager::ChooseRetxFlowLabel(
    const Packet* packet, const ConnectionState* connection_state) {
  uint32_t flow_label = packet->metadata.flow_label;
  uint32_t scid = connection_state->connection_metadata.scid;
  switch (multipath_config_.retx_flow_label_policy) {
    case FalconConfig::Gen2ConfigOptions::MultipathConfig::
        SAME_FLOW_ID_AS_INITIAL_TX: {
      // Use the same flow label as the initial tx packet.
      uint8_t chosen_flow_id =
          GetFlowIdFromFlowLabel(flow_label, falcon_, scid);
      return connection_state->congestion_control_metadata
          .gen2_flow_labels[chosen_flow_id];
    }
  }
}

// Piggybacks ACK information on an outgoing data/request packet.
void Gen2ReliabilityManager::PiggybackAck(uint32_t scid, Packet* packet) {
  uint8_t flow_id =
      GetFlowIdFromFlowLabel(packet->metadata.flow_label, falcon_, scid);
  auto ack_coalescing_key = Gen2AckCoalescingKey(scid, flow_id);
  CHECK_OK(falcon_->get_ack_coalescing_engine()->PiggybackAck(
      ack_coalescing_key, packet));
}

// Accumulates the number of packets acked to be used for the congestion
// control algorithm in a RUE event.
void Gen2ReliabilityManager::AccumulateNumAcked(
    ConnectionState* connection_state,
    const OutstandingPacketContext* packet_context) {
  uint32_t num_flows =
      connection_state->congestion_control_metadata.gen2_flow_labels.size();
  auto gen2_packet_context =
      dynamic_cast<const Gen2OutstandingPacketContext*>(packet_context);
  uint8_t flow_id = gen2_packet_context->flow_id;
  if (num_flows == 1 &&
      multipath_config_.single_path_connection_ack_unrolling_delay >
          absl::ZeroDuration()) {
    CHECK_OK(falcon_->get_environment()->ScheduleEvent(
        multipath_config_.single_path_connection_ack_unrolling_delay,
        [connection_state, flow_id]() {
          connection_state->congestion_control_metadata
              .gen2_num_acked[flow_id] += 1;
        }));
  } else if (num_flows > 1 &&
             multipath_config_.multipath_connection_ack_unrolling_delay >
                 absl::ZeroDuration()) {
    CHECK_OK(falcon_->get_environment()->ScheduleEvent(
        multipath_config_.multipath_connection_ack_unrolling_delay,
        [connection_state, flow_id]() {
          connection_state->congestion_control_metadata
              .gen2_num_acked[flow_id] += 1;
        }));
  } else {
    // No ACK unrolling delays, so just immediately accumulate the ACKs in the
    // Gen2 connection state.
    connection_state->congestion_control_metadata.gen2_num_acked[flow_id] += 1;
  }
}

// Handles a stale ACK where RX window BPSN in ACK < TX window BPSN.
absl::Status Gen2ReliabilityManager::HandleStaleAck(const Packet* ack_packet) {
  // Get a handle of the connection state.
  uint32_t scid = GetFalconPacketConnectionId(*ack_packet);
  CHECK_OK_THEN_ASSIGN(auto connection_state,
                       falcon_->get_state_manager()->PerformDirectLookup(scid));
  uint32_t num_flows =
      connection_state->congestion_control_metadata.gen2_flow_labels.size();
  if ((num_flows == 1 &&
       multipath_config_.single_path_connection_accept_stale_acks) ||
      (num_flows > 1 &&
       multipath_config_.multipath_connection_accept_stale_acks)) {
    // As discussed in b/293321846, Gen2 drops stale ACKs with num_acked = 0.
    uint8_t flow_id =
        GetFlowIdFromFlowLabel(ack_packet->metadata.flow_label, falcon_, scid);
    if (connection_state->congestion_control_metadata.gen2_num_acked[flow_id] ==
        0) {
      return absl::FailedPreconditionError(
          "Stale ACK with num_acked=0. Dropped.");
    }
    // Send a RUE event if the corresponding config flag is enabled. For
    // multipath connections, ACKs from different flows can arrive out-of-order
    // at the sender. For example, ACK 1 from flow 1 then ACK 2 from flow 2 are
    // sent (in this order) from the receiver, but ACK 2 can arrive before ACK 1
    // at the sender because they take different paths in the network. However,
    // ACK 1 - which is now considered stale for the connection window state -
    // should still be processed because it contains new information for flow
    // 1's path (i.e., delay state) that is still relevant for the RUE.
    falcon_->get_rate_update_engine()->ExplicitAckReceived(
        ack_packet, ack_packet->ack.ack_type == Packet::Ack::kEack, false);
    return absl::OkStatus();
  }
  // Otherwise, revert to Gen1 behavior.
  return ProtocolPacketReliabilityManager::HandleStaleAck(ack_packet);
}

// Records the input packet as outstanding and changes the connection and
// transaction state accordingly. In Gen2, we also add flow_id to the
// outstanding packet context because it is needed for ACK unrolling.
void Gen2ReliabilityManager::AddToOutstandingPackets(
    ConnectionState* const connection_state,
    TransactionMetadata* const transaction,
    PacketMetadata* const packet_metadata) {
  auto* window = GetAppropriateTxWindow(
      &connection_state->tx_reliability_metadata, packet_metadata->type);
  window->outstanding_packets.insert(RetransmissionWorkId(
      transaction->rsn, packet_metadata->psn, packet_metadata->type));
  // Add packet to outstanding packet contexts.
  uint8_t flow_id = GetFlowIdFromFlowLabel(
      packet_metadata->active_packet->metadata.flow_label, falcon_,
      connection_state->connection_metadata.scid);
  window->outstanding_packet_contexts[packet_metadata->psn] =
      std::make_unique<Gen2OutstandingPacketContext>(
          transaction->rsn, packet_metadata->type, flow_id);
}

void Gen2ReliabilityManager::DequeuePacketFromRetxScheduler(
    uint32_t scid, uint32_t rsn, falcon::PacketType type,
    TransactionMetadata* transaction) {
  transaction->decrement_orrc_on_pull_response = false;
  ProtocolPacketReliabilityManager::DequeuePacketFromRetxScheduler(
      scid, rsn, type, transaction);
}

// Performs the sliding window related processing for incoming packet or
// resync packets. In Gen-2, it decrements ORC/ORRC for Pull transactions when
// Pull Data is received based on the NIC configuration.
absl::Status Gen2ReliabilityManager::HandleIncomingPacketOrResync(
    const Packet* packet) {
  auto status =
      ProtocolPacketReliabilityManager::HandleIncomingPacketOrResync(packet);
  if (decrement_orc_on_pull_response_ && status.ok() &&
      packet->packet_type == falcon::PacketType::kPullData) {
    auto cid = packet->falcon.dest_cid;
    // Decrement ORC as we have received Pull Data for the Pull transaction.
    DecrementOutstandingRequestCountOnPullResponse(cid);
    // Get a handle on the Pull Request packet of the Pull transaction.
    ConnectionStateManager* const state_manager = falcon_->get_state_manager();
    CHECK_OK_THEN_ASSIGN(ConnectionState* const connection_state,
                         state_manager->PerformDirectLookup(cid));
    CHECK_OK_THEN_ASSIGN(
        auto transaction,
        connection_state->GetTransaction(TransactionKey(
            packet->falcon.rsn, GetTransactionLocation(
                                    /*type=*/falcon::PacketType::kPullData,
                                    /*incoming=*/true))));
    CHECK_OK_THEN_ASSIGN(
        auto packet_metadata,
        transaction->GetPacketMetadata(falcon::PacketType::kPullRequest));
    // Decrement ORRC in case this is > 1 retransmission of the Pull Request
    // packet.
    if (packet_metadata->transmit_attempts > 1 &&
        transaction->decrement_orrc_on_pull_response) {
      DecrementOutstandingRetransmittedRequestCountOnPullResponse(cid);
    }
  }
  return status;
}

// Decrements outstanding request count when an ACK is received leaving the case
// when the NIC is configured to decrement ORC on receiving Pull Response for
// Pull transactions.
void Gen2ReliabilityManager::DecrementOutstandingRequestCount(
    uint32_t scid, falcon::PacketType type) {
  if (type == falcon::PacketType::kPullRequest &&
      decrement_orc_on_pull_response_) {
    return;
  }
  ProtocolPacketReliabilityManager::DecrementOutstandingRequestCount(scid,
                                                                     type);
}

// Decrement outstanding request count when a Pull Response is received.
void Gen2ReliabilityManager::DecrementOutstandingRequestCountOnPullResponse(
    uint32_t scid) {
  // Get a handle on the connection state.
  ConnectionStateManager* const state_manager = falcon_->get_state_manager();
  CHECK_OK_THEN_ASSIGN(auto connection_state,
                       state_manager->PerformDirectLookup(scid));
  connection_state->tx_reliability_metadata.request_window_metadata
      .outstanding_requests_counter--;
}

// Decrements outstanding retransmission request count when an ACK is
// received leaving the case when the NIC is configured to decrement ORRC on
// receiving Pull Response for Pull transactions.
void Gen2ReliabilityManager::DecrementOutstandingRetransmittedRequestCount(
    uint32_t scid, falcon::PacketType type, bool is_acked) {
  if (decrement_orc_on_pull_response_ &&
      type == falcon::PacketType::kPullRequest && is_acked) {
    return;
  }
  ProtocolPacketReliabilityManager::
      DecrementOutstandingRetransmittedRequestCount(scid, type, is_acked);
}

// Decrement outstanding retransmission request count when a Pull Response is
// received.
void Gen2ReliabilityManager::
    DecrementOutstandingRetransmittedRequestCountOnPullResponse(uint32_t scid) {
  // Get a handle on the connection state.
  ConnectionStateManager* const state_manager = falcon_->get_state_manager();
  CHECK_OK_THEN_ASSIGN(auto connection_state,
                       state_manager->PerformDirectLookup(scid));
  connection_state->tx_reliability_metadata.request_window_metadata
      .outstanding_retransmission_requests_counter--;
}

}  // namespace isekai
