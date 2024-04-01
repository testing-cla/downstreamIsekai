#ifndef ISEKAI_HOST_FALCON_FALCON_RESOURCE_CREDITS_H_
#define ISEKAI_HOST_FALCON_FALCON_RESOURCE_CREDITS_H_

#include <cstdint>

#include "isekai/common/config.pb.h"

namespace isekai {

struct FalconResourceCredits {
  static FalconResourceCredits Create(
      const FalconConfig::ResourceCredits& config);

  struct TxPacketCredits {
    int32_t ulp_requests = 0;
    int32_t ulp_data = 0;
    int32_t network_requests = 0;
    // This field tracks the maximum packet credits ulp requests can consume and
    // is set when initializing the resource credits. This is required and used
    // only when oversubscription is enabled.
    int32_t max_ulp_requests = 0;
  } tx_packet_credits;
  struct TxBufferCredits {
    int32_t ulp_requests = 0;
    int32_t ulp_data = 0;
    int32_t network_requests = 0;
    // This field tracks the maximum buffer credits ulp requests can consume and
    // is set when initializing the resource credits. This is required and used
    // only when oversubscription is enabled.
    int32_t max_ulp_requests = 0;
  } tx_buffer_credits;
  struct RxPacketCredits {
    int32_t ulp_requests = 0;
    int32_t network_requests = 0;
  } rx_packet_credits;
  struct RxBufferCredits {
    int32_t ulp_requests = 0;
    int32_t network_requests = 0;
  } rx_buffer_credits;

  // When ULP pool oversubsciption is enabled, ulp_data can consume credits from
  // ulp_requests for tx_packet_credits and tx_buffer_credits.
  bool enable_ulp_pool_oversubscription = false;

  bool IsInitialized() const;
  bool operator<=(const FalconResourceCredits& rhs) const;
  bool operator==(const FalconResourceCredits& rhs) const;
  FalconResourceCredits& operator+=(const FalconResourceCredits& rhs);
  FalconResourceCredits& operator-=(const FalconResourceCredits& rhs);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_RESOURCE_CREDITS_H_
