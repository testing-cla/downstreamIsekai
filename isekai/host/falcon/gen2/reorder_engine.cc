#include "isekai/host/falcon/gen2/reorder_engine.h"

namespace isekai {

Gen2ReorderEngine::Gen2ReorderEngine(FalconModelInterface* falcon)
    : ProtocolBufferReorderEngine(falcon) {}

}  // namespace isekai
