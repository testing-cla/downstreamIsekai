#include "isekai/host/falcon/event_response_format_adapter.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/packet.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_rate_update_engine_adapter.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/gen2/falcon_types.h"
#include "isekai/host/falcon/gen2/reliability_manager.h"
#include "isekai/host/falcon/rue/algorithm/dram_state_manager.h"
#include "isekai/host/falcon/rue/algorithm/stateful_algorithm.h"
#include "isekai/host/falcon/rue/algorithm/swift.h"
#include "isekai/host/falcon/rue/algorithm/swift.pb.h"
#include "isekai/host/falcon/rue/bits.h"
#include "isekai/host/falcon/rue/fixed.h"
#include "isekai/host/falcon/rue/format.h"
#include "isekai/host/falcon/rue/format_bna.h"

namespace isekai {

// Changes the connection state as a result of a received RUE response.
template <typename EventT, typename ResponseT>
void EventResponseFormatAdapter<EventT, ResponseT>::
    UpdateConnectionStateFromResponse(ConnectionState* connection_state,
                                      const ResponseT* response) const {
  auto& congestion_control_metadata =
      connection_state->congestion_control_metadata;
  // Updates the congestion control metadata.
  if (response->randomize_path) {
    congestion_control_metadata.flow_label =
        falcon_->get_rate_update_engine()->GenerateRandomFlowLabel();
  }
  congestion_control_metadata.fabric_congestion_window =
      response->fabric_congestion_window;
  congestion_control_metadata.inter_packet_gap =
      falcon_->get_rate_update_engine()->FromFalconTimeUnits(
          response->inter_packet_gap);
  congestion_control_metadata.nic_congestion_window =
      response->nic_congestion_window;
  congestion_control_metadata.retransmit_timeout =
      falcon_->get_rate_update_engine()->FromFalconTimeUnits(
          response->retransmit_timeout);
  congestion_control_metadata.cc_metadata = response->cc_metadata;
  congestion_control_metadata.fabric_window_time_marker =
      response->fabric_window_time_marker;
  congestion_control_metadata.nic_window_time_marker =
      response->nic_window_time_marker;
  congestion_control_metadata.nic_window_direction =
      response->nic_window_direction;
  congestion_control_metadata.delay_select = response->delay_select;
  congestion_control_metadata.delay_state = response->delay_state;
  congestion_control_metadata.rtt_state = response->rtt_state;
  congestion_control_metadata.cc_opaque = congestion_control_metadata.cc_opaque;
}

template <>
void EventResponseFormatAdapter<falcon_rue::Event_BNA,
                                falcon_rue::Response_BNA>::
    UpdateConnectionStateFromResponse(
        ConnectionState* connection_state,
        const falcon_rue::Response_BNA* response) const {
  auto& congestion_control_metadata =
      connection_state->congestion_control_metadata;
  // Updates the congestion control metadata.
  uint8_t num_flows = congestion_control_metadata.gen2_flow_labels.size();
  if (num_flows == 1) {
    // Only update flow label for flow ID 0 for single path connections. Flow
    // weights and other flow labels are not relevant for single path
    // connections.
    if (response->flow_label_1_valid) {
      congestion_control_metadata.gen2_flow_labels[0] = response->flow_label_1;
    }
  } else {
    // Update the flow labels if any of them is valid.
    if (response->flow_label_1_valid) {
      congestion_control_metadata.gen2_flow_labels[0] = response->flow_label_1;
    }
    if (response->flow_label_2_valid) {
      congestion_control_metadata.gen2_flow_labels[1] = response->flow_label_2;
    }
    if (response->flow_label_3_valid) {
      congestion_control_metadata.gen2_flow_labels[2] = response->flow_label_3;
    }
    if (response->flow_label_4_valid) {
      congestion_control_metadata.gen2_flow_labels[3] = response->flow_label_4;
    }
    // The datapath expects some weights to be nonzero for multipath
    // connections. If all weights are zero, exit with an error.
    CHECK_GT(response->flow_label_1_weight + response->flow_label_2_weight +
                 response->flow_label_3_weight + response->flow_label_4_weight,
             0);
    congestion_control_metadata.gen2_flow_weights[0] =
        response->flow_label_1_weight;
    congestion_control_metadata.gen2_flow_weights[1] =
        response->flow_label_2_weight;
    congestion_control_metadata.gen2_flow_weights[2] =
        response->flow_label_3_weight;
    congestion_control_metadata.gen2_flow_weights[3] =
        response->flow_label_4_weight;
    if (response->wrr_restart_round) {
      dynamic_cast<Gen2ReliabilityManager*>(
          falcon_->get_packet_reliability_manager())
          ->ResetWrrForConnection(response->connection_id);
    }
  }

  congestion_control_metadata.fabric_congestion_window =
      response->fabric_congestion_window;
  congestion_control_metadata.inter_packet_gap =
      falcon_->get_rate_update_engine()->FromFalconTimeUnits(
          response->inter_packet_gap);  // fipg

  // response.
  // Until ncwnd pacing is supported, the minimum value allowed for an integer
  // ncwnd is 1.
  congestion_control_metadata.nic_congestion_window = std::max<uint32_t>(
      1, falcon_rue::FixedToUint<uint32_t, uint32_t>(
             response->nic_congestion_window, falcon_rue::kFractionalBits));

  congestion_control_metadata.retransmit_timeout =
      falcon_->get_rate_update_engine()->FromFalconTimeUnits(
          response->retransmit_timeout);
  congestion_control_metadata.cc_metadata = response->cc_metadata;
  congestion_control_metadata.fabric_window_time_marker =
      response->fabric_window_time_marker;
  congestion_control_metadata.nic_window_time_marker =
      response->nic_window_time_marker;
  congestion_control_metadata.delay_select = response->delay_select;
  congestion_control_metadata.delay_state = response->delay_state;
  congestion_control_metadata.rtt_state = response->rtt_state;
  congestion_control_metadata.cc_opaque = congestion_control_metadata.cc_opaque;
  congestion_control_metadata.gen2_plb_state = response->plb_state;

  // Update Hawees metadata.
  auto& connection_rdma_xoff_metadata =
      connection_state->connection_xoff_metadata;
  connection_rdma_xoff_metadata.alpha_request = response->alpha_request;
  connection_rdma_xoff_metadata.alpha_response = response->alpha_response;

  // response->csig_select).
}

template <typename EventT, typename ResponseT>
bool EventResponseFormatAdapter<EventT, ResponseT>::IsRandomizePath(
    const ResponseT* response) const {
  return response->randomize_path;
}

template <>
bool EventResponseFormatAdapter<falcon_rue::Event_BNA,
                                falcon_rue::Response_BNA>::
    IsRandomizePath(const falcon_rue::Response_BNA* response) const {
  // For Gen2, if any of the valid bits are set then the response would be
  // signaling a randomize path signal to the datapath. The RUE class which is
  // keeping stats of path changes would then record this change by incrementing
  // a counter.
  return response->flow_label_1_valid || response->flow_label_2_valid ||
         response->flow_label_3_valid || response->flow_label_4_valid;
}

template <typename EventT, typename ResponseT>
void EventResponseFormatAdapter<EventT, ResponseT>::
    FillTimeoutRetransmittedEvent(
        EventT& event, const RueKey* rue_key, const Packet* packet,
        const ConnectionState::CongestionControlMetadata& ccmeta,
        uint8_t retransmit_count) const {
  event.connection_id = rue_key->scid;
  event.event_type = falcon::RueEventType::kRetransmit;
  event.timestamp_1 = 0;  // not used
  event.timestamp_2 = 0;  // not used
  event.timestamp_3 = 0;  // not used
  event.timestamp_4 = 0;  // not used
  event.retransmit_count = retransmit_count;
  event.retransmit_reason = falcon::RetransmitReason::kTimeout;
  event.nack_code = falcon::NackCode::kNotANack;  // not used
  event.forward_hops = 0;                         // not used
  event.rx_buffer_level = 0;                      // not used
  event.cc_metadata = 0;                          // not used
  event.fabric_congestion_window = ccmeta.fabric_congestion_window;
  event.inter_packet_gap =
      falcon_->get_rate_update_engine()->ToTimingWheelTimeUnits(
          ccmeta.inter_packet_gap);
  event.nic_congestion_window = ccmeta.nic_congestion_window;
  event.retransmit_timeout =
      falcon_->get_rate_update_engine()->ToFalconTimeUnits(
          ccmeta.retransmit_timeout);
  event.num_packets_acked = 0;   // not used
  event.event_queue_select = 0;  // not used
  event.delay_select = ccmeta.delay_select;
  event.fabric_window_time_marker = ccmeta.fabric_window_time_marker;
  event.nic_window_time_marker = ccmeta.nic_window_time_marker;
  event.nic_window_direction = ccmeta.nic_window_direction;
  event.base_delay = ccmeta.base_delay;
  event.delay_state = ccmeta.delay_state;
  event.rtt_state = ccmeta.rtt_state;
  event.cc_opaque = ccmeta.cc_opaque;
  event.eack = false;       // not used
  event.eack_drop = false;  // not used
  event.reserved_0 = 0;     // not used
  event.reserved_1 = 0;     // not used
  event.reserved_2 = 0;     // not used
  event.gen_bit = 0;        // not used
}

template <>
void EventResponseFormatAdapter<falcon_rue::Event_BNA,
                                falcon_rue::Response_BNA>::
    FillTimeoutRetransmittedEvent(
        falcon_rue::Event_BNA& event, const RueKey* rue_key,
        const Packet* packet,
        const ConnectionState::CongestionControlMetadata& ccmeta,
        uint8_t retransmit_count) const {
  auto gen2_rue_key = dynamic_cast<const Gen2RueKey*>(rue_key);
  uint32_t num_flows = ccmeta.gen2_flow_labels.size();

  event.multipath_enable = (num_flows == 1) ? false : true;
  event.connection_id = gen2_rue_key->scid;
  // The flow label in the event uses the flow label from the retx packet, not
  // the latest flow label for the flow.
  event.flow_label = packet->metadata.flow_label;
  event.event_type = falcon::RueEventType::kRetransmit;
  event.timestamp_1 = 0;  // not used
  event.timestamp_2 = 0;  // not used
  event.timestamp_3 = 0;  // not used
  event.timestamp_4 = 0;  // not used
  event.retransmit_count = retransmit_count;
  event.retransmit_reason = falcon::RetransmitReason::kTimeout;
  event.nack_code = falcon::NackCode::kNotANack;  // not used
  event.forward_hops = 0;                         // not used
  event.rx_buffer_level = 0;                      // not used
  event.cc_metadata = 0;                          // not used
  event.fabric_congestion_window = ccmeta.fabric_congestion_window;

  event.nic_congestion_window = falcon_rue::UintToFixed<uint32_t, uint32_t>(
      ccmeta.nic_congestion_window, falcon_rue::kFractionalBits);
  event.num_packets_acked = 0;   // not used
  event.event_queue_select = 0;  // not used
  event.delay_select = ccmeta.delay_select;
  event.fabric_window_time_marker = ccmeta.fabric_window_time_marker;
  event.nic_window_time_marker = ccmeta.nic_window_time_marker;
  event.base_delay = ccmeta.base_delay;
  event.delay_state = ccmeta.delay_state;
  event.rtt_state = ccmeta.rtt_state;
  event.cc_opaque = ccmeta.cc_opaque;
  event.eack = false;       // not used
  event.eack_drop = false;  // not used
  event.eack_own = 0;       // not used
  event.reserved_0 = 0;     // not used
  event.reserved_1 = 0;     // not used
  event.gen_bit = 0;        // not used
  event.plb_state = ccmeta.gen2_plb_state;
}

template <typename EventT, typename ResponseT>
void EventResponseFormatAdapter<EventT, ResponseT>::FillNackEvent(
    EventT& event, const RueKey* rue_key, const Packet* packet,
    const ConnectionState::CongestionControlMetadata& ccmeta,
    uint32_t num_packets_acked) const {
  event.connection_id = rue_key->scid;
  event.event_type = falcon::RueEventType::kNack;
  event.timestamp_1 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->nack.timestamp_1);
  event.timestamp_2 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->nack.timestamp_2);
  event.timestamp_3 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->timestamps.sent_timestamp);
  event.timestamp_4 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->timestamps.received_timestamp);
  event.retransmit_count = 0;                                  // not used
  event.retransmit_reason = falcon::RetransmitReason::kEarly;  // not used
  event.nack_code = packet->nack.code;
  event.forward_hops = packet->nack.forward_hops;
  event.rx_buffer_level = packet->nack.rx_buffer_level;
  event.cc_metadata = packet->nack.cc_metadata;
  event.fabric_congestion_window = ccmeta.fabric_congestion_window;
  event.inter_packet_gap =
      falcon_->get_rate_update_engine()->ToTimingWheelTimeUnits(
          ccmeta.inter_packet_gap);
  event.nic_congestion_window = ccmeta.nic_congestion_window;
  event.retransmit_timeout =
      falcon_->get_rate_update_engine()->ToFalconTimeUnits(
          ccmeta.retransmit_timeout);
  event.num_packets_acked = static_cast<uint16_t>(num_packets_acked);
  event.event_queue_select = 0;  // not used
  event.delay_select = ccmeta.delay_select;
  event.fabric_window_time_marker = ccmeta.fabric_window_time_marker;
  event.nic_window_time_marker = ccmeta.nic_window_time_marker;
  event.nic_window_direction = ccmeta.nic_window_direction;
  event.base_delay = ccmeta.base_delay;
  event.delay_state = ccmeta.delay_state;
  event.rtt_state = ccmeta.rtt_state;
  event.cc_opaque = ccmeta.cc_opaque;
  event.eack = false;       // not used
  event.eack_drop = false;  // not used
  event.reserved_0 = 0;     // not used
  event.reserved_1 = 0;     // not used
  event.reserved_2 = 0;     // not used
  event.gen_bit = 0;        // not used
}

