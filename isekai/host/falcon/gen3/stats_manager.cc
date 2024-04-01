#include "isekai/host/falcon/gen3/stats_manager.h"

#include "isekai/host/falcon/gen3/falcon_utils.h"

namespace isekai {

Gen3StatsManager::Gen3StatsManager(FalconModelInterface* falcon)
    : Gen2StatsManager(falcon) {}

void Gen3StatsManager::UpdateUlpRxCounters(Packet::Rdma::Opcode opcode,
                                           uint32_t cid) {
  Gen2StatsManager::UpdateUlpRxCounters(opcode, cid);
}

}  // namespace isekai
