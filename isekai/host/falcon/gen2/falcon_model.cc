#include "isekai/host/falcon/gen2/falcon_model.h"

#include <cstdint>
#include <memory>

#include "absl/log/check.h"
#include "absl/strings/string_view.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon_component_factories.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_model.h"
#include "isekai/host/falcon/gen2/falcon_utils.h"

namespace isekai {

Gen2FalconModel::Gen2FalconModel(const FalconConfig& configuration,
                                 Environment* env,
                                 StatisticCollectionInterface* stats_collector,
                                 ConnectionManagerInterface* connection_manager,
                                 std::string_view host_id,
                                 uint8_t number_of_hosts)
    : FalconModel(configuration, env, stats_collector, connection_manager,
                  host_id, number_of_hosts),
      on_nic_dram_interface_(CreateOnNicDramInterface(this)),
      sram_dram_reorder_engine_(
          CreateSramDramReorderEngine(this, number_of_hosts)),
      payload_fetch_arbiter_(
          CreateRxPacketBufferFetchArbiter(this, number_of_hosts)) {}

// as an argument to this function, or TX/RX packet context containing the
// flow_id works instead (rather than getting the flow_id from flow label in
// multiple places). Creates a Gen2OpaqueCookie and fills it with the needed
// information.
std::unique_ptr<OpaqueCookie> Gen2FalconModel::CreateCookie(
    const Packet& packet) {
  // In Gen2, we need the flow_id to be reflected inside the cookie along with a
  // ULP (N)ACK.
  uint8_t flow_id = GetFlowIdFromFlowLabel(packet.metadata.flow_label, this,
                                           packet.falcon.dest_cid);
  return std::make_unique<Gen2OpaqueCookie>(flow_id);
}

// Sets the right connection type for a connection to be established based on
// FalconConfig flags and Falcon version.
void Gen2FalconModel::SetConnectionType(
    ConnectionState::ConnectionMetadata& metadata) {
  auto connection_state_type =
      ConnectionState::ConnectionMetadata::ConnectionStateType::Gen2;
  metadata.connection_type = connection_state_type;
}

ConnectionState::ConnectionMetadata Gen2FalconModel::CreateConnectionMetadata(
    uint32_t scid, uint32_t dcid, uint8_t source_bifurcation_id,
    uint8_t destination_bifurcation_id, absl::string_view dst_ip_address,
    OrderingMode ordering_mode,
    const FalconConnectionOptions& connection_options) {
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconModel::CreateConnectionMetadata(
          scid, dcid, source_bifurcation_id, destination_bifurcation_id,
          dst_ip_address, ordering_mode, connection_options);

  // Have to set degree_of_multipathing for non-Gen1 ConnectionStateTypes. In
  // this case, we expect the underlying type of FalconConnectionOptions to be
  // FalconMultipathConnectionOptions.
  auto multipath_connection_options =
      dynamic_cast<const FalconMultipathConnectionOptions&>(connection_options);
  connection_metadata.degree_of_multipathing =
      multipath_connection_options.degree_of_multipathing;
  return connection_metadata;
}

}  // namespace isekai
