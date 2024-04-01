#include "isekai/host/rnic/network_channel_manager.h"

#include <algorithm>

#include "absl/log/check.h"
#include "absl/time/time.h"
#include "inet/linklayer/ethernet/EtherPhyFrame_m.h"
#include "isekai/common/environment.h"
#include "omnetpp/csimplemodule.h"

namespace isekai {

NetworkChannelManager::NetworkChannelManager(
    omnetpp::cSimpleModule* host_module, Environment* env)
    : host_module_(host_module), env_(env) {}

NetworkChannelManager::~NetworkChannelManager() {
  // Deletes packets that were scheduled to be sent but weren't before the end
  // of a simulation.
  while (!delayed_signals_.empty()) {
    DelayedEthernetSignal delayed_signal = delayed_signals_.top();
    delete delayed_signal.signal;
    delayed_signals_.pop();
  }
}

void NetworkChannelManager::ScheduleSend(inet::EthernetSignal* signal,
                                         absl::Duration send_delay,
                                         absl::Duration transmission_delay) {
  absl::Duration scheduled_time = env_->ElapsedTime() + send_delay;
  delayed_signals_.push(
      DelayedEthernetSignal{signal, scheduled_time, transmission_delay});

  absl::Duration scheduled_delay =
      std::max(send_delay, next_available_time_ - env_->ElapsedTime());
  CHECK_OK(env_->ScheduleEvent(scheduled_delay, [this] { SendOnWire(); }));
}

void NetworkChannelManager::SendOnWire() {
  // Channel is busy, so reschedule the Send event until after the channel
  // finishes its current transmission.
  if (env_->ElapsedTime() < next_available_time_) {
    CHECK_OK(env_->ScheduleEvent(next_available_time_ - env_->ElapsedTime(),
                                 [this] { SendOnWire(); }));
    return;
  }
  DelayedEthernetSignal delayed_signal = delayed_signals_.top();
  delayed_signals_.pop();

  host_module_->send(delayed_signal.signal, "out");
  // Keep the channel busy for the packet's given transmission_delay.
  next_available_time_ =
      env_->ElapsedTime() + delayed_signal.transmission_delay;
}

}  // namespace isekai
