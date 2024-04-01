#include "isekai/host/falcon/gen3/multipath_round_robin_scheduling_policy.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "glog/logging.h"

namespace isekai {

absl::Status Gen3MultipathRoundRobinPolicy::InitConnection(uint32_t cid,
                                                           uint32_t cbid) {
  std::vector<uint32_t> &cid_list = cid_list_[cbid];
  if (std::find(cid_list.begin(), cid_list.end(), cid) != cid_list.end()) {
    LOG(FATAL) << "Duplicate source connection bundle ID.";
  }
  cid_list.push_back(cid);
  current_connection_indices_[cbid] = 0;
  return absl::OkStatus();
}

absl::StatusOr<uint32_t> Gen3MultipathRoundRobinPolicy::SelectConnection(
    uint32_t cbid) {
  std::vector<uint32_t> &cid_list = cid_list_[cbid];
  uint32_t &current_connection_index = current_connection_indices_[cbid];

  if (cid_list.empty()) LOG(FATAL) << "No connection found for selection.";

  uint32_t selected_cid = cid_list[current_connection_index];
  current_connection_index = (current_connection_index + 1) % cid_list.size();
  return selected_cid;
}

}  // namespace isekai
