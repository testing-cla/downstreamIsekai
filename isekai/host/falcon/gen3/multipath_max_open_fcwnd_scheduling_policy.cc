#include "isekai/host/falcon/gen3/multipath_max_open_fcwnd_scheduling_policy.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/gen3/connection_scheduler.h"

namespace isekai {

Gen3MultipathMaxOpenFcwndPolicy::Gen3MultipathMaxOpenFcwndPolicy(
    FalconModelInterface *const falcon)
    : falcon_(falcon) {}

absl::Status Gen3MultipathMaxOpenFcwndPolicy::InitConnection(uint32_t cid,
                                                             uint32_t cbid) {
  std::vector<uint32_t> &cid_list = cid_list_[cbid];
  if (std::find(cid_list.begin(), cid_list.end(), cid) != cid_list.end()) {
    LOG(FATAL) << "Duplicate source connection bundle ID.";
  }
  cid_list.push_back(cid);
  previous_selected_connection[cbid] = cid_list[0];
  return absl::OkStatus();
}

absl::StatusOr<uint32_t> Gen3MultipathMaxOpenFcwndPolicy::SelectConnection(
    uint32_t cbid) {
  std::vector<uint32_t> &cid_list = cid_list_[cbid];

  int max_open_fcwnd;
  uint32_t selected_cid;

  auto scheduler = static_cast<Gen3ConnectionScheduler *>(
      falcon_->get_connection_scheduler());
  // Queries the scheduler to get the current candidate packet type for
  // obtaining the corresponding open fcwnd later.
  auto packet_type = scheduler->GetCandidatePacketType();

  // Prioritizes using the previous selected connection to avoid unnecessary
  // connection changing if there are multiple connections sharing the same max
  // open fcwnd value.
  selected_cid = previous_selected_connection[cbid];
  max_open_fcwnd = falcon_->get_packet_reliability_manager()->GetOpenFcwnd(
      selected_cid, packet_type);

  //  should keep track of the CID within the bundle that has the max-open-fcwnd
  //  to optimize scheduling performance.
  for (auto cid : cid_list) {
    int connection_open_cwnd =
        falcon_->get_packet_reliability_manager()->GetOpenFcwnd(cid,
                                                                packet_type);
    if (connection_open_cwnd > max_open_fcwnd) {
      max_open_fcwnd = connection_open_cwnd;
      selected_cid = cid;
    }
  }
  previous_selected_connection[cbid] = selected_cid;
  return selected_cid;
}

}  // namespace isekai
