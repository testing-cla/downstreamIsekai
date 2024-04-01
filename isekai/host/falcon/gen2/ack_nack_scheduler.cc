#include "isekai/host/falcon/gen2/ack_nack_scheduler.h"

namespace isekai {

Gen2AckNackScheduler::Gen2AckNackScheduler(FalconModelInterface* falcon)
    : ProtocolAckNackScheduler(falcon) {}

}  // namespace isekai
