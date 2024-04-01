#include "isekai/host/falcon/gen2/arbiter.h"

namespace isekai {

Gen2Arbiter::Gen2Arbiter(FalconModelInterface* falcon)
    : ProtocolRoundRobinArbiter(falcon) {}

}  // namespace isekai
