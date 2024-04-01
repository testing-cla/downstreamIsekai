#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"

#include <queue>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "glog/logging.h"

namespace isekai {

ConnectionSchedulerQueues::ConnectionSchedulerQueues() {
  // Initialize the various scheduler queue types. Store it in a hash_map so
  // that the policy can enforce ordering as required.
  std::vector<PacketTypeQueue> queue_types = {
      PacketTypeQueue::kPullAndOrderedPushRequest,
      PacketTypeQueue::kUnorderedPushRequest, PacketTypeQueue::kPushData,
      PacketTypeQueue::kPushGrant, PacketTypeQueue::kPullData};
  for (const auto& type : queue_types) {
    queues_[type] = std::queue<WorkId>();
  }
}

void ConnectionSchedulerQueues::Enqueue(PacketTypeQueue queue_type,
                                        WorkId& work) {
  queues_[queue_type].push(work);
}

const WorkId& ConnectionSchedulerQueues::Peek(PacketTypeQueue queue_type) {
  CHECK(!queues_[queue_type].empty());
  return queues_[queue_type].front();
}

void ConnectionSchedulerQueues::Pop(PacketTypeQueue queue_type) {
  CHECK(!queues_[queue_type].empty());
  queues_[queue_type].pop();
}

bool ConnectionSchedulerQueues::IsEmpty(PacketTypeQueue queue_type) {
  return queues_[queue_type].empty();
}

uint32_t ConnectionSchedulerQueues::GetSize(PacketTypeQueue queue_type) {
  return queues_[queue_type].size();
}

RetransmissionSchedulerQueues::RetransmissionSchedulerQueues() {
  // Initialize the various scheduler queue types. Store it in a hash_map so
  // that the policy can enforce ordering as required.
  std::vector<WindowTypeQueue> queue_types = {WindowTypeQueue::kRequest,
                                              WindowTypeQueue::kData};
  for (const auto& type : queue_types) {
    queues_[type] = absl::btree_set<RetransmissionWorkId>();
  }
}

bool RetransmissionSchedulerQueues::Enqueue(WindowTypeQueue queue_type,
                                            RetransmissionWorkId& work) {
  return queues_[queue_type].insert(work).second;
}

const RetransmissionWorkId& RetransmissionSchedulerQueues::Peek(
    WindowTypeQueue queue_type) {
  CHECK(!queues_[queue_type].empty());
  return *queues_[queue_type].begin();
}

bool RetransmissionSchedulerQueues::Dequeue(WindowTypeQueue queue_type,
                                            RetransmissionWorkId& work_id) {
  auto elements_erased = queues_[queue_type].erase(work_id);
  return elements_erased > 0;
}

bool RetransmissionSchedulerQueues::IsEmpty(WindowTypeQueue queue_type) {
  return queues_[queue_type].empty();
}

}  // namespace isekai
