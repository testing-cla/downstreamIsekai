#include "isekai/host/falcon/gen2/resource_manager.h"

#include <cstdint>

#include "absl/log/check.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_protocol_resource_manager.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/gen2/falcon_component_interfaces.h"

namespace isekai {

Gen2ResourceManager::Gen2ResourceManager(FalconModelInterface* falcon)
    : ProtocolResourceManager(falcon) {}

absl::Status Gen2ResourceManager::VerifyResourceAvailabilityOrReserveResources(
    uint32_t scid, const Packet* packet, PacketDirection direction,
    bool reserve_resources) {
  auto status =
      ProtocolResourceManager::VerifyResourceAvailabilityOrReserveResources(
          scid, packet, direction, reserve_resources);
  return status;
}

void Gen2ResourceManager::ReturnRdmaManagedFalconResourceCredits(
    ConnectionState* connection_state,
    FalconCredit rdma_managed_resource_credits) {
  ProtocolResourceManager::ReturnRdmaManagedFalconResourceCredits(
      connection_state, rdma_managed_resource_credits);
}

}  // namespace isekai
