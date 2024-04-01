#ifndef ISEKAI_HOST_RNIC_RNIC_H_
#define ISEKAI_HOST_RNIC_RNIC_H_

#include <memory>
#include <utility>

#include "absl/strings/string_view.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/host/rnic/memory_interface.h"

namespace isekai {

// The RNic class owns RDMA, FALCON and traffic shaper.
class RNic {
 public:
  RNic(std::unique_ptr<std::vector<std::unique_ptr<MemoryInterface>>> hif,
       std::unique_ptr<RdmaBaseInterface> rdma,
       std::unique_ptr<RoceInterface> roce,
       std::unique_ptr<TrafficShaperInterface> traffic_shaper,
       std::unique_ptr<FalconInterface> falcon)
      : hif_(std::move(hif)),
        rdma_(std::move(rdma)),
        roce_(std::move(roce)),
        traffic_shaper_(std::move(traffic_shaper)),
        falcon_(std::move(falcon)) {}

  // Getters for the Isekai models.
  RdmaBaseInterface* get_rdma_model() const { return rdma_.get(); }
  RoceInterface* get_roce_model() const;
  TrafficShaperInterface* get_traffic_shaper() const {
    return traffic_shaper_.get();
  }
  FalconInterface* get_falcon_model() const { return falcon_.get(); }
  std::vector<std::unique_ptr<MemoryInterface>>* get_host_interface_list()
      const {
    return hif_.get();
  }

  // Registers the RDMA module and the FALCON module to Connection Manager.
  void RegisterToConnectionManager(
      absl::string_view host_id,
      ConnectionManagerInterface* connection_manager);

 private:
  // Maintaining unique_ptr to vector to have ownership of all the HIFs and
  // using unique_ptr for HostInterface to avoid copy operation.
  const std::unique_ptr<std::vector<std::unique_ptr<MemoryInterface>>> hif_;
  const std::unique_ptr<RdmaBaseInterface> rdma_;
  const std::unique_ptr<RoceInterface> roce_;
  const std::unique_ptr<TrafficShaperInterface> traffic_shaper_;
  const std::unique_ptr<FalconInterface> falcon_;
};

};  // namespace isekai

#endif  // ISEKAI_HOST_RNIC_RNIC_H_
