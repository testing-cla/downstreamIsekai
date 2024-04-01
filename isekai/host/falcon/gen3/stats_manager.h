#ifndef ISEKAI_HOST_FALCON_GEN3_STATS_MANAGER_H_
#define ISEKAI_HOST_FALCON_GEN3_STATS_MANAGER_H_

#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/gen2/stats_manager.h"

namespace isekai {

class Gen3StatsManager : public Gen2StatsManager {
 public:
  explicit Gen3StatsManager(FalconModelInterface* falcon);
  void UpdateUlpRxCounters(Packet::Rdma::Opcode opcode, uint32_t cid) override;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_STATS_MANAGER_H_
