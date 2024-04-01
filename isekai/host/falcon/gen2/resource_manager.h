#ifndef ISEKAI_HOST_FALCON_GEN2_RESOURCE_MANAGER_H_
#define ISEKAI_HOST_FALCON_GEN2_RESOURCE_MANAGER_H_

#include <cstdint>

#include "absl/status/status.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_protocol_resource_manager.h"
#include "isekai/host/falcon/falcon_types.h"

namespace isekai {

class Gen2ResourceManager : public ProtocolResourceManager {
 public:
  explicit Gen2ResourceManager(FalconModelInterface* falcon);
  absl::Status VerifyResourceAvailabilityOrReserveResources(
      uint32_t scid, const Packet* packet, PacketDirection direction,
      bool reserve_resources) override;

 private:
  void ReturnRdmaManagedFalconResourceCredits(
      ConnectionState* connection_state,
      FalconCredit rdma_managed_resource_credits) override;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_RESOURCE_MANAGER_H_