template <>
void EventResponseFormatAdapter<falcon_rue::Event_BNA,
                                falcon_rue::Response_BNA>::
    FillNackEvent(falcon_rue::Event_BNA& event, const RueKey* rue_key,
                  const Packet* packet,
                  const ConnectionState::CongestionControlMetadata& ccmeta,
                  uint32_t num_packets_acked) const {
  auto gen2_rue_key = dynamic_cast<const Gen2RueKey*>(rue_key);
  uint32_t num_flows = ccmeta.gen2_flow_labels.size();

  event.multipath_enable = (num_flows == 1) ? false : true;
  event.connection_id = gen2_rue_key->scid;
  // The flow label in the event uses the flow label from the
  // NACK, not the latest flow label for the flow.
  event.flow_label = packet->metadata.flow_label;
  event.event_type = falcon::RueEventType::kNack;
  event.timestamp_1 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->nack.timestamp_1);
  event.timestamp_2 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->nack.timestamp_2);
  event.timestamp_3 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->timestamps.sent_timestamp);
  event.timestamp_4 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->timestamps.received_timestamp);
  event.retransmit_count = 0;                                  // not used
  event.retransmit_reason = falcon::RetransmitReason::kEarly;  // not used
  event.nack_code = packet->nack.code;
  event.forward_hops = packet->nack.forward_hops;
  event.rx_buffer_level = packet->nack.rx_buffer_level;
  event.cc_metadata = packet->nack.cc_metadata;
  event.fabric_congestion_window = ccmeta.fabric_congestion_window;

  event.nic_congestion_window = falcon_rue::UintToFixed<uint32_t, uint32_t>(
      ccmeta.nic_congestion_window, falcon_rue::kFractionalBits);
  event.num_packets_acked = static_cast<uint16_t>(num_packets_acked);
  event.event_queue_select = 0;  // not used
  event.delay_select = ccmeta.delay_select;
  event.fabric_window_time_marker = ccmeta.fabric_window_time_marker;
  event.nic_window_time_marker = ccmeta.nic_window_time_marker;
  event.base_delay = ccmeta.base_delay;
  event.delay_state = ccmeta.delay_state;
  event.rtt_state = ccmeta.rtt_state;
  event.cc_opaque = ccmeta.cc_opaque;
  event.eack = false;       // not used
  event.eack_drop = false;  // not used
  event.eack_own = 0;       // not used
  event.reserved_0 = 0;     // not used
  event.reserved_1 = 0;     // not used
  event.gen_bit = 0;        // not used
  event.plb_state = ccmeta.gen2_plb_state;
}

