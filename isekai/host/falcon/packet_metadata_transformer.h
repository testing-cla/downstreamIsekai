#ifndef ISEKAI_HOST_FALCON_PACKET_METADATA_TRANSFORMER_H_
#define ISEKAI_HOST_FALCON_PACKET_METADATA_TRANSFORMER_H_

#include <cstdint>
#include <memory>

#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"

namespace isekai {

class Gen1PacketMetadataTransformer : public PacketMetadataTransformer {
 public:
  explicit Gen1PacketMetadataTransformer(FalconModelInterface* falcon);

  // Transfers the Tx Packet belonging the 'scid' connection from Falcon to the
  // PacketMetadataTransformer module.
  void TransferTxPacket(std::unique_ptr<Packet> packet, uint32_t scid) override;

 protected:
  // Handles adding any fields to the packet metadata as configured.
  virtual void TransformPacketMetadata(Packet* packet,
                                       const ConnectionState* connection_state);
  FalconModelInterface* falcon_;

 private:
  // Inserts the static routing port list into the packet if the list exists.
  virtual void InsertStaticPortListToPacketIfExist(
      Packet* packet, const ConnectionState* connection_state);
  // Retrieves and inserts the static port list to the packet.
  virtual void InsertStaticPortList(Packet* packet,
                                    const ConnectionState* connection_state);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_PACKET_METADATA_TRANSFORMER_H_
