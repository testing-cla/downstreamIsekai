#ifndef ISEKAI_FABRIC_ARBITER_H_
#define ISEKAI_FABRIC_ARBITER_H_

#include <vector>

#include "absl/strings/string_view.h"
#include "isekai/common/config.pb.h"
#include "isekai/fabric/model_interfaces.h"
#include "omnetpp/cmessage.h"

namespace isekai {

// Create an arbiter based on the given scheme, e.g., fixed priority arbitration
// or weighted round robin arbitration.
std::unique_ptr<ArbiterInterface> CreateArbiter(
    RouterConfigProfile::ArbitrationScheme arbitration_scheme);

// The class for priority arbiter.
class FixedPriorityArbiter : public ArbiterInterface {
 public:
  // Arbitrate TX packet based on packet priority.
  int Arbitrate(
      const std::vector<const omnetpp::cMessage*>& packet_list) override;
};

}  // namespace isekai

#endif  // ISEKAI_FABRIC_ARBITER_H_
