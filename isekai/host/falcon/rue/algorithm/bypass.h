#ifndef ISEKAI_HOST_FALCON_RUE_ALGORITHM_BYPASS_H_
#define ISEKAI_HOST_FALCON_RUE_ALGORITHM_BYPASS_H_

#include <cstdint>
#include <memory>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/rue/algorithm/algorithm.pb.h"
#include "isekai/host/falcon/rue/algorithm/bypass.pb.h"
#include "isekai/host/falcon/rue/algorithm/swift.h"
#include "isekai/host/falcon/rue/format.h"
#include "isekai/host/falcon/rue/format_bna.h"

namespace isekai {
namespace rue {

// This is a simple RUE algorithm module that only keeps the
// fabric congestion window, NIC congestion window, and the inter packet gap
// at their default values.

template <typename EventT, typename ResponseT>
class Bypass {
 public:
  // The algorithm class has to expose the template arguments as EventType and
  // ResponseType so that the StatefulAlgorithm class can internally deduce
  // them.
  using EventType = EventT;
  using ResponseType = ResponseT;

  ~Bypass() = default;
  Bypass() = default;
  Bypass(const Bypass&) = delete;
  Bypass& operator=(const Bypass&) = delete;

  template <typename ChildT = Bypass>
  static absl::StatusOr<std::unique_ptr<ChildT>> Create(
      const BypassConfiguration& config);

  // Processes an event and generates the response.
  void Process(const EventT& event, ResponseT& response, uint32_t now) const;
  // Processes an event for a multipath connection and generates the response.
  void ProcessMultipath(const EventT& event, ResponseT& response,
                        RueConnectionState& rue_connection_state,
                        uint32_t now) {
    // For bypass, there is no difference between Process() and
    // ProcessMultipath().
    Process(event, response, now);
  }
  absl::Status InstallAlgorithmProfile(int profile_index,
                                       AlgorithmConfiguration profile);
  absl::Status UninstallAlgorithmProfile(int profile_index);
  void PickProfile(const EventT& event) {}
};

template <typename EventT, typename ResponseT>
template <typename ChildT>
absl::StatusOr<std::unique_ptr<ChildT>> Bypass<EventT, ResponseT>::Create(
    const BypassConfiguration& config) {
  (void)config;  // UNUSED
  return std::make_unique<ChildT>();
}

template <typename EventT, typename ResponseT>
void Bypass<EventT, ResponseT>::Process(const EventT& event,
                                        ResponseT& response,
                                        uint32_t now) const {
  // Writes the values to the response
  falcon_rue::SetResponse(
      /*connection_id=*/event.connection_id,
      /*randomize_path=*/false,
      /*cc_metadata=*/event.cc_metadata,
      /*fabric_congestion_window=*/event.fabric_congestion_window,
      /*inter_packet_gap=*/event.inter_packet_gap,
      /*nic_congestion_window=*/event.nic_congestion_window,
      /*retransmit_timeout=*/event.retransmit_timeout,
      /*fabric_window_time_marker=*/0,
      /*nic_window_time_marker=*/0,
      /*nic_window_direction=*/falcon::WindowDirection::kDecrease,
      /*event_queue_select=*/event.event_queue_select,
      /*delay_select=*/event.delay_select,
      /*base_delay=*/event.base_delay,
      /*delay_state=*/event.delay_state,
      /*rtt_state=*/event.rtt_state,
      /*cc_opaque=*/event.cc_opaque,
      /*response=*/response);
}

// Template specialization for BNA.
template <>
void Bypass<falcon_rue::Event_BNA, falcon_rue::Response_BNA>::Process(
    const falcon_rue::Event_BNA& event, falcon_rue::Response_BNA& response,
    uint32_t now) const;

typedef Bypass<falcon_rue::Event, falcon_rue::Response> BypassDna;
typedef Bypass<falcon_rue::Event_BNA, falcon_rue::Response_BNA> BypassBna;

}  // namespace rue
}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_RUE_ALGORITHM_BYPASS_H_