template <typename EventT, typename ResponseT>
void EventResponseFormatAdapter<EventT, ResponseT>::FillExplicitAckEvent(
    EventT& event, const RueKey* rue_key, const Packet* packet,
    const ConnectionState::CongestionControlMetadata& ccmeta,
    uint32_t num_packets_acked, bool eack, bool eack_drop) const {
  event.connection_id = rue_key->scid;
  event.event_type = falcon::RueEventType::kAck;
  event.timestamp_1 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->ack.timestamp_1);
  event.timestamp_2 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->ack.timestamp_2);
  event.timestamp_3 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->timestamps.sent_timestamp);
  event.timestamp_4 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->timestamps.received_timestamp);
  event.retransmit_count = 0;                                  // not used
  event.retransmit_reason = falcon::RetransmitReason::kEarly;  // not used
  event.nack_code = falcon::NackCode::kNotANack;               // not used
  event.forward_hops = packet->ack.forward_hops;
  event.rx_buffer_level = packet->ack.rx_buffer_level;
  event.cc_metadata = packet->ack.cc_metadata;
  event.fabric_congestion_window = ccmeta.fabric_congestion_window;
  event.inter_packet_gap =
      falcon_->get_rate_update_engine()->ToTimingWheelTimeUnits(
          ccmeta.inter_packet_gap);
  event.nic_congestion_window = ccmeta.nic_congestion_window;
  event.retransmit_timeout =
      falcon_->get_rate_update_engine()->ToFalconTimeUnits(
          ccmeta.retransmit_timeout);
  event.num_packets_acked = static_cast<uint16_t>(num_packets_acked);
  event.event_queue_select = 0;  // not used
  event.delay_select = ccmeta.delay_select;
  event.fabric_window_time_marker = ccmeta.fabric_window_time_marker;
  event.nic_window_time_marker = ccmeta.nic_window_time_marker;
  event.nic_window_direction = ccmeta.nic_window_direction;
  event.base_delay = ccmeta.base_delay;
  event.delay_state = ccmeta.delay_state;
  event.rtt_state = ccmeta.rtt_state;
  event.cc_opaque = ccmeta.cc_opaque;
  event.reserved_0 = 0;  // not used
  event.reserved_1 = 0;  // not used
  event.reserved_2 = 0;  // not used
  event.gen_bit = 0;     // not used
  event.eack = eack;
  event.eack_drop = eack_drop;
}

