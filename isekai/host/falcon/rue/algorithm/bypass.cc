#include "isekai/host/falcon/rue/algorithm/bypass.h"

#include <cmath>
#include <cstdint>

#include "absl/status/status.h"
#include "isekai/host/falcon/rue/algorithm/algorithm.pb.h"
#include "isekai/host/falcon/rue/algorithm/swift.h"
#include "isekai/host/falcon/rue/format.h"
#include "isekai/host/falcon/rue/format_bna.h"
#include "isekai/host/falcon/rue/format_dna_a.h"
#include "isekai/host/falcon/rue/format_dna_b.h"
#include "isekai/host/falcon/rue/format_dna_c.h"

namespace isekai {
namespace rue {

template <typename EventT, typename ResponseT>
absl::Status Bypass<EventT, ResponseT>::InstallAlgorithmProfile(
    int profile_index, AlgorithmConfiguration profile) {
  if (!profile.has_bypass()) {
    return absl::InvalidArgumentError("Not a bypass profile");
  }
  return absl::OkStatus();
}

template <typename EventT, typename ResponseT>
absl::Status Bypass<EventT, ResponseT>::UninstallAlgorithmProfile(
    int profile_index) {
  return absl::OkStatus();
}

template <>
void Bypass<falcon_rue::Event_BNA, falcon_rue::Response_BNA>::Process(
    const falcon_rue::Event_BNA& event, falcon_rue::Response_BNA& response,
    uint32_t now) const {
  constexpr double kFalconUnitTimeUs = 0.131072;
  const uint32_t kDefaultRetransmissionTimeout =
      std::round(1000 / kFalconUnitTimeUs);  // ~1ms.
  uint8_t flow_id = Swift<falcon_rue::Event_BNA,
                          falcon_rue::Response_BNA>::GetFlowIdFromEvent(event);
  // Writes the values to the response.
  falcon_rue::SetResponse(

      /*connection_id=*/event.connection_id,
      /*cc_metadata=*/event.cc_metadata,
      /*fabric_congestion_window=*/event.fabric_congestion_window,
      /*fabric_inter_packet_gap=*/0,
      /*nic_congestion_window=*/event.nic_congestion_window,
      /*retransmit_timeout=*/kDefaultRetransmissionTimeout,
      /*fabric_window_time_marker=*/0,
      /*nic_window_time_marker=*/event.nic_window_time_marker,
      /*event_queue_select=*/event.event_queue_select,
      /*delay_select=*/event.delay_select,
      /*base_delay=*/event.base_delay,
      /*delay_state=*/event.delay_state,
      /*rtt_state=*/event.rtt_state,
      /*cc_opaque=*/event.cc_opaque,
      /*plb_state=*/event.plb_state,
      /*alpha_request=*/0,
      /*alpha_response=*/0,
      /*nic_inter_packet_gap=*/0,
      /*flow_label_1=*/0,
      /*flow_label_2=*/0,
      /*flow_label_3=*/0,
      /*flow_label_4=*/0,
      /*flow_label_1_valid=*/false,
      /*flow_label_2_valid=*/false,
      /*flow_label_3_valid=*/false,
      /*flow_label_4_valid=*/false,
      /*flow_label_1_weight=*/1,
      /*flow_label_2_weight=*/1,
      /*flow_label_3_weight=*/1,
      /*flow_label_4_weight=*/1,
      /*wrr_restart_round=*/false,
      /*flow_id=*/flow_id,
      /*csig_enable=*/event.csig_enable,
      /*csig_select=*/0,
      /*ar_rate=*/0,
      /*response=*/response);
}

// Explicit template instantiations.
template class Bypass<falcon_rue::Event_DNA_A, falcon_rue::Response_DNA_A>;
template class Bypass<falcon_rue::Event_DNA_B, falcon_rue::Response_DNA_B>;
template class Bypass<falcon_rue::Event_DNA_C, falcon_rue::Response_DNA_C>;
template class Bypass<falcon_rue::Event_BNA, falcon_rue::Response_BNA>;

}  // namespace rue
}  // namespace isekai
