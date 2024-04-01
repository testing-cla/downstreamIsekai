#include "isekai/host/falcon/gen3/multipathing_batch_scheduler.h"

#include <cstdint>
#include <memory>
#include <utility>

#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "glog/logging.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"

namespace isekai {

Gen3MultipathBatchScheduler::Gen3MultipathBatchScheduler(
    uint32_t batch_size, std::unique_ptr<MultipathSchedulingPolicy> policy)
    : batch_size_limit_(batch_size),
      multipath_scheduling_policy_(std::move(policy)) {}

absl::Status Gen3MultipathBatchScheduler::InitConnection(uint32_t cid,
                                                         uint32_t cbid) {
  BundleSchedulingState& scheduling_state = scheduling_states_[cbid];
  scheduling_state.current_batch_tx_count = 0;
  // The rescheduling signal is set to true in the initial state to enforce the
  // first-time scheduling in the scheduling policy.
  scheduling_state.rescheduling_signal = true;
  CHECK_OK(multipath_scheduling_policy_->InitConnection(cid, cbid));
  return absl::OkStatus();
}

// Returns the selected connection in a bundle according to a pre-defined
// multipath scheduling policy.
absl::StatusOr<uint32_t> Gen3MultipathBatchScheduler::SelectConnection(
    uint32_t cbid) {
  BundleSchedulingState& scheduling_state = scheduling_states_[cbid];

  // Batch size 0 means unlimited batching. It will only be rescheduled when the
  // connection criteria are not met.
  bool have_space_left_in_batch =
      scheduling_state.current_batch_tx_count < batch_size_limit_ ||
      batch_size_limit_ == 0;

  if (have_space_left_in_batch && !scheduling_state.rescheduling_signal) {
    return scheduling_state.current_connection;
  } else {
    // Picks a new path, resets counters.
    scheduling_state.current_batch_tx_count = 0;
    scheduling_state.rescheduling_signal = false;
    scheduling_state.current_connection =
        multipath_scheduling_policy_->SelectConnection(cbid);
    return scheduling_state.current_connection;
  }
}

// Receives feedback from a packet scheduling decision, and takes path
// scheduling actions according to the feedback.
absl::Status Gen3MultipathBatchScheduler::ReceiveSchedulingFeedback(
    uint32_t cbid, bool packet_sent) {
  BundleSchedulingState& scheduling_state = scheduling_states_[cbid];
  if (packet_sent) {
    scheduling_state.current_batch_tx_count++;
  } else {
    scheduling_state.rescheduling_signal = true;
  }
  return absl::OkStatus();
}

}  // namespace isekai
