#ifndef ISEKAI_HOST_FALCON_FALCON_PROTOCOL_PACKET_TYPE_BASED_CONNECTION_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_FALCON_PROTOCOL_PACKET_TYPE_BASED_CONNECTION_SCHEDULER_H_

#include <cstdint>
#include <memory>
#include <optional>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_protocol_base_connection_scheduler.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"

namespace isekai {

// Reflects the connection scheduler of the FALCON block that consists of a
// hierarchical scheduling policy that selects the packet type and then the
// connection.
class ProtocolPacketTypeBasedConnectionScheduler
    : public ProtocolBaseConnectionScheduler {
 public:
  explicit ProtocolPacketTypeBasedConnectionScheduler(
      FalconModelInterface* falcon);
  // Initializes the intra connection scheduling queues.
  absl::Status InitConnectionSchedulerQueues(uint32_t scid) override;
  // Add a packet to the relevant queue for transmitting over the network.
  absl::Status EnqueuePacket(uint32_t scid, uint32_t rsn,
                             falcon::PacketType type) override;
  // Removes a packet to the relevant queue for transmitting over the network.
  absl::Status DequeuePacket(uint32_t scid, uint32_t rsn,
                             falcon::PacketType type) override {
    //
    return absl::OkStatus();
  };
  // Returns true if the connection scheduler has outstanding work.
  bool HasWork() override;
  // Performs one unit of work from the connection scheduler.
  bool ScheduleWork() override;
  // Picks a candidate packet type based on the selected packet type scheduling
  // policy.
  absl::StatusOr<PacketTypeQueue> PickCandidatePacketType(
      PacketTypeQueue& starting_packet_type_queue,
      bool& is_starting_packet_type_queue_initialized);
  // Picks a candidate connection based the connection scheduling policy.
  absl::StatusOr<uint32_t> PickCandidateConnection(
      absl::StatusOr<PacketTypeQueue>& candidate_packet_type_queue,
      std::optional<uint32_t>& starting_connection_id);
  // Attempts to schedule the selected packet based on packet type and
  // connection policies.
  virtual bool AttemptPacketScheduling(
      uint32_t scid, const WorkId& work_id,
      absl::StatusOr<PacketTypeQueue> candidate_packet_type_queue);

 protected:
  // Schedules the packet by handing it off to the downstream blocks.
  void SchedulePacket(uint32_t scid, PacketTypeQueue queue_type,
                      const WorkId& work_id);

  virtual void UpdatePolicyStates(uint32_t scid, PacketTypeQueue queue_type);

 private:
  // Selects a candidate connection to transmit an eligible packet.
  absl::StatusOr<uint32_t> SelectCandidateConnection(
      PacketTypeQueue queue_type,
      std::optional<uint32_t> starting_connection_id);
  void HandlePhantomRequest(uint32_t scid, const WorkId& work_id,
                            PacketTypeQueue candidate_queue_type) override;
  // Represent the intra and inter connection scheduling policies adopted by the
  // connection scheduler.
  std::unique_ptr<IntraPacketSchedulingPolicy> intra_packet_policy_;
  std::unique_ptr<InterPacketSchedulingPolicy> inter_packet_policy_;
};
};  // namespace isekai
#endif  // ISEKAI_HOST_FALCON_FALCON_PROTOCOL_PACKET_TYPE_BASED_CONNECTION_SCHEDULER_H_
