#ifndef ISEKAI_HOST_FALCON_FALCON_INTER_HOST_RX_SCHDULER_H_
#define ISEKAI_HOST_FALCON_FALCON_INTER_HOST_RX_SCHDULER_H_

#include <cstdint>
#include <memory>
#include <queue>

#include "absl/container/flat_hash_map.h"
#include "absl/functional/any_invocable.h"
#include "absl/status/status.h"
#include "absl/time/time.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"

namespace isekai {

// Reflects the Inter Host Rx scheduler that runs inside Falcon.
class ProtocolInterHostRxScheduler : public InterHostRxScheduler {
 public:
  // Constructor for inter host rx scheduler.
  explicit ProtocolInterHostRxScheduler(FalconModelInterface* falcon,
                                        uint8_t number_of_hosts);
  // Initializes the inter host scheduling queues.
  absl::Status InitInterHostSchedulerQueues(uint8_t bifurcation_id) override;
  // Enqueues the callback into the host specific Rx queue.
  void Enqueue(uint8_t bifurcation_id, absl::AnyInvocable<void()> cb) override;
  // Returns true if the host scheduler has outstanding work.
  bool HasWork() override;
  // Performs one unit of work from the host scheduler.
  void ScheduleWork() override;
  // Set Xoff/Xon for the given host Rx queue.
  void SetXoff(uint8_t bifurcation_id, bool xoff) override;
  // Update the scheduler inter-packet gap based on the size of the packet being
  // sent to ULP.
  void UpdateInterPacketGap(uint32_t packet_size) override;

 private:
  FalconModelInterface* const falcon_;
  // Indicates the minimum time it takes for the scheduler to service the next
  // packet after it sends the current packet to the ULP.
  absl::Duration scheduler_cycle_time_ns_;
  // Indicates the inter_packet_gap of the scheduler which is determined by the
  // max(chip_cycle_time_ns, packet_serialization_delay).
  absl::Duration inter_packet_gap_ns_;
  // Receive link bandwidth.
  uint32_t rx_link_bandwidth_gpbs_;
  // Flag to indicate if an arbitration event is scheduled or not.
  bool is_running_;
  // Represent the inter host scheduling policies adopted by the
  // host scheduler.
  std::unique_ptr<InterHostRxSchedulingPolicy> inter_host_rx_policy_;
  // Last time when the host scheduler ran and sent a packet to ULP.
  absl::Duration last_scheduler_run_time_ = absl::ZeroDuration();

  absl::flat_hash_map<uint8_t, std::queue<absl::AnyInvocable<void()>>>
      hosts_queue_;
  bool collect_inter_host_scheduler_queue_length_ = false;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_INTER_HOST_RX_SCHDULER_H_
