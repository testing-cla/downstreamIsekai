#ifndef ISEKAI_HOST_FALCON_GEN2_BASE_CONNECTION_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_GEN2_BASE_CONNECTION_SCHEDULER_H_

#include "isekai/host/falcon/falcon_protocol_base_connection_scheduler.h"

namespace isekai {

class Gen2BaseConnectionScheduler : public ProtocolBaseConnectionScheduler {
 public:
  explicit Gen2BaseConnectionScheduler(FalconModelInterface* falcon);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_BASE_CONNECTION_SCHEDULER_H_
