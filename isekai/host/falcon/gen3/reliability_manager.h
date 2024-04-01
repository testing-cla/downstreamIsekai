#ifndef ISEKAI_HOST_FALCON_GEN3_RELIABILITY_MANAGER_H_
#define ISEKAI_HOST_FALCON_GEN3_RELIABILITY_MANAGER_H_

#include "isekai/host/falcon/gen2/reliability_manager.h"

namespace isekai {

class Gen3ReliabilityManager : public Gen2ReliabilityManager {
 public:
  explicit Gen3ReliabilityManager(FalconModelInterface* falcon);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_RELIABILITY_MANAGER_H_
