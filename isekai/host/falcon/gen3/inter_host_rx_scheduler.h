#ifndef ISEKAI_HOST_FALCON_GEN3_INTER_HOST_RX_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_GEN3_INTER_HOST_RX_SCHEDULER_H_

#include "isekai/host/falcon/gen2/inter_host_rx_scheduler.h"

namespace isekai {

class Gen3InterHostRxScheduler : public Gen2InterHostRxScheduler {
 public:
  explicit Gen3InterHostRxScheduler(FalconModelInterface* falcon,
                                    const uint8_t number_of_hosts);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_INTER_HOST_RX_SCHEDULER_H_
