#ifndef ISEKAI_HOST_FALCON_GEN3_BASE_CONNECTION_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_GEN3_BASE_CONNECTION_SCHEDULER_H_

#include "isekai/host/falcon/gen2/base_connection_scheduler.h"

namespace isekai {

class Gen3BaseConnectionScheduler : public Gen2BaseConnectionScheduler {
 public:
  explicit Gen3BaseConnectionScheduler(FalconModelInterface* falcon);

  // Adds a packet to the relevant queue for transmitting over the network and
  // returns the queue to which the packet was added.
  PacketTypeQueue AddPacketToQueue(uint32_t scid, uint32_t rsn,
                                   falcon::PacketType type) override;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_BASE_CONNECTION_SCHEDULER_H_
