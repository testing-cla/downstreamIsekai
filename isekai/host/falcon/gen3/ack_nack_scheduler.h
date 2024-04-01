#ifndef ISEKAI_HOST_FALCON_GEN3_ACK_NACK_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_GEN3_ACK_NACK_SCHEDULER_H_

#include "isekai/host/falcon/gen2/ack_nack_scheduler.h"

namespace isekai {

class Gen3AckNackScheduler : public Gen2AckNackScheduler {
 public:
  explicit Gen3AckNackScheduler(FalconModelInterface* falcon);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_ACK_NACK_SCHEDULER_H_
