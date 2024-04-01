#ifndef ISEKAI_HOST_FALCON_GEN3_PACKET_METADATA_TRANSFORMER_H_
#define ISEKAI_HOST_FALCON_GEN3_PACKET_METADATA_TRANSFORMER_H_

#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/gen2/packet_metadata_transformer.h"

namespace isekai {

class Gen3PacketMetadataTransformer : public Gen2PacketMetadataTransformer {
 public:
  explicit Gen3PacketMetadataTransformer(FalconModelInterface* falcon);

 private:
  // Inserts the static routing port list into the packet if the list exists.
  void InsertStaticPortList(Packet* packet,
                            const ConnectionState* connection_state) override;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_PACKET_METADATA_TRANSFORMER_H_
