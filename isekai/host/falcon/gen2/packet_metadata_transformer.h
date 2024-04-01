#ifndef ISEKAI_HOST_FALCON_GEN2_PACKET_METADATA_TRANSFORMER_H_
#define ISEKAI_HOST_FALCON_GEN2_PACKET_METADATA_TRANSFORMER_H_

#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/packet_metadata_transformer.h"

namespace isekai {

class Gen2PacketMetadataTransformer : public Gen1PacketMetadataTransformer {
 public:
  explicit Gen2PacketMetadataTransformer(FalconModelInterface* falcon);

 private:
  // Handles adding any fields to the packet metadata as configured.
  void TransformPacketMetadata(
      Packet* packet, const ConnectionState* connection_state) override;

  // Inserts the static routing port list into the packet if the list exists.
  void InsertStaticPortList(Packet* packet,
                            const ConnectionState* connection_state) override;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_PACKET_METADATA_TRANSFORMER_H_
