#ifndef ISEKAI_HOST_FALCON_GEN3_CONNECTION_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_GEN3_CONNECTION_SCHEDULER_H_

#include <cstdint>

#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"
#include "isekai/host/falcon/gen2/connection_scheduler.h"

namespace isekai {

class Gen3ConnectionScheduler : public Gen2ConnectionScheduler {
 public:
  explicit Gen3ConnectionScheduler(FalconModelInterface* falcon);
  // Initializes the intra connection scheduling queues.
  absl::Status InitConnectionSchedulerQueues(uint32_t scid) override;
  bool AttemptPacketScheduling(
      uint32_t scid, const WorkId& work_id,
      absl::StatusOr<PacketTypeQueue> candidate_packet_type_queue) override;

  void UpdatePolicyStates(uint32_t scid, PacketTypeQueue queue_type) override;

 private:
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_CONNECTION_SCHEDULER_H_
