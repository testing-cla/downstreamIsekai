#ifndef ISEKAI_HOST_FALCON_GEN2_ARBITER_H_
#define ISEKAI_HOST_FALCON_GEN2_ARBITER_H_

#include "isekai/host/falcon/falcon_protocol_round_robin_arbiter.h"

namespace isekai {

class Gen2Arbiter : public ProtocolRoundRobinArbiter {
 public:
  explicit Gen2Arbiter(FalconModelInterface* falcon);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_ARBITER_H_
