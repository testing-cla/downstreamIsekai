#ifndef ISEKAI_HOST_FALCON_GEN2_ADMISSION_CONTROL_MANAGER_H_
#define ISEKAI_HOST_FALCON_GEN2_ADMISSION_CONTROL_MANAGER_H_

#include "isekai/host/falcon/falcon_protocol_admission_control_manager.h"

namespace isekai {

class Gen2AdmissionControlManager : public ProtocolAdmissionControlManager {
 public:
  explicit Gen2AdmissionControlManager(FalconModelInterface* falcon);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_ADMISSION_CONTROL_MANAGER_H_
