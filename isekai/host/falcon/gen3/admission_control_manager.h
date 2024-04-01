#ifndef ISEKAI_HOST_FALCON_GEN3_ADMISSION_CONTROL_MANAGER_H_
#define ISEKAI_HOST_FALCON_GEN3_ADMISSION_CONTROL_MANAGER_H_

#include "isekai/host/falcon/gen2/admission_control_manager.h"

namespace isekai {

class Gen3AdmissionControlManager : public Gen2AdmissionControlManager {
 public:
  explicit Gen3AdmissionControlManager(FalconModelInterface* falcon);
  // Checks to see if the packet in question meets the admission control
  // criteria.
  bool MeetsAdmissionControlCriteria(uint32_t scid, uint32_t rsn,
                                     falcon::PacketType type) override;
  // Reserves resources related to admission control. Reserved resources
  // correspond to bytes in the solicitation window in case of window-based
  // solicitation and tokens deducted in the token bucket in case of the rate
  // limiters.
  absl::Status ReserveAdmissionControlResource(
      uint32_t scid, uint32_t rsn, falcon::PacketType type) override;
  // Refunds the reserved resources once the transaction is complete, if
  // necessary. This method is meaningful only for window-based admission
  // control as it is closed-loop while the rate based ones are open loop and
  // hence do not need to refund.
  absl::Status RefundAdmissionControlResource(
      uint32_t scid, const TransactionKey& transaction_key) override;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_ADMISSION_CONTROL_MANAGER_H_
