#ifndef ISEKAI_HOST_FALCON_GEN3_ARBITER_H_
#define ISEKAI_HOST_FALCON_GEN3_ARBITER_H_

#include "isekai/host/falcon/gen2/arbiter.h"

namespace isekai {

class Gen3Arbiter : public Gen2Arbiter {
 public:
  explicit Gen3Arbiter(FalconModelInterface* falcon);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_ARBITER_H_
