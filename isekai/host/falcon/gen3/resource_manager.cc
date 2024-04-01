#include "isekai/host/falcon/gen3/resource_manager.h"

#include "isekai/host/falcon/gen3/falcon_utils.h"

namespace isekai {

Gen3ResourceManager::Gen3ResourceManager(FalconModelInterface* falcon)
    : Gen2ResourceManager(falcon) {}

// Reserves the necessary TX/RX resources for the transaction or checks
// availability, if required.
absl::Status Gen3ResourceManager::VerifyResourceAvailabilityOrReserveResources(
    uint32_t scid, const Packet* packet, PacketDirection direction,
    bool reserve_resources) {
  return Gen2ResourceManager::VerifyResourceAvailabilityOrReserveResources(
      scid, packet, direction, reserve_resources);
}

// Releases the necessary TX/RX resources for the transaction, if required.
absl::Status Gen3ResourceManager::ReleaseResources(
    uint32_t scid, const TransactionKey& transaction_key,
    falcon::PacketType type) {
  return Gen2ResourceManager::ReleaseResources(scid, transaction_key, type);
}

}  // namespace isekai
