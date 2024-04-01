#include "isekai/host/falcon/gen2/inter_host_rx_scheduler.h"

#include <cstdint>

#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_inter_host_rx_scheduler.h"

namespace isekai {

Gen2InterHostRxScheduler::Gen2InterHostRxScheduler(
    FalconModelInterface* falcon, const uint8_t number_of_hosts)
    : ProtocolInterHostRxScheduler(falcon, number_of_hosts) {}

}  // namespace isekai