template <>
void EventResponseFormatAdapter<falcon_rue::Event_BNA,
                                falcon_rue::Response_BNA>::
    FillExplicitAckEvent(
        falcon_rue::Event_BNA& event, const RueKey* rue_key,
        const Packet* packet,
        const ConnectionState::CongestionControlMetadata& ccmeta,
        uint32_t num_packets_acked, bool eack, bool eack_drop) const {
  auto gen2_rue_key = dynamic_cast<const Gen2RueKey*>(rue_key);
  uint32_t num_flows = ccmeta.gen2_flow_labels.size();

  event.multipath_enable = (num_flows == 1) ? false : true;
  event.connection_id = gen2_rue_key->scid;
  // The flow label in the event uses the flow label from the ACK, not the
  // latest flow label for the flow.
  event.flow_label = packet->metadata.flow_label;
  event.event_type = falcon::RueEventType::kAck;
  event.timestamp_1 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->ack.timestamp_1);
  event.timestamp_2 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->ack.timestamp_2);
  event.timestamp_3 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->timestamps.sent_timestamp);
  event.timestamp_4 = falcon_->get_rate_update_engine()->ToFalconTimeUnits(
      packet->timestamps.received_timestamp);
  event.retransmit_count = 0;                                  // not used
  event.retransmit_reason = falcon::RetransmitReason::kEarly;  // not used
  event.nack_code = falcon::NackCode::kNotANack;               // not used
  event.forward_hops = packet->ack.forward_hops;
  event.rx_buffer_level = packet->ack.rx_buffer_level;
  event.cc_metadata = packet->ack.cc_metadata;
  event.fabric_congestion_window = ccmeta.fabric_congestion_window;

  event.nic_congestion_window = falcon_rue::UintToFixed<uint32_t, uint32_t>(
      ccmeta.nic_congestion_window, falcon_rue::kFractionalBits);
  event.num_packets_acked = static_cast<uint16_t>(num_packets_acked);
  event.event_queue_select = 0;  // not used
  event.delay_select = ccmeta.delay_select;
  event.fabric_window_time_marker = ccmeta.fabric_window_time_marker;
  event.nic_window_time_marker = ccmeta.nic_window_time_marker;
  event.base_delay = ccmeta.base_delay;
  event.delay_state = ccmeta.delay_state;
  event.rtt_state = ccmeta.rtt_state;
  event.cc_opaque = ccmeta.cc_opaque;
  event.reserved_0 = 0;  // not used
  event.reserved_1 = 0;  // not used
  event.gen_bit = 0;     // not used
  event.eack_own = 0;    // not used
  event.eack = eack;
  event.eack_drop = eack_drop;
  event.plb_state = ccmeta.gen2_plb_state;
}

