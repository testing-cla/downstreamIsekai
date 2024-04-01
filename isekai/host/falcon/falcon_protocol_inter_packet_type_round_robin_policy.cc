#include "isekai/host/falcon/falcon_protocol_inter_packet_type_round_robin_policy.h"

#include <list>
#include <memory>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"

namespace isekai {

InterPacketTypeRoundRobinPolicy::InterPacketTypeRoundRobinPolicy() {
  static constexpr std::array<PacketTypeQueue, 5> queue_types = {
      PacketTypeQueue::kPullAndOrderedPushRequest,
      PacketTypeQueue::kUnorderedPushRequest, PacketTypeQueue::kPushData,
      PacketTypeQueue::kPushGrant, PacketTypeQueue::kPullData};

  for (const auto& queue : queue_types) {
    is_active_[queue] = false;
    inactive_queues_.push_back(queue);
  }
}

// Selects the packet type based queue in a round robin manner from the
// active queues.
absl::StatusOr<PacketTypeQueue>
InterPacketTypeRoundRobinPolicy::SelectPacketTypeBasedQueueType() {
  if (active_queues_.empty()) {
    return absl::UnavailableError("No active packet types to schedule.");
  }
  PacketTypeQueue packet_queue_type = active_queues_.front();
  active_queues_.pop_front();
  active_queues_.push_back(packet_queue_type);
  return packet_queue_type;
}

// Marks a given packet type active when the scheduler receives work.
void InterPacketTypeRoundRobinPolicy::MarkPacketTypeBasedQueueTypeActive(
    PacketTypeQueue packet_queue_type) {
  if (is_active_[packet_queue_type]) {
    return;
  } else {
    is_active_[packet_queue_type] = true;
    inactive_queues_.remove(packet_queue_type);
    active_queues_.push_back(packet_queue_type);
  }
}

// Marks a given packet type inactive when no outstanding work.
void InterPacketTypeRoundRobinPolicy::MarkPacketTypeBasedQueueTypeInactive(
    PacketTypeQueue packet_queue_type) {
  if (!is_active_[packet_queue_type]) {
    return;
  } else {
    is_active_[packet_queue_type] = false;
    active_queues_.remove(packet_queue_type);
    inactive_queues_.push_back(packet_queue_type);
  }
}

// Determines if the policy has outstanding work based on the available credits.
bool InterPacketTypeRoundRobinPolicy::HasWork() const {
  return !active_queues_.empty();
}

}  // namespace isekai
