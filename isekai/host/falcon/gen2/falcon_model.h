#ifndef ISEKAI_HOST_FALCON_GEN2_FALCON_MODEL_H_
#define ISEKAI_HOST_FALCON_GEN2_FALCON_MODEL_H_

#include <cstdint>
#include <memory>

#include "absl/strings/string_view.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_model.h"
#include "isekai/host/falcon/gen2/falcon_component_interfaces.h"
#include "isekai/host/falcon/gen2/rx_packet_buffer_fetch_arbiter.h"
#include "isekai/host/falcon/gen2/sram_dram_reorder_engine.h"
#include "isekai/host/rnic/memory_interface.h"

namespace isekai {

class Gen2FalconModel : public FalconModel,
                        public Gen2FalconModelExtensionInterface {
 public:
  explicit Gen2FalconModel(const FalconConfig& configuration, Environment* env,
                           StatisticCollectionInterface* stats_collector,
                           ConnectionManagerInterface* connection_manager,
                           std::string_view host_id, uint8_t number_of_hosts);
  // Getter for a pointer to the on-NIC DRAM interface.
  MemoryInterface* get_on_nic_dram_interface() const override {
    return on_nic_dram_interface_.get();
  }
  // Getter for a pointer to the SRAM-DRAM reorder engine.
  Gen2SramDramReorderEngine* get_sram_dram_reorder_engine() const override {
    return sram_dram_reorder_engine_.get();
  }
  // Getter for a pointer to the packet payload fetch arbiter.
  Gen2RxPacketBufferFetchArbiter* get_payload_fetch_arbiter() {
    return payload_fetch_arbiter_.get();
  }

 private:
  // Creates the cookie that flows from Falcon to the ULP and then back to
  // Falcon in a ULP ACK.
  std::unique_ptr<OpaqueCookie> CreateCookie(const Packet& packet) override;
  // Sets the right connection type for a connection to be established based on
  // FalconConfig flags and Falcon version.
  void SetConnectionType(
      ConnectionState::ConnectionMetadata& metadata) override;
  ConnectionState::ConnectionMetadata CreateConnectionMetadata(
      uint32_t scid, uint32_t dcid, uint8_t source_bifurcation_id,
      uint8_t destination_bifurcation_id, absl::string_view dst_ip_address,
      OrderingMode ordering_mode,
      const FalconConnectionOptions& options) override;

  const std::unique_ptr<MemoryInterface> on_nic_dram_interface_;
  const std::unique_ptr<Gen2SramDramReorderEngine> sram_dram_reorder_engine_;
  const std::unique_ptr<Gen2RxPacketBufferFetchArbiter> payload_fetch_arbiter_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_FALCON_MODEL_H_
