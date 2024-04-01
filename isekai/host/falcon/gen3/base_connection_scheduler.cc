#include "isekai/host/falcon/gen3/base_connection_scheduler.h"

namespace isekai {

Gen3BaseConnectionScheduler::Gen3BaseConnectionScheduler(
    FalconModelInterface* falcon)
    : Gen2BaseConnectionScheduler(falcon) {}

// Adds a packet to the relevant queue for transmitting over the network and
// returns the queue to which the packet was added.
PacketTypeQueue Gen3BaseConnectionScheduler::AddPacketToQueue(
    uint32_t scid, uint32_t rsn, falcon::PacketType type) {
  return Gen2BaseConnectionScheduler::AddPacketToQueue(scid, rsn, type);
}

}  // namespace isekai
