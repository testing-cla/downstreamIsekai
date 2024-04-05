#ifndef ISEKAI_HOST_FALCON_FALCON_PROTOCOL_FAST_RETRANSMISSION_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_FALCON_PROTOCOL_FAST_RETRANSMISSION_SCHEDULER_H_

#include <cstdint>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"
#include "isekai/host/falcon/weighted_round_robin_policy.h"

namespace isekai {

// Event-based retransmission scheduler that only schedules work when it becomes
// eligibile for retransmission.
class ProtocolFastRetransmissionScheduler : public FastScheduler {
 public:
  explicit ProtocolFastRetransmissionScheduler(FalconModelInterface* falcon);
  // Initializes the connection scheduling queues.
  absl::Status InitConnectionSchedulerQueues(uint32_t scid) override;
  // Add a transaction to the relevant queue for transmitting over the network.
  absl::Status EnqueuePacket(uint32_t scid, uint32_t rsn,
                             falcon::PacketType type) override;
  // Remove a transaction from the relevant queue.
  absl::Status DequeuePacket(uint32_t scid, uint32_t rsn,
                             falcon::PacketType type) override;
  // Returns true if the retransmission scheduler has outstanding work.
  bool HasWork() override;
  // Performs one unit of work from the retransmission scheduler.
  bool ScheduleWork() override;
  // Recompute eligibility of connection queues upon gating variable updates.
  void RecomputeEligibility(uint32_t scid) override;
  uint32_t GetConnectionQueueLength(uint32_t scid) override { return 0; };
  uint64_t GetQueueTypeBasedOutstandingPacketCount(
      PacketTypeQueue queue_type) override {
    return 0;
  };

 private:
  // Schedules the given packet by handing it of to the downstream blocks.
  void SchedulePacket(uint32_t scid, WindowTypeQueue queue_type,
                      RetransmissionWorkId& work_id);

  // Check if a packet meets retransmission criteria.
  bool CanSend(uint32_t scid, uint32_t psn, falcon::PacketType type);

  // Stores the mapping between the connection ID and the corresponding intra
  // connection queues corresponding to the various transactions.
  absl::flat_hash_map<uint32_t, RetransmissionSchedulerQueues>
      connection_queues_;

  // Stores the round robin policy over all connections, along with which window
  // should be serviced next within each connection.
  WeightedRoundRobinPolicy<uint32_t> connection_policy_;
  absl::flat_hash_map<uint32_t, WindowTypeQueue> next_window_type_;

  FalconModelInterface* const falcon_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_PROTOCOL_FAST_RETRANSMISSION_SCHEDULER_H_
