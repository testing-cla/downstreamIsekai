#ifndef ISEKAI_HOST_RDMA_RDMA_FREE_LIST_MANAGER_H_
#define ISEKAI_HOST_RDMA_RDMA_FREE_LIST_MANAGER_H_

#include <cstdint>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/host/rdma/rdma_component_interfaces.h"

namespace isekai {

class RdmaFreeListManager : public RdmaFreeListManagerInterface {
 public:
  explicit RdmaFreeListManager(const RdmaConfig& config);

  absl::Status AllocateResource(QpId qp_id) override;
  absl::Status FreeResource(QpId qp_id) override;

 private:
  // Map from QP to number of free list entries used by the QP.
  absl::flat_hash_map<QpId, int> qp_entries_;
  // Total number of free list entries remaining.
  uint32_t total_remaining_entries_;

  const RdmaConfig& config_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_RDMA_RDMA_FREE_LIST_MANAGER_H_
