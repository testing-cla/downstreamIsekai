#include "isekai/host/falcon/gen2/rx_packet_buffer_fetch_arbiter.h"

#include <cstdint>
#include <memory>
#include <queue>

#include "absl/log/check.h"
#include "absl/status/status.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_inter_host_rx_round_robin_policy.h"
#include "isekai/host/falcon/gen2/falcon_component_interfaces.h"
#include "isekai/host/falcon/gen2/falcon_types.h"

namespace isekai {

Gen2RxPacketBufferFetchArbiter::Gen2RxPacketBufferFetchArbiter(
    FalconModelInterface* falcon, uint8_t num_hosts)
    : falcon_(falcon),
      dram_interface_(dynamic_cast<Gen2FalconModelExtensionInterface*>(falcon_)
                          ->get_on_nic_dram_interface()),
      sram_dram_reorder_engine_(
          dynamic_cast<Gen2FalconModelExtensionInterface*>(falcon_)
              ->get_sram_dram_reorder_engine()),
      is_arbiter_xoff_(false) {
  arbitration_policy_ =
      std::make_unique<InterHostRxSchedulingRoundRobinPolicy>();
  for (auto host_id = 0; host_id < num_hosts; ++host_id) {
    CHECK_OK(arbitration_policy_->InitHost(host_id));
    CHECK_OK(InitializeHostQueue(host_id));
  }
}

// Initializes the corresponding host queue.
absl::Status Gen2RxPacketBufferFetchArbiter::InitializeHostQueue(
    uint8_t bifurcation_id) {
  bool was_inserted =
      host_queues_.insert({bifurcation_id, std::queue<BufferFetchWorkId>()})
          .second;
  if (!was_inserted) {
    return absl::AlreadyExistsError("Host queue already exist.");
  }
  return absl::OkStatus();
}

}  // namespace isekai
