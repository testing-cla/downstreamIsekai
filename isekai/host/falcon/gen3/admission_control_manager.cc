#include "isekai/host/falcon/gen3/admission_control_manager.h"

#include "isekai/host/falcon/gen3/falcon_utils.h"

namespace isekai {

Gen3AdmissionControlManager::Gen3AdmissionControlManager(
    FalconModelInterface* falcon)
    : Gen2AdmissionControlManager(falcon) {}

bool Gen3AdmissionControlManager::MeetsAdmissionControlCriteria(
    uint32_t scid, uint32_t rsn, falcon::PacketType type) {
  return Gen2AdmissionControlManager::MeetsAdmissionControlCriteria(scid, rsn,
                                                                    type);
}

absl::Status Gen3AdmissionControlManager::ReserveAdmissionControlResource(
    uint32_t scid, uint32_t rsn, falcon::PacketType type) {
  return Gen2AdmissionControlManager::ReserveAdmissionControlResource(scid, rsn,
                                                                      type);
}

absl::Status Gen3AdmissionControlManager::RefundAdmissionControlResource(
    uint32_t scid, const TransactionKey& transaction_key) {
  return Gen2AdmissionControlManager::RefundAdmissionControlResource(
      scid, transaction_key);
}

}  // namespace isekai
