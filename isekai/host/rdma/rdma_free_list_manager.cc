#include "isekai/host/rdma/rdma_free_list_manager.h"

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "glog/logging.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/model_interfaces.h"

namespace isekai {

RdmaFreeListManager::RdmaFreeListManager(const RdmaConfig& config)
    : total_remaining_entries_(config.total_free_list_entries()),
      config_(config) {}

absl::Status RdmaFreeListManager::AllocateResource(QpId qp_id) {
  // Check if we have enough total free list entries.
  if (total_remaining_entries_ <= 0) {
    return absl::ResourceExhaustedError("Not enough free list entries.");
  }

  if (qp_entries_.contains(qp_id)) {
    if (qp_entries_[qp_id] >= config_.max_free_list_entries_per_qp()) {
      return absl::ResourceExhaustedError("Not enough QP free list entries.");
    } else {
      qp_entries_[qp_id] += 1;
    }
  } else {
    qp_entries_[qp_id] = 1;
  }
  --total_remaining_entries_;
  return absl::OkStatus();
}

absl::Status RdmaFreeListManager::FreeResource(QpId qp_id) {
  CHECK(qp_entries_.contains(qp_id))
      << "QP " << qp_id << " does not exist in FreeListManager.";
  CHECK(qp_entries_[qp_id] > 0)
      << "QP " << qp_id << " does not hold any free list entries.";

  qp_entries_[qp_id] -= 1;
  ++total_remaining_entries_;

  return absl::OkStatus();
}

}  // namespace isekai
