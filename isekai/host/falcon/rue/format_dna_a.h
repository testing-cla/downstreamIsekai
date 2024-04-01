#ifndef ISEKAI_HOST_FALCON_RUE_FORMAT_DNA_A_H_
#define ISEKAI_HOST_FALCON_RUE_FORMAT_DNA_A_H_

#include <cstdint>

#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/rue/bits.h"
#include "isekai/host/falcon/rue/constants.h"

namespace falcon_rue {

// The comment above a variable refers to the name for the field in Falcon MAS.
struct __attribute__((packed)) Event_DNA_A {
  // cid
  uint32_t connection_id : kConnectionIdBits;
  // event_type
  falcon::RueEventType event_type : kEventTypeBits;
  // t1
  uint32_t timestamp_1 : kTimeBits;
  // t2
  uint32_t timestamp_2 : kTimeBits;
  // t3
  uint32_t timestamp_3 : kTimeBits;
  // t4
  uint32_t timestamp_4 : kTimeBits;
  // retx_count
  uint8_t retransmit_count : kDnaRetransmitCountBits;
  // retx_reason
  falcon::RetransmitReason retransmit_reason : kRetransmitReasonBits;
  // nack_code
  falcon::NackCode nack_code : kNackCodeBits;
  // forward_hops
  uint8_t forward_hops : kForwardHopsBits;
  // rx_req_buf_level
  uint8_t rx_buffer_level : kRxBufferLevelBits;
  // eack_own bits
  uint8_t eack_own : kEackOwnBits;
  // cc_meta
  uint32_t cc_metadata : kCcMetadataBits;
  // cwnd_frac and cwnd concatenated
  uint32_t fabric_congestion_window : kFabricCongestionWindowBits;
  // ipg
  uint32_t inter_packet_gap : kInterPacketGapBits;
  // ncwnd
  uint32_t nic_congestion_window : kDnaNicCongestionWindowBits;
  // retx_to
  uint32_t retransmit_timeout : kTimeBits;
  // num_acked
  uint16_t num_packets_acked : kNumPacketsAckedBits;
  // ecn_cntr
  uint16_t ecn_counter : kEcnCounterBits;
  // window_guard
  uint32_t fabric_window_time_marker : kTimeBits;
  // ncwnd_guard
  uint32_t nic_window_time_marker : kTimeBits;
  // ncwnd_dir
  falcon::WindowDirection nic_window_direction : kWindowDirectionBits;
  // event_q_sel
  uint8_t event_queue_select : kEventQueueSelectBits;
  // delay_sel
  falcon::DelaySelect delay_select : kDelaySelectBits;
  // delay_base
  uint32_t base_delay : kTimeBits;
  // delay_state
  uint32_t delay_state : kTimeBits;
  // smoothed_rtt
  uint32_t rtt_state : kTimeBits;
  // cc_opaque
  // bit 0 being used by HW-RUE for randomizing paths
  uint8_t cc_opaque : kDnaCcOpaqueBits;
  // reserved
  uint64_t reserved_1 : 64;
  uint64_t reserved_2 : 57;
  // gen
  uint8_t gen_bit : 1;
};

struct __attribute__((packed)) Response_DNA_A {
  // cid
  uint32_t connection_id : kConnectionIdBits;
  // randomize_path
  bool randomize_path : 1;
  // eack_own bits
  uint8_t eack_own : kEackOwnBits;
  // cc_meta
  uint32_t cc_metadata : kCcMetadataBits;
  // cwnd_frac and cwnd concatenated
  uint32_t fabric_congestion_window : kFabricCongestionWindowBits;
  // ipg
  uint32_t inter_packet_gap : kInterPacketGapBits;
  // ncwnd
  uint32_t nic_congestion_window : kDnaNicCongestionWindowBits;
  // retx_to
  uint32_t retransmit_timeout : kTimeBits;
  // window_guard
  uint32_t fabric_window_time_marker : kTimeBits;
  // ncwnd_guard
  uint32_t nic_window_time_marker : kTimeBits;
  // ncwnd_dir
  falcon::WindowDirection nic_window_direction : kWindowDirectionBits;
  // event_q_sel
  uint8_t event_queue_select : kEventQueueSelectBits;
  // delay_sel
  falcon::DelaySelect delay_select : kDelaySelectBits;
  // delay_base
  uint32_t base_delay : kTimeBits;
  // delay_state
  uint32_t delay_state : kTimeBits;
  // smoothed_rtt
  uint32_t rtt_state : kTimeBits;
  // cc_opaque
  // bit 0 being used by HW-RUE for randomizing paths
  uint8_t cc_opaque : kDnaCcOpaqueBits;
  // reserved
  uint64_t padding_1 : 64;
  uint64_t padding_2 : 64;
  uint64_t padding_3 : 64;
  uint64_t padding_4 : 64;
  uint64_t padding_5 : 4;
};

static_assert(sizeof(Event_DNA_A) == ABSL_CACHELINE_SIZE,
              "Event is not one cache line :(");
static_assert(sizeof(Response_DNA_A) == ABSL_CACHELINE_SIZE,
              "Response is not one cache line :(");

}  // namespace falcon_rue

#endif  // ISEKAI_HOST_FALCON_RUE_FORMAT_DNA_A_H_
