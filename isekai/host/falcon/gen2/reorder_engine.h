#ifndef ISEKAI_HOST_FALCON_GEN2_REORDER_ENGINE_H_
#define ISEKAI_HOST_FALCON_GEN2_REORDER_ENGINE_H_

#include "isekai/host/falcon/falcon_protocol_buffer_reorder_engine.h"

namespace isekai {

class Gen2ReorderEngine : public ProtocolBufferReorderEngine {
 public:
  explicit Gen2ReorderEngine(FalconModelInterface* falcon);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_REORDER_ENGINE_H_
