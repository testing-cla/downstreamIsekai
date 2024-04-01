#ifndef ISEKAI_HOST_FALCON_EVENT_RESPONSE_FORMAT_ADAPTER_H_
#define ISEKAI_HOST_FALCON_EVENT_RESPONSE_FORMAT_ADAPTER_H_

#include <cstdint>
#include <memory>

#include "isekai/common/config.pb.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_rate_update_engine_adapter.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/rue/algorithm/swift.pb.h"
#include "isekai/host/falcon/rue/format_bna.h"

namespace isekai {

// This class is responsible for handling format changes in the RUE event and
// response formats. Any function in the RUE module that access fields in the
// response or event that are not shared across event/response formats should be
// managed by this EventResponseFormatAdapter class (e.g., functions filling the
// events to be sent to algorithm, the function which handles the RUE
// response from the algorithm, or functions which return the RUE adapters for
// different algorithms).
template <typename EventT, typename ResponseT>
class EventResponseFormatAdapter {
 public:
  explicit EventResponseFormatAdapter(const FalconModelInterface* falcon) {
    falcon_ = falcon;
  }

  // Changes the connection state as a result of a received RUE response.
  void UpdateConnectionStateFromResponse(ConnectionState* connection_state,
                                         const ResponseT* response) const;

  // Returns true if the response is signaling the datapath to repath the
  // connection.
  bool IsRandomizePath(const ResponseT* response) const;

  // Fills an RTO event before it can be sent to the algorithm.
  void FillTimeoutRetransmittedEvent(
      EventT& event, const RueKey* rue_key, const Packet* packet,
      const ConnectionState::CongestionControlMetadata& ccmeta,
      uint8_t retransmit_count) const;

  void FillNackEvent(EventT& event, const RueKey* rue_key, const Packet* packet,
                     const ConnectionState::CongestionControlMetadata& ccmeta,
                     uint32_t num_packets_acked) const;

  void FillExplicitAckEvent(
      EventT& event, const RueKey* rue_key, const Packet* packet,
      const ConnectionState::CongestionControlMetadata& ccmeta,
      uint32_t num_packets_acked, bool eack, bool eack_drop) const;

  // Returns the RUE adapter object instantiated with the Swift algorithm.
  std::unique_ptr<RueAdapterInterface<EventT, ResponseT>> GetSwiftRueAdapter(
      const ::isekai::rue::SwiftConfiguration& configuration,
      uint32_t initial_fcwnd_fixed) const;

 private:
  const FalconModelInterface* falcon_;
};

// BNA template specializations.
template <>
void EventResponseFormatAdapter<falcon_rue::Event_BNA,
                                falcon_rue::Response_BNA>::
    UpdateConnectionStateFromResponse(
        ConnectionState* connection_state,
        const falcon_rue::Response_BNA* response) const;

template <>
bool EventResponseFormatAdapter<falcon_rue::Event_BNA,
                                falcon_rue::Response_BNA>::
    IsRandomizePath(const falcon_rue::Response_BNA* response) const;

template <>
void EventResponseFormatAdapter<falcon_rue::Event_BNA,
                                falcon_rue::Response_BNA>::
    FillTimeoutRetransmittedEvent(
        falcon_rue::Event_BNA& event, const RueKey* rue_key,
        const Packet* packet,
        const ConnectionState::CongestionControlMetadata& ccmeta,
        uint8_t retransmit_count) const;

template <>
void EventResponseFormatAdapter<falcon_rue::Event_BNA,
                                falcon_rue::Response_BNA>::
    FillNackEvent(falcon_rue::Event_BNA& event, const RueKey* rue_key,
                  const Packet* packet,
                  const ConnectionState::CongestionControlMetadata& ccmeta,
                  uint32_t num_packets_acked) const;

template <>
void EventResponseFormatAdapter<falcon_rue::Event_BNA,
                                falcon_rue::Response_BNA>::
    FillExplicitAckEvent(
        falcon_rue::Event_BNA& event, const RueKey* rue_key,
        const Packet* packet,
        const ConnectionState::CongestionControlMetadata& ccmeta,
        uint32_t num_packets_acked, bool eack, bool eack_drop) const;

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_EVENT_RESPONSE_FORMAT_ADAPTER_H_
