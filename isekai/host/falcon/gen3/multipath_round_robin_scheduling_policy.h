#ifndef ISEKAI_HOST_FALCON_GEN3_MULTIPATHING_ROUND_ROBIN_SCHEDULING_POLICY_H_
#define ISEKAI_HOST_FALCON_GEN3_MULTIPATHING_ROUND_ROBIN_SCHEDULING_POLICY_H_

#include "isekai/host/falcon/falcon_component_interfaces.h"

namespace isekai {

class Gen3MultipathRoundRobinPolicy : public MultipathSchedulingPolicy {
 public:
  Gen3MultipathRoundRobinPolicy() {}
  absl::Status InitConnection(uint32_t cid, uint32_t cbid) override;

  // Selects connection based on the round-robin scheduling policy.
  absl::StatusOr<uint32_t> SelectConnection(uint32_t cbid) override;

 private:
  // List of all associated CIDs indexed by the connection bundle.
  absl::flat_hash_map</*cbid*/ uint32_t, std::vector<uint32_t>> cid_list_;
  // Tracks the current round-robin connection index for each connection
  // bundle.
  absl::flat_hash_map</*cbid*/ uint32_t, uint32_t> current_connection_indices_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_MULTIPATHING_ROUND_ROBIN_SCHEDULING_POLICY_H_
