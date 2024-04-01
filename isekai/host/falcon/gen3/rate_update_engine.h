#ifndef ISEKAI_HOST_FALCON_GEN3_RATE_UPDATE_ENGINE_H_
#define ISEKAI_HOST_FALCON_GEN3_RATE_UPDATE_ENGINE_H_

#include "isekai/host/falcon/gen2/rate_update_engine.h"

namespace isekai {

class Gen3RateUpdateEngine : public Gen2RateUpdateEngine {
 public:
  explicit Gen3RateUpdateEngine(FalconModelInterface* falcon);
  friend class Gen3RateUpdateAdapterTestPeer;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_RATE_UPDATE_ENGINE_H_
