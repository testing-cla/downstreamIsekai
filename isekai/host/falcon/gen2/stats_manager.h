#ifndef ISEKAI_HOST_FALCON_GEN2_STATS_MANAGER_H_
#define ISEKAI_HOST_FALCON_GEN2_STATS_MANAGER_H_

#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/stats_manager.h"

namespace isekai {

class Gen2StatsManager : public FalconStatsManager {
 public:
  explicit Gen2StatsManager(FalconModelInterface* falcon);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_STATS_MANAGER_H_
