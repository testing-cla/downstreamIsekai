#include "isekai/fabric/arbiter.h"

#include <memory>

#include "glog/logging.h"

namespace isekai {

std::unique_ptr<ArbiterInterface> CreateArbiter(
    RouterConfigProfile::ArbitrationScheme arbitration_scheme) {
  switch (arbitration_scheme) {
    case RouterConfigProfile::FIXED_PRIORITY:
      return std::make_unique<FixedPriorityArbiter>();
    default:
      //
      LOG(FATAL) << "unknown arbitration scheme.";
  }
}

int FixedPriorityArbiter::Arbitrate(
    const std::vector<const omnetpp::cMessage*>& packet_list) {
  for (int i = 0; i < packet_list.size(); i++) {
    if (packet_list[i]) {
      return i;
    }
  }

  return -1;
}

}  // namespace isekai
