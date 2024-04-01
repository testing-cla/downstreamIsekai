#include "isekai/host/falcon/gen2/admission_control_manager.h"

namespace isekai {

Gen2AdmissionControlManager::Gen2AdmissionControlManager(
    FalconModelInterface* falcon)
    : ProtocolAdmissionControlManager(falcon) {}

}  // namespace isekai
