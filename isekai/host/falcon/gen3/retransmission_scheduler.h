#ifndef ISEKAI_HOST_FALCON_GEN3_RETRANSMISSION_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_GEN3_RETRANSMISSION_SCHEDULER_H_

#include "isekai/host/falcon/gen2/retransmission_scheduler.h"

namespace isekai {

class Gen3RetransmissionScheduler : public Gen2RetransmissionScheduler {
 public:
  explicit Gen3RetransmissionScheduler(FalconModelInterface* falcon);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_RETRANSMISSION_SCHEDULER_H_
