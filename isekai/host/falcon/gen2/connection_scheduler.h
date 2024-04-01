#ifndef ISEKAI_HOST_FALCON_GEN2_CONNECTION_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_GEN2_CONNECTION_SCHEDULER_H_

#include "isekai/host/falcon/falcon_protocol_packet_type_based_connection_scheduler.h"

namespace isekai {

class Gen2ConnectionScheduler
    : public ProtocolPacketTypeBasedConnectionScheduler {
 public:
  explicit Gen2ConnectionScheduler(FalconModelInterface* falcon);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_CONNECTION_SCHEDULER_H_
