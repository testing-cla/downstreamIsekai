#ifndef ISEKAI_HOST_FALCON_FALCON_PROTOCOL_ACK_NACK_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_FALCON_PROTOCOL_ACK_NACK_SCHEDULER_H_

#include <memory>
#include <queue>

#include "absl/status/status.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"

namespace isekai {

// Global FIFO for Ack/Nack messages. It is implemented by extending Scheduler
// class, such that the Ack/Nack messages can be arbitrated together with
// initial transmissions and retransmissions/resyncs in an unified manner.
class ProtocolAckNackScheduler : public Scheduler {
 public:
  explicit ProtocolAckNackScheduler(FalconModelInterface* falcon)
      : falcon_(falcon) {}
  // Initializes the intra connection scheduling queues.
  absl::Status InitConnectionSchedulerQueues(uint32_t scid) override;
  // Add a transaction to the relevant queue for transmitting over the network.
  absl::Status EnqueuePacket(uint32_t scid, uint32_t rsn,
                             falcon::PacketType type) override;
  // Remove a transaction to the relevant queue for transmitting over the
  // network.
  absl::Status DequeuePacket(uint32_t scid, uint32_t rsn,
                             falcon::PacketType type) override;
  // Enqueues the outgoing Ack/Nack packet.
  absl::Status EnqueuePacket(std::unique_ptr<Packet> ack_nack_packet);
  // Returns true if we have outstanding Ack/Nack packets.
  bool HasWork() override;
  // Performs one unit of work from the Ack/Nack scheduler.
  bool ScheduleWork() override;
  // Return the queue length of an inter-connection queue.
  uint32_t GetConnectionQueueLength(uint32_t scid) override { return 0; };
  uint64_t GetQueueTypeBasedOutstandingPacketCount(
      PacketTypeQueue queue_type) override {
    return 0;
  };

 private:
  std::queue<std::unique_ptr<Packet>> ack_nack_queue_;
  FalconModelInterface* const falcon_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_PROTOCOL_ACK_NACK_SCHEDULER_H_
