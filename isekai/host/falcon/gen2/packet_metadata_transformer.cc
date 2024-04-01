#include "isekai/host/falcon/gen2/packet_metadata_transformer.h"

#include <cstdint>

#include "absl/log/check.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/gen2/falcon_utils.h"
#include "isekai/host/falcon/packet_metadata_transformer.h"

namespace isekai {

Gen2PacketMetadataTransformer::Gen2PacketMetadataTransformer(
    FalconModelInterface* falcon)
    : Gen1PacketMetadataTransformer(falcon) {}

void Gen2PacketMetadataTransformer::TransformPacketMetadata(
    Packet* packet, const ConnectionState* connection_state) {
  // Apply all transforms from Gen1.
  Gen1PacketMetadataTransformer::TransformPacketMetadata(packet,
                                                         connection_state);
}

void Gen2PacketMetadataTransformer::InsertStaticPortList(
    Packet* packet, const ConnectionState* connection_state) {
  // The degree of multipathing should be equal to the size of the vector of
  // static routing port lists.
  CHECK(connection_state->connection_metadata.degree_of_multipathing ==
        connection_state->connection_metadata.static_routing_port_lists.value()
            .size());
  // In Gen2, the index of the static route list corresponds to the flow ID,
  // where the flow ID is the lower N bits of the flow label, with N being equal
  // to log2(degree_multipathing).
  auto flow_id =
      GetFlowIdFromFlowLabel(packet->metadata.flow_label, falcon_,
                             connection_state->connection_metadata.scid);
  packet->metadata.static_route.port_list =
      connection_state->connection_metadata.static_routing_port_lists
          .value()[flow_id];
}

}  // namespace isekai
