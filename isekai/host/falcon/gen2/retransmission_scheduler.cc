#include "isekai/host/falcon/gen2/retransmission_scheduler.h"

namespace isekai {

Gen2RetransmissionScheduler::Gen2RetransmissionScheduler(
    FalconModelInterface* falcon)
    : ProtocolRetransmissionScheduler(falcon) {}

}  // namespace isekai
