#ifndef ISEKAI_HOST_FALCON_RUE_FORMAT_H_
#define ISEKAI_HOST_FALCON_RUE_FORMAT_H_

#include <cstdint>

#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/rue/format_bna.h"
// t_rue_sharing:strip_begin(Not needed in external)
#include "isekai/host/falcon/rue/format_dna_a.h"
#include "isekai/host/falcon/rue/format_dna_b.h"
// t_rue_sharing:strip_end
#include "isekai/host/falcon/rue/format_dna_c.h"

namespace falcon_rue {

// t_rue_sharing:strip_begin(Not needed in external)
// Sets DNA_A Response.
inline void SetResponse(
    uint32_t connection_id, bool randomize_path, uint32_t cc_metadata,
    uint32_t fabric_congestion_window, uint32_t inter_packet_gap,
    uint32_t nic_congestion_window, uint32_t retransmit_timeout,
    uint32_t fabric_window_time_marker, uint32_t nic_window_time_marker,
    falcon::WindowDirection nic_window_direction, uint8_t event_queue_select,
    falcon::DelaySelect delay_select, uint32_t base_delay, uint32_t delay_state,
    uint32_t rtt_state, uint8_t cc_opaque, Response_DNA_A& response) {
  response = (Response_DNA_A){
      .connection_id = connection_id,
      .randomize_path = randomize_path,
      .eack_own = 0,  // 2 bits unused by Falcon.
      .cc_metadata = cc_metadata,
      .fabric_congestion_window = fabric_congestion_window,
      .inter_packet_gap = inter_packet_gap,
      .nic_congestion_window = nic_congestion_window,
      .retransmit_timeout = retransmit_timeout,
      .fabric_window_time_marker = fabric_window_time_marker,
      .nic_window_time_marker = nic_window_time_marker,
      .nic_window_direction = nic_window_direction,
      .event_queue_select = event_queue_select,
      .delay_select = delay_select,
      .base_delay = base_delay,  // Doesn't change.
      .delay_state = delay_state,
      .rtt_state = rtt_state,
      .cc_opaque = cc_opaque,
  };
}

// Sets DNA_B Response.
// - event_queue_select and delay_select moved ahead in format.
inline void SetResponse(
    uint32_t connection_id, bool randomize_path, uint32_t cc_metadata,
    uint32_t fabric_congestion_window, uint32_t inter_packet_gap,
    uint32_t nic_congestion_window, uint32_t retransmit_timeout,
    uint32_t fabric_window_time_marker, uint32_t nic_window_time_marker,
    falcon::WindowDirection nic_window_direction, uint8_t event_queue_select,
    falcon::DelaySelect delay_select, uint32_t base_delay, uint32_t delay_state,
    uint32_t rtt_state, uint8_t cc_opaque, Response_DNA_B& response) {
  response = (Response_DNA_B){
      .connection_id = connection_id,
      .randomize_path = randomize_path,
      .eack_own = 0,  // 2 bits unused by Falcon.
      .cc_metadata = cc_metadata,
      .fabric_congestion_window = fabric_congestion_window,
      .inter_packet_gap = inter_packet_gap,
      .nic_congestion_window = nic_congestion_window,
      .retransmit_timeout = retransmit_timeout,
      .event_queue_select = event_queue_select,
      .delay_select = delay_select,
      .fabric_window_time_marker = fabric_window_time_marker,
      .nic_window_time_marker = nic_window_time_marker,
      .nic_window_direction = nic_window_direction,
      .base_delay = base_delay,  // Doesn't change.
      .delay_state = delay_state,
      .rtt_state = rtt_state,
      .cc_opaque = cc_opaque,
  };
}
// t_rue_sharing:strip_end

// Sets DNA_C Response.
// - No change from DNA_B.
inline void SetResponse(
    uint32_t connection_id, bool randomize_path, uint32_t cc_metadata,
    uint32_t fabric_congestion_window, uint32_t inter_packet_gap,
    uint32_t nic_congestion_window, uint32_t retransmit_timeout,
    uint32_t fabric_window_time_marker, uint32_t nic_window_time_marker,
    falcon::WindowDirection nic_window_direction, uint8_t event_queue_select,
    falcon::DelaySelect delay_select, uint32_t base_delay, uint32_t delay_state,
    uint32_t rtt_state, uint8_t cc_opaque, Response_DNA_C& response) {
  response = (Response_DNA_C){
      .connection_id = connection_id,
      .randomize_path = randomize_path,
      .eack_own = 0,  // 2 bits unused by Falcon.
      .cc_metadata = cc_metadata,
      .fabric_congestion_window = fabric_congestion_window,
      .inter_packet_gap = inter_packet_gap,
      .nic_congestion_window = nic_congestion_window,
      .retransmit_timeout = retransmit_timeout,
      .event_queue_select = event_queue_select,
      .delay_select = delay_select,
      .fabric_window_time_marker = fabric_window_time_marker,
      .nic_window_time_marker = nic_window_time_marker,
      .nic_window_direction = nic_window_direction,
      .base_delay = base_delay,  // Doesn't change.
      .delay_state = delay_state,
      .rtt_state = rtt_state,
      .cc_opaque = cc_opaque,
  };
}

// Sets BNA Response.
inline void SetResponse(
    uint32_t connection_id, uint32_t cc_metadata,
    uint32_t fabric_congestion_window, uint32_t fabric_inter_packet_gap,
    uint32_t nic_congestion_window, uint32_t retransmit_timeout,
    uint32_t fabric_window_time_marker, uint32_t nic_window_time_marker,
    uint8_t event_queue_select, falcon::DelaySelect delay_select,
    uint32_t base_delay, uint32_t delay_state, uint32_t rtt_state,
    uint32_t cc_opaque, uint32_t plb_state, uint8_t alpha_request,
    uint8_t alpha_response, uint32_t nic_inter_packet_gap,
    uint32_t flow_label_1, uint32_t flow_label_2, uint32_t flow_label_3,
    uint32_t flow_label_4, bool flow_label_1_valid, bool flow_label_2_valid,
    bool flow_label_3_valid, bool flow_label_4_valid,
    uint8_t flow_label_1_weight, uint8_t flow_label_2_weight,
    uint8_t flow_label_3_weight, uint8_t flow_label_4_weight,
    bool wrr_restart_round, uint8_t flow_id, bool csig_enable,
    uint8_t csig_select, uint8_t ar_rate, Response_BNA& response) {
  response = (Response_BNA){
      .connection_id = connection_id,
      .fabric_congestion_window = fabric_congestion_window,
      .inter_packet_gap = fabric_inter_packet_gap,
      .nic_congestion_window = nic_congestion_window,
      .nic_inter_packet_gap = nic_inter_packet_gap,
      .cc_metadata = cc_metadata,
      .alpha_request = alpha_request,
      .alpha_response = alpha_response,
      .fabric_window_time_marker = fabric_window_time_marker,
      .nic_window_time_marker = nic_window_time_marker,
      .base_delay = base_delay,
      .delay_state = delay_state,
      .plb_state = plb_state,
      .cc_opaque = cc_opaque,
      .delay_select = delay_select,
      .ar_rate = ar_rate,
      .rtt_state = rtt_state,
      .flow_label_1 = flow_label_1,
      .flow_label_2 = flow_label_2,
      .flow_label_3 = flow_label_3,
      .flow_label_4 = flow_label_4,
      .flow_label_1_weight = flow_label_1_weight,
      .flow_label_2_weight = flow_label_2_weight,
      .flow_label_3_weight = flow_label_3_weight,
      .flow_label_4_weight = flow_label_4_weight,
      .flow_label_1_valid = flow_label_1_valid,
      .flow_label_2_valid = flow_label_2_valid,
      .flow_label_3_valid = flow_label_3_valid,
      .flow_label_4_valid = flow_label_4_valid,
      .wrr_restart_round = wrr_restart_round,
      .flow_id = flow_id,
      .csig_enable = csig_enable,
      .csig_select = csig_select,
      .retransmit_timeout = retransmit_timeout,
      .event_queue_select = event_queue_select,
  };
}

//
#ifndef BASALT
using Event = Event_DNA_C;
using Response = Response_DNA_C;
#else
using Event = Event_BNA;
using Response = Response_BNA;
#endif
}  // namespace falcon_rue

#endif  // ISEKAI_HOST_FALCON_RUE_FORMAT_H_
