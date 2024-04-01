#ifndef ISEKAI_HOST_FALCON_GEN3_RESOURCE_MANAGER_H_
#define ISEKAI_HOST_FALCON_GEN3_RESOURCE_MANAGER_H_

#include "isekai/host/falcon/gen2/resource_manager.h"

namespace isekai {

class Gen3ResourceManager : public Gen2ResourceManager {
 public:
  explicit Gen3ResourceManager(FalconModelInterface* falcon);
  // Reserves the necessary TX/RX resources for the transaction or checks
  // availability, if required.
  absl::Status VerifyResourceAvailabilityOrReserveResources(
      uint32_t scid, const Packet* packet, PacketDirection direction,
      bool reserve_resources) override;
  // Releases the necessary TX/RX resources for the transaction, if required.
  absl::Status ReleaseResources(uint32_t scid,
                                const TransactionKey& transaction_key,
                                falcon::PacketType type) override;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_RESOURCE_MANAGER_H_
