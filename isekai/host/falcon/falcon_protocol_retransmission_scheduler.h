#ifndef ISEKAI_HOST_FALCON_FALCON_PROTOCOL_RETRANSMISSION_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_FALCON_PROTOCOL_RETRANSMISSION_SCHEDULER_H_

#include <cstdint>
#include <memory>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"

namespace isekai {

// Reflects the retransmission scheduler of the FALCON block that consists of a
// hierarchical scheduling policies within and across connections.
class ProtocolRetransmissionScheduler : public Scheduler {
 public:
  explicit ProtocolRetransmissionScheduler(FalconModelInterface* falcon);
  // Initializes the intra connection scheduling queues.
  absl::Status InitConnectionSchedulerQueues(uint32_t scid) override;
  // Add a transaction to the relevant queue for transmitting over the network.
  absl::Status EnqueuePacket(uint32_t scid, uint32_t rsn,
                             falcon::PacketType type) override;
  // Add a transaction to the relevant queue for transmitting over the network.
  absl::Status DequeuePacket(uint32_t scid, uint32_t rsn,
                             falcon::PacketType type) override;
  // Returns true if the retransmission scheduler has outstanding work.
  bool HasWork() override;
  // Performs one unit of work from the retransmission scheduler.
  bool ScheduleWork() override;
  uint32_t GetConnectionQueueLength(uint32_t scid) override { return 0; };
  uint64_t GetQueueTypeBasedOutstandingPacketCount(
      PacketTypeQueue queue_type) override {
    return 0;
  };

 private:
  // Schedules the given packet by handing it of to the downstream blocks.
  void SchedulePacket(uint32_t scid, const RetransmissionWorkId& work_id);

  FalconModelInterface* const falcon_;

  // Stores the mapping between the connection ID and the corresponding intra
  // connection queues corresponding to the various transactions.
  absl::flat_hash_map<uint32_t, RetransmissionSchedulerQueues>
      connection_queues_;
  // Represent the intra and inter connection scheduling policies adopted by the
  // connection scheduler.
  std::unique_ptr<IntraConnectionSchedulingPolicy<WindowTypeQueue>>
      intra_connection_policy_;
  std::unique_ptr<InterConnectionSchedulingPolicy> inter_connection_policy_;
};
};  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_PROTOCOL_RETRANSMISSION_SCHEDULER_H_
