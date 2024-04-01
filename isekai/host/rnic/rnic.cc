#include "isekai/host/rnic/rnic.h"

#include <memory>

#include "isekai/common/model_interfaces.h"
#include "isekai/host/rdma/rdma_roce_model.h"

namespace isekai {

RoceInterface* RNic::get_roce_model() const {
  //
  // roce. So for now we directly up cast rdma_ to return the RoceInterface. In
  // the future, we may separate roce from rdma, so that we return the
  // RoceInterface directly.
  if (get_falcon_model()) {
    // We can not have both FALCON and RoCE model in RNIC.
    return nullptr;
  } else {
    return static_cast<RdmaRoceModel*>(rdma_.get());
  }
}

void RNic::RegisterToConnectionManager(
    absl::string_view host_id, ConnectionManagerInterface* connection_manager) {
  connection_manager->RegisterRdma(host_id, rdma_.get());
  connection_manager->RegisterFalcon(host_id, falcon_.get());
}

}  // namespace isekai