template <typename EventT, typename ResponseT>
std::unique_ptr<RueAdapterInterface<EventT, ResponseT>>
EventResponseFormatAdapter<EventT, ResponseT>::GetSwiftRueAdapter(
    const ::isekai::rue::SwiftConfiguration& configuration,
    uint32_t initial_fcwnd_fixed) const {
  auto algorithm =
      ::isekai::rue::Swift<EventT, ResponseT>::Create(configuration);
  CHECK_OK(algorithm.status());
  return std::make_unique<
      RueAdapter<::isekai::rue::Swift<EventT, ResponseT>, EventT, ResponseT>>(
      std::move(algorithm.value()));
}

template <>
std::unique_ptr<
    RueAdapterInterface<falcon_rue::Event_BNA, falcon_rue::Response_BNA>>
EventResponseFormatAdapter<falcon_rue::Event_BNA, falcon_rue::Response_BNA>::
    GetSwiftRueAdapter(const ::isekai::rue::SwiftConfiguration& configuration,
                       uint32_t initial_fcwnd_fixed) const {
  // In BNA, the instantiated Swift algorithm should be stateful.
  using StateT = ::isekai::rue::RueConnectionState;
  using EventT = falcon_rue::Event_BNA;
  using ResponseT = falcon_rue::Response_BNA;
  using DramStateManagerT =
      ::isekai::rue::HashMapDramStateManagerWithOnDemandInit<EventT>;
  using AlgorithmT = ::isekai::rue::Swift<EventT, ResponseT>;
  using StatefulAlgorithmT = ::isekai::rue::StatefulAlgorithm<
      /*Algorithm=*/AlgorithmT,
      /*DramStateManagerT=*/DramStateManagerT,
      /*ForBenchmarking=*/false>;

  CHECK_OK_THEN_ASSIGN(
      std::unique_ptr<StatefulAlgorithmT> swift,
      AlgorithmT::template Create<StatefulAlgorithmT>(configuration));

  auto default_state_value = StateT{};
  // In Gen2 multipathing, we need to make sure that the initial fcwnd for a
  // connection (initial_fcwnd_fixed) is the sum of the initial flow
  // fcwnds (default_state_value.fcwnd[i]) as initialized in the Swift
  // algorithm.
  size_t num_flows = default_state_value.fcwnd.size();
  uint32_t initial_flow_fcwnd_fixed = initial_fcwnd_fixed / num_flows;
  for (size_t flow_id = 0; flow_id < num_flows; ++flow_id) {
    default_state_value.fcwnd[flow_id] = initial_flow_fcwnd_fixed;
  }
  auto manager =
      std::make_unique<DramStateManagerT>(sizeof(StateT), &default_state_value);
  swift->set_dram_state_manager(std::move(manager));
  return std::make_unique<RueAdapter<StatefulAlgorithmT, EventT, ResponseT>>(
      std::move(swift));
}

// Explicit template instantiations.
template class EventResponseFormatAdapter<falcon_rue::Event,
                                          falcon_rue::Response>;
template class EventResponseFormatAdapter<falcon_rue::Event_BNA,
                                          falcon_rue::Response_BNA>;

}  // namespace isekai
