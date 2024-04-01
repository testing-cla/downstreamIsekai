#include "isekai/host/falcon/gen2/connection_scheduler.h"

namespace isekai {

Gen2ConnectionScheduler::Gen2ConnectionScheduler(FalconModelInterface* falcon)
    : ProtocolPacketTypeBasedConnectionScheduler(falcon) {}

}  // namespace isekai
