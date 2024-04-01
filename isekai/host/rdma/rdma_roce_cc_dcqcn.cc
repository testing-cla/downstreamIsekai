#include "isekai/host/rdma/rdma_roce_cc_dcqcn.h"

#include <algorithm>
#include <cstdint>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "glog/logging.h"
#include "isekai/common/common_util.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/status_util.h"
#include "isekai/host/rdma/rdma_component_interfaces.h"

namespace isekai {

void RdmaRoceCcDcqcn::HandleRxPacket(RoceQpContext* context, Packet* packet) {
  if (packet->roce.opcode != Packet::Roce::Opcode::kCongestionNotification)
    return;
  context->dcqcn.alpha_cnp_arrived = true;
  context->dcqcn.decrease_cnp_arrived = true;
  // On the first CNP.
  if (context->dcqcn.first_cnp) {
    // Initialize alpha.
    context->dcqcn.alpha = options_.initial_alpha_value;
    context->dcqcn.alpha_cnp_arrived = false;
    // Reduce rate.
    context->dcqcn.target_rate = options_.rate_to_set_on_first_cnp;
    context->rate = options_.rate_to_set_on_first_cnp;
    context->dcqcn.decrease_cnp_arrived = false;
    // Clear the first CNP bit.
    context->dcqcn.first_cnp = false;

    QpId qp_id = context->qp_id;
    // Schedule the next alpha update timer.
    CHECK_OK(env_->ScheduleEvent(
        absl::Microseconds(options_.dce_alpha_update_period),
        [this, qp_id]() { HandleAlphaUpdateTimer(qp_id); }));
    // Schedule the next rate decrease timer.
    CHECK_OK(env_->ScheduleEvent(
        absl::Microseconds(options_.rate_reduce_monitor_period),
        [this, qp_id]() { HandleRateDecreaseTimer(qp_id); }));
    // Schedule the next rate increase timer.
    CHECK_OK(env_->ScheduleEvent(
        absl::Microseconds(options_.time_reset),
        [this, qp_id]() { HandleRateIncreaseTimer(qp_id); }));
    context->dcqcn.rate_increase_timer_start_time = env_->ElapsedTime();
  }
}

void RdmaRoceCcDcqcn::HandleTxPacket(RoceQpContext* context, Packet* packet) {}

void RdmaRoceCcDcqcn::SentBytes(RoceQpContext* context, uint32_t bytes) {
  context->dcqcn.byte_count += bytes;
  while (context->dcqcn.byte_count >= options_.byte_reset * 64) {
    IncreaseRate(context);
    context->dcqcn.byte_count -= options_.byte_reset * 64;
    context->dcqcn.increase_stage_by_byte++;
  }
}

void RdmaRoceCcDcqcn::InitContext(RoceQpContext* context) {
  context->rate = context->dcqcn.target_rate = options_.max_rate;
}

void RdmaRoceCcDcqcn::HandleAlphaUpdateTimer(QpId qp_id) {
  qp_manager_->InitiateQpLookup(
      qp_id, [this, qp_id](absl::StatusOr<BaseQpContext*> qp_context) {
        CHECK_OK(qp_context.status())
            << "UpdateAlpha at unknown qp id: " << qp_context.status() << "\n";
        auto context = down_cast<RoceQpContext*>(*qp_context);
        if (context->dcqcn.alpha_cnp_arrived) {
          context->dcqcn.alpha =
              options_.dce_alpha_g * context->dcqcn.alpha / 1024 + 1024 -
              options_.dce_alpha_g;
          if (context->dcqcn.alpha > 1023) context->dcqcn.alpha = 1023;
        } else {
          context->dcqcn.alpha =
              options_.dce_alpha_g * context->dcqcn.alpha / 1024;
        }
        // Clear the CNP arrived bit.
        context->dcqcn.alpha_cnp_arrived = false;
        CHECK_OK(env_->ScheduleEvent(
            absl::Microseconds(options_.dce_alpha_update_period),
            [this, qp_id]() { HandleAlphaUpdateTimer(qp_id); }));
      });
}

void RdmaRoceCcDcqcn::HandleRateDecreaseTimer(QpId qp_id) {
  qp_manager_->InitiateQpLookup(
      qp_id, [this, qp_id](absl::StatusOr<BaseQpContext*> qp_context) {
        CHECK_OK(qp_context.status())
            << "CheckRateDecrease at unknown qp id: " << qp_context.status()
            << "\n";
        auto context = down_cast<RoceQpContext*>(*qp_context);
        // Schedule the next rate decrease timer.
        CHECK_OK(env_->ScheduleEvent(
            absl::Microseconds(options_.rate_reduce_monitor_period),
            [this, qp_id]() { HandleRateDecreaseTimer(qp_id); }));
        if (context->dcqcn.decrease_cnp_arrived) {
          // Test if target rate should be set to current rate.
          bool clamp = options_.clamp_tgt_rate ||
                       context->dcqcn.increase_stage_by_byte > 0 ||
                       (options_.clamp_tgt_rate_after_time_inc &&
                        context->dcqcn.increase_stage_by_timer > 0);
          if (clamp) context->dcqcn.target_rate = context->rate;
          // Reduce current rate.
          uint64_t rate_reduction =
              std::min(context->rate * context->dcqcn.alpha >>
                           options_.alpha_to_rate_shift,
                       context->rate * (100 - options_.min_dec_fac) / 100);
          context->rate -= rate_reduction;
          if (options_.min_rate > context->rate)
            context->rate = options_.min_rate;
          // Clear CNP arrived bit.
          context->dcqcn.decrease_cnp_arrived = false;
          // Reset rate increase related things.
          context->dcqcn.increase_stage_by_timer = 0;
          context->dcqcn.increase_stage_by_byte = 0;
          context->dcqcn.byte_count = 0;
          // Reschedule rate increase timer.
          CHECK_OK(env_->ScheduleEvent(
              absl::Microseconds(options_.time_reset),
              [this, qp_id]() { HandleRateIncreaseTimer(qp_id); }));
          context->dcqcn.rate_increase_timer_start_time = env_->ElapsedTime();
        }
      });
}

void RdmaRoceCcDcqcn::HandleRateIncreaseTimer(QpId qp_id) {
  // Check if this timer is cancelled.
  // The use of DirectQpLookup is only for checking cancellation, which is ok.
  auto context = down_cast<RoceQpContext*>(qp_manager_->DirectQpLookup(qp_id));
  if (context->dcqcn.rate_increase_timer_start_time +
          absl::Microseconds(options_.time_reset) !=
      env_->ElapsedTime())
    return;
  qp_manager_->InitiateQpLookup(
      qp_id, [this, qp_id](absl::StatusOr<BaseQpContext*> qp_context) {
        CHECK_OK(qp_context.status())
            << "RateIncrease at unknown qp id: " << qp_context.status() << "\n";
        auto context = down_cast<RoceQpContext*>(*qp_context);
        // Schedule the next rate increase timer.
        CHECK_OK(env_->ScheduleEvent(
            absl::Microseconds(options_.time_reset),
            [this, qp_id]() { HandleRateIncreaseTimer(qp_id); }));
        context->dcqcn.rate_increase_timer_start_time = env_->ElapsedTime();
        // Increase rate.
        IncreaseRate(context);
        // Update rate increase stage.
        context->dcqcn.increase_stage_by_timer++;
      });
}

void RdmaRoceCcDcqcn::IncreaseRate(RoceQpContext* context) {
  // Update target rate.
  if (std::min(context->dcqcn.increase_stage_by_timer,
               context->dcqcn.increase_stage_by_byte) >
      options_.stage_threshold) {
    // Hyper increase.
    context->dcqcn.target_rate += options_.hai_rate;
  } else if (std::max(context->dcqcn.increase_stage_by_timer,
                      context->dcqcn.increase_stage_by_byte) >=
             options_.stage_threshold) {
    // Additive increase.
    context->dcqcn.target_rate += options_.ai_rate;
  }
  if (context->dcqcn.target_rate > options_.max_rate)
    context->dcqcn.target_rate = options_.max_rate;
  // Update current rate.
  context->rate = (context->rate + context->dcqcn.target_rate) / 2;
}

}  // namespace isekai
