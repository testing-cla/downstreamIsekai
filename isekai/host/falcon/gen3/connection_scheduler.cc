#include "isekai/host/falcon/gen3/connection_scheduler.h"

#include <memory>
#include <optional>
#include <utility>

#include "absl/log/check.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/gen2/connection_scheduler.h"
#include "isekai/host/falcon/gen3/falcon_utils.h"

namespace isekai {

Gen3ConnectionScheduler::Gen3ConnectionScheduler(FalconModelInterface* falcon)
    : Gen2ConnectionScheduler(falcon) {}

// Initializes the intra connection scheduling queues.
absl::Status Gen3ConnectionScheduler::InitConnectionSchedulerQueues(
    uint32_t scid) {
  return Gen2ConnectionScheduler::InitConnectionSchedulerQueues(scid);
}

// Schedules the packet with the selected packet type and connection bundle.
bool Gen3ConnectionScheduler::AttemptPacketScheduling(
    uint32_t scid, const WorkId& work_id,
    absl::StatusOr<PacketTypeQueue> candidate_packet_type_queue) {
  bool is_packet_scheduled = Gen2ConnectionScheduler::AttemptPacketScheduling(
      scid, work_id, candidate_packet_type_queue);
  if (is_packet_scheduled) {
    return true;
  }
  return false;
}

void Gen3ConnectionScheduler::UpdatePolicyStates(uint32_t scid,
                                                 PacketTypeQueue queue_type) {
  Gen2ConnectionScheduler::UpdatePolicyStates(scid, queue_type);
}
}  // namespace isekai
