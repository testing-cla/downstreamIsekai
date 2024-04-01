#ifndef ISEKAI_HOST_FALCON_GEN3_ACK_COALESCING_ENGINE_H_
#define ISEKAI_HOST_FALCON_GEN3_ACK_COALESCING_ENGINE_H_

#include "isekai/host/falcon/gen2/ack_coalescing_engine.h"

namespace isekai {

class Gen3AckCoalescingEngine : public Gen2AckCoalescingEngine {
 public:
  explicit Gen3AckCoalescingEngine(FalconModelInterface* falcon);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_ACK_COALESCING_ENGINE_H_
