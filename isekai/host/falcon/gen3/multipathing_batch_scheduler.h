#ifndef ISEKAI_HOST_FALCON_GEN3_MULTIPATHING_BATCH_SCHEDULER_H_
#define ISEKAI_HOST_FALCON_GEN3_MULTIPATHING_BATCH_SCHEDULER_H_

#include "isekai/host/falcon/falcon_component_interfaces.h"

namespace isekai {

// Per connection bundle scheduling state.
struct BundleSchedulingState {
  // Keeps track of the current connection for this bundle.
  absl::StatusOr<uint32_t> current_connection;
  // Keeps track of how many packets are sent in current batch.
  uint32_t current_batch_tx_count;
  // Keeps track of whether a bundle receives a rescheduling signal.
  bool rescheduling_signal;
};

// This class implements multipath batch-based scheduling for the scheduling
// policy.
class Gen3MultipathBatchScheduler : public MultipathBatchScheduler {
 public:
  Gen3MultipathBatchScheduler(
      uint32_t batch_size, std::unique_ptr<MultipathSchedulingPolicy> policy);
  // Associates a CID as a connection candidate for a given bundle.
  absl::Status InitConnection(uint32_t cid, uint32_t cbid) override;
  // Returns the selected connection in a bundle according to a pre-defined
  // multipath scheduling policy.
  absl::StatusOr<uint32_t> SelectConnection(uint32_t cbid) override;

  // Receives feedback from a packet scheduling decision, and takes path
  // scheduling actions according to the feedback.
  absl::Status ReceiveSchedulingFeedback(uint32_t cbid,
                                         bool packet_sent) override;

 protected:
  // Keeps track of the scheduling state for each connection bundle.
  absl::flat_hash_map<uint32_t, BundleSchedulingState> scheduling_states_;
  // The batch size configuration in this scheduler. This is the maximum number
  // of packets we should send for a connection before making another scheduling
  // decision in multipath scheduling.
  // For example, a batch size of 8 means that we can schedule up to 8 packets
  // on the same connection before needing to reschedule, or reschedule
  // immediately if the connection gating criteria is not met for a packet.
  // Batch size of 0 means unlimited batching. It will only be rescheduled when
  // the connection gating criteria is not met. A batch size of 1 is equivalent
  // to disabling batching.
  const uint32_t batch_size_limit_;
  // The scheduling policy used for this scheduler. This multipath selection
  // policy should be invoked every N (N=batch size) packet, or when the sending
  // gating criteria is not met.
  std::unique_ptr<MultipathSchedulingPolicy> multipath_scheduling_policy_;
};
}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_MULTIPATHING_BATCH_SCHEDULER_H_
