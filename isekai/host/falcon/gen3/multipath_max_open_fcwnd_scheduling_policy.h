#ifndef ISEKAI_HOST_FALCON_GEN3_MULTIPATHING_MAX_OPEN_FCWND_SCHEDULING_POLICY_H_
#define ISEKAI_HOST_FALCON_GEN3_MULTIPATHING_MAX_OPEN_FCWND_SCHEDULING_POLICY_H_

#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/gen3/connection_scheduler.h"

namespace isekai {

class Gen3MultipathMaxOpenFcwndPolicy : public MultipathSchedulingPolicy {
 public:
  Gen3MultipathMaxOpenFcwndPolicy(FalconModelInterface* falcon);

  absl::Status InitConnection(uint32_t cid, uint32_t cbid) override;

  // Selects connection based on the max-open-fcwnd-first scheduling policy.
  absl::StatusOr<uint32_t> SelectConnection(uint32_t cbid) override;

  // List of all associated CIDs indexed by the connection bundle.
  absl::flat_hash_map</*cbid*/ uint32_t, std::vector<uint32_t>> cid_list_;
  // Keeps track of the last selected connection for each bundle.
  absl::flat_hash_map</*cbid*/ uint32_t, /*cid=*/uint32_t>
      previous_selected_connection;
  FalconModelInterface* falcon_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_MULTIPATHING_MAX_OPEN_FCWND_SCHEDULING_POLICY_H_
