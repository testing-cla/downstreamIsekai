#include "isekai/host/falcon/gen3/packet_metadata_transformer.h"

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/gen2/packet_metadata_transformer.h"
#include "isekai/host/falcon/gen3/falcon_utils.h"

namespace isekai {

Gen3PacketMetadataTransformer::Gen3PacketMetadataTransformer(
    FalconModelInterface* falcon)
    : Gen2PacketMetadataTransformer(falcon) {}

void Gen3PacketMetadataTransformer::InsertStaticPortList(
    Packet* packet, const ConnectionState* connection_state) {
  // In Gen3, the size of the static_routing_port_list must be 1.
  CHECK_EQ(
      connection_state->connection_metadata.static_routing_port_lists.value()
          .size(),
      1);
  packet->metadata.static_route.port_list =
      connection_state->connection_metadata.static_routing_port_lists
          .value()[0];
}

}  // namespace isekai
