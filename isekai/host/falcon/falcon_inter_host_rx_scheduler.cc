#include "isekai/host/falcon/falcon_inter_host_rx_scheduler.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>

#include "absl/functional/any_invocable.h"
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/substitute.h"
#include "absl/time/time.h"
#include "glog/logging.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_inter_host_rx_round_robin_policy.h"

namespace isekai {
namespace {
// Flag: enable_inter_host_rx_scheduler_queue_length
constexpr std::string_view kStatVectorInterHostRxSchedulerQueueLength =
    "falcon.inter_host_rx_scheduler.bifurcation$0.outstanding_packets";
}  // namespace

// Constructor for inter host rx scheduler.
ProtocolInterHostRxScheduler::ProtocolInterHostRxScheduler(
    FalconModelInterface* falcon, uint8_t number_of_hosts)
    : falcon_(falcon), is_running_(false) {
  CHECK(falcon_) << "Falcon cannot be nullptr";
  switch (falcon_->get_config()->inter_host_rx_scheduling_policy()) {
    case FalconConfig::ROUND_ROBIN:
      inter_host_rx_policy_ =
          std::make_unique<InterHostRxSchedulingRoundRobinPolicy>();
      break;
    default:
      LOG(FATAL) << "Unsupported scheduling policy provided.";
  }
  rx_link_bandwidth_gpbs_ = falcon_->get_config()->rx_falcon_ulp_link_gbps();
  scheduler_cycle_time_ns_ = absl::Nanoseconds(
      falcon_->get_config()->inter_host_rx_scheduling_tick_ns());
  // Initialize the inter-packet gap to the minimum possible value, which is the
  // scheduler cycle time.
  inter_packet_gap_ns_ = scheduler_cycle_time_ns_;
  for (uint8_t host_idx = 0; host_idx < number_of_hosts; host_idx++) {
    CHECK_OK(InitInterHostSchedulerQueues(host_idx));
  }
  collect_inter_host_scheduler_queue_length_ =
      falcon->get_stats_manager()
          ->GetStatsConfig()
          .enable_inter_host_rx_scheduler_queue_length();
}

// Initializes the inter host scheduling queues.
absl::Status ProtocolInterHostRxScheduler::InitInterHostSchedulerQueues(
    uint8_t bifurcation_id) {
  return inter_host_rx_policy_->InitHost(bifurcation_id);
}

void ProtocolInterHostRxScheduler::Enqueue(uint8_t bifurcation_id,
                                           absl::AnyInvocable<void()> cb) {
  hosts_queue_[bifurcation_id].push(std::move(cb));
  // Record new outstanding Falcon packets to process.
  StatisticCollectionInterface* stats_collector =
      falcon_->get_stats_collector();
  if (collect_inter_host_scheduler_queue_length_ && stats_collector) {
    // Records TX Packet Credits available.
    CHECK_OK(stats_collector->UpdateStatistic(
        absl::Substitute(kStatVectorInterHostRxSchedulerQueueLength,
                         bifurcation_id),
        hosts_queue_[bifurcation_id].size(),
        StatisticsCollectionConfig::TIME_SERIES_STAT));
  }

  inter_host_rx_policy_->MarkHostActive(bifurcation_id);
  // if the scheduler is already not running, start it.
  if (!is_running_) {
    auto current_time = falcon_->get_environment()->ElapsedTime();
    absl::Duration next_run_time =
        std::max(last_scheduler_run_time_ + inter_packet_gap_ns_, current_time);
    is_running_ = true;
    CHECK_OK(falcon_->get_environment()->ScheduleEvent(
        (next_run_time - current_time), [this]() { ScheduleWork(); }));
  }
}

// Checks if there is any outstanding work to do, if yes, perform it and
// schedule another event.
void ProtocolInterHostRxScheduler::ScheduleWork() {
  is_running_ = false;
  absl::StatusOr<uint8_t> candidate_status_or_id =
      inter_host_rx_policy_->SelectHost();

  if (!candidate_status_or_id.ok()) {
    return;
  }

  uint8_t candidate_bifurcation_id = candidate_status_or_id.value();
  auto& callback = hosts_queue_[candidate_bifurcation_id].front();
  last_scheduler_run_time_ = falcon_->get_environment()->ElapsedTime();
  callback();
  hosts_queue_[candidate_bifurcation_id].pop();
  // Record new outstanding Falcon packets to process.
  StatisticCollectionInterface* stats_collector =
      falcon_->get_stats_collector();
  if (collect_inter_host_scheduler_queue_length_ && stats_collector) {
    // Records TX Packet Credits available.
    CHECK_OK(stats_collector->UpdateStatistic(
        absl::Substitute(kStatVectorInterHostRxSchedulerQueueLength,
                         candidate_bifurcation_id),
        hosts_queue_[candidate_bifurcation_id].size(),
        StatisticsCollectionConfig::TIME_SERIES_STAT));
  }

  if (hosts_queue_[candidate_bifurcation_id].empty()) {
    inter_host_rx_policy_->MarkHostInactive(candidate_bifurcation_id);
  }

  if (HasWork()) {
    is_running_ = true;
    CHECK_OK(falcon_->get_environment()->ScheduleEvent(
        inter_packet_gap_ns_, [this]() { ScheduleWork(); }));
  }
}

// Xoffs and Xons the provided host Rx queue.
void ProtocolInterHostRxScheduler::SetXoff(uint8_t bifurcation_id, bool xoff) {
  if (xoff) {
    inter_host_rx_policy_->XoffHost(bifurcation_id);
  } else {
    inter_host_rx_policy_->XonHost(bifurcation_id);
    if (HasWork() && !is_running_) {
      auto current_time = falcon_->get_environment()->ElapsedTime();
      absl::Duration next_run_time = std::max(
          last_scheduler_run_time_ + inter_packet_gap_ns_, current_time);
      is_running_ = true;
      CHECK_OK(falcon_->get_environment()->ScheduleEvent(
          (next_run_time - current_time), [this]() { ScheduleWork(); }));
    }
  }
}

// Returns true if the host scheduler has outstanding work.
bool ProtocolInterHostRxScheduler::HasWork() {
  return inter_host_rx_policy_->HasWork();
}

void ProtocolInterHostRxScheduler::UpdateInterPacketGap(uint32_t packet_size) {
  auto packet_serialization_delay =
      absl::Nanoseconds(packet_size * 8.0 / rx_link_bandwidth_gpbs_);
  inter_packet_gap_ns_ =
      std::max(scheduler_cycle_time_ns_, packet_serialization_delay);
}

}  // namespace isekai
