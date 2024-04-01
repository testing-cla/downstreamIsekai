#include "isekai/host/rnic/omnest_environment.h"

#include <string>
#include <utility>

#include "absl/functional/any_invocable.h"
#include "absl/status/status.h"
#include "absl/time/time.h"
#include "glog/logging.h"
#include "omnetpp/csimplemodule.h"
#include "omnetpp/csimulation.h"
#include "omnetpp/simtime.h"
#include "omnetpp/simtime_t.h"

namespace isekai {

absl::Status OmnestEnvironment::ScheduleEvent(
    absl::Duration delay, absl::AnyInvocable<void()> callback) {
  if (delay < absl::ZeroDuration()) {
    LOG(FATAL) << "Negative delay: " << delay;
  }

  auto* callback_message = new CallbackMessage(std::move(callback));
  if (!callback_message) {
    return absl::InternalError("Fail to allocate callback message.");
  }

  // The scheduled event needs to be wrt a certain host.
  // Converts the delay in seconds for OMNest to schedule events correctly.
  module_->scheduleAt(
      omnetpp::simTime() + omnetpp::SimTime(absl::ToDoubleSeconds(delay)),
      callback_message);

  return absl::OkStatus();
}

}  // namespace isekai
