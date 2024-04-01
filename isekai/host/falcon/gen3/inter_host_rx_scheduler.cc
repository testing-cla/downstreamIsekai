#include "isekai/host/falcon/gen3/inter_host_rx_scheduler.h"

namespace isekai {

Gen3InterHostRxScheduler::Gen3InterHostRxScheduler(
    FalconModelInterface* falcon, const uint8_t number_of_hosts)
    : Gen2InterHostRxScheduler(falcon, number_of_hosts) {}

}  // namespace isekai
