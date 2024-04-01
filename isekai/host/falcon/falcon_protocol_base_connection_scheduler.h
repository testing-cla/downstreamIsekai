#ifndef ISEKAI_HOST_FALCON_FALCON_PROTOCOL_BASE_CONNECTION_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_FALCON_PROTOCOL_BASE_CONNECTION_SCHEDULER_H_

#include <cstdint>
#include <memory>
#include <string_view>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"

namespace isekai {

// Reflects the connection scheduler of the FALCON block that consists of a
// hierarchical scheduling policies within and across connections.
class ProtocolBaseConnectionScheduler : public Scheduler {
 public:
  explicit ProtocolBaseConnectionScheduler(FalconModelInterface* falcon);
  // Initializes the intra connection scheduling queues.
  absl::Status InitConnectionSchedulerQueues(uint32_t scid) override;
  // Add a packet to the relevant queue for transmitting over the network.
  absl::Status DequeuePacket(uint32_t scid, uint32_t rsn,
                             falcon::PacketType type) override {
    return absl::OkStatus();
  };

  // Return the queue length of an inter-connection queue.
  uint32_t GetConnectionQueueLength(uint32_t scid) override;
  // Returns the number of outstanding packets corresponding to a packet queue
  // type.
  uint64_t GetQueueTypeBasedOutstandingPacketCount(
      PacketTypeQueue queue_type) override;

 private:
  void RecordRsnDiffMeasure(uint32_t scid, const WorkId& work_id);
  uint32_t last_push_unsolicited_data_rsn = 0;
  bool sent_out_push_unsolicited_data = false;

 protected:
  // Add a packet to the relevant queue for transmitting over the network and
  // returns the queue to which the packet was added.
  virtual PacketTypeQueue AddPacketToQueue(uint32_t scid, uint32_t rsn,
                                           falcon::PacketType type);
  // Schedules the given packet by handing it off to the downstream blocks.
  void SchedulePacket(uint32_t scid, const WorkId& work_id);
  // Checks if a packet meets its congestion window and admission criteria.
  bool CanSend(uint32_t scid, uint32_t rsn, falcon::PacketType type);
  // Checks if a packet meets its op rate criteria.
  bool MeetsOpRateCriteria(uint32_t scid);
  // Handles the phantom request in the connection scheduler.
  virtual void HandlePhantomRequest(uint32_t scid, const WorkId& work_id,
                                    PacketTypeQueue candidate_queue_type);
  void UpdateOutstandingPacketCount(PacketTypeQueue queue_type, bool is_dequed);
  // Uses or Fills the token bucket tokens per connection.
  bool UseOrFillOpRateTokenBucketTokens(uint32_t scid);
  // Update queuing delay stats, including those of the max-delayed packets.
  void UpdateQueuingDelayStatistics(
      PacketMetadata::PacketQueuingDelayMetadata* queuing_delay_metadata,
      uint32_t scid, bool is_ordered_connection,
      falcon::PacketType packet_type);

  FalconModelInterface* const falcon_;

  // Stores the mapping between the connection ID and the corresponding intra
  // connection queues corresponding to the various packets.
  absl::flat_hash_map<uint32_t, ConnectionSchedulerQueues> connection_queues_;
  absl::flat_hash_map<PacketTypeQueue, uint64_t> packet_queue_wise_pkt_count_;
  absl::flat_hash_map<falcon::PacketType, uint64_t> packet_type_wise_tx_count_;
  bool collect_max_delay_stats_ = false;
  absl::flat_hash_map<falcon::PacketType, absl::Duration>
      packet_type_wise_max_queueing_delay_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_PROTOCOL_BASE_CONNECTION_SCHEDULER_H_
