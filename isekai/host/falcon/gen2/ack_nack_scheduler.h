#ifndef ISEKAI_HOST_FALCON_GEN2_ACK_NACK_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_GEN2_ACK_NACK_SCHEDULER_H_

#include "isekai/host/falcon/falcon_protocol_ack_nack_scheduler.h"

namespace isekai {

class Gen2AckNackScheduler : public ProtocolAckNackScheduler {
 public:
  explicit Gen2AckNackScheduler(FalconModelInterface* falcon);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_ACK_NACK_SCHEDULER_H_
