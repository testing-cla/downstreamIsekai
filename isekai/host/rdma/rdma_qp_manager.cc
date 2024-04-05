#include "isekai/host/rdma/rdma_qp_manager.h"

#include <memory>
#include <string>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/functional/any_invocable.h"
#include "absl/time/time.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/status_util.h"
#include "isekai/host/rdma/rdma_component_interfaces.h"

namespace isekai {

void RdmaQpManagerInfiniteResources::CreateQp(
    std::unique_ptr<BaseQpContext> context) {
  CHECK(context->qp_id != kInvalidQpId) << "QP id 0 is reserved.";
  CHECK(!qp_contexts_.contains(context->qp_id))
      << "Duplicate QP id: " << context->qp_id;
  qp_contexts_[context->qp_id] = std::move(context);
}

void RdmaQpManagerInfiniteResources::ConnectQp(QpId local_qp_id,
                                               QpId remote_qp_id,
                                               RdmaConnectedMode rc_mode) {
  CHECK(qp_contexts_.contains(local_qp_id)) << "Qp context not found.";
  CHECK(remote_qp_id != kInvalidQpId)
      << "RC QP requires valid destination QP id.";
  qp_contexts_[local_qp_id]->rc_mode = rc_mode;
  qp_contexts_[local_qp_id]->dest_qp_id = remote_qp_id;
}

void RdmaQpManagerInfiniteResources::InitiateQpLookup(
    QpId qp_id,
    absl::AnyInvocable<void(absl::StatusOr<BaseQpContext*>)> callback) {
  CHECK_OK(env_->ScheduleEvent(absl::ZeroDuration(), [this, qp_id,
                                                      callback = std::move(
                                                          callback)]() mutable {
    callback(DirectQpLookup(qp_id));
  })) << "Fail to initiate QP lookup.";
}

BaseQpContext* RdmaQpManagerInfiniteResources::DirectQpLookup(QpId qp_id) {
  if (qp_contexts_.contains(qp_id)) {
    return qp_contexts_[qp_id].get();
  }
  LOG(FATAL) << "Qp context not found.";
}

}  // namespace isekai
