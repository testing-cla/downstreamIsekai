#ifndef ISEKAI_HOST_FALCON_GEN2_INTER_HOST_RX_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_GEN2_INTER_HOST_RX_SCHEDULER_H_

#include "isekai/host/falcon/falcon_inter_host_rx_scheduler.h"

namespace isekai {

class Gen2InterHostRxScheduler : public ProtocolInterHostRxScheduler {
 public:
  explicit Gen2InterHostRxScheduler(FalconModelInterface* falcon,
                                    const uint8_t number_of_hosts);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_INTER_HOST_RX_SCHEDULER_H_
