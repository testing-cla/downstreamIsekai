#ifndef ISEKAI_HOST_FALCON_GEN2_RETRANSMISSION_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_GEN2_RETRANSMISSION_SCHEDULER_H_

#include "isekai/host/falcon/falcon_protocol_retransmission_scheduler.h"

namespace isekai {

class Gen2RetransmissionScheduler : public ProtocolRetransmissionScheduler {
 public:
  explicit Gen2RetransmissionScheduler(FalconModelInterface* falcon);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_RETRANSMISSION_SCHEDULER_H_
