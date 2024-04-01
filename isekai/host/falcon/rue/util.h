#ifndef ISEKAI_HOST_FALCON_RUE_UTIL_H_
#define ISEKAI_HOST_FALCON_RUE_UTIL_H_

#include <cstdint>

#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/rue/bits.h"

namespace falcon_rue {

struct PacketTiming {
  uint32_t rtt;
  uint32_t delay;
};

struct NicWindowGuardInfo {
  uint32_t time_marker;
  falcon::WindowDirection direction;
};

// Determines the delta time since the change in the congestion window.
// GetFabricWindowGuard only counts decreases as 'changes' whereas
// GetNicWindowGuard counts decreases and increases as 'changes'.
inline uint32_t GetWindowDelta(uint32_t now, uint32_t window_time_marker) {
  return TimeBits(now - window_time_marker);
}

// Returns the RTT and delay based on the configured packet timing mode.
template <typename EventT>
PacketTiming GetPacketTiming(const EventT& event) {
  PacketTiming timing;
  timing.rtt = TimeBits<uint32_t>(event.timestamp_4 - event.timestamp_1);
  switch (event.delay_select) {
    case falcon::DelaySelect::kFull:
      // T4-T1
      timing.delay = timing.rtt;
      break;
    case falcon::DelaySelect::kFabric:
      // (T4-T1)-(T3-T2)
      timing.delay = TimeBits<uint32_t>(
          timing.rtt -
          TimeBits<uint32_t>(event.timestamp_3 - event.timestamp_2));
      break;
    case falcon::DelaySelect::kForward:
      // T2-T1
      timing.delay = TimeBits<uint32_t>(event.timestamp_2 - event.timestamp_1);
      break;
    case falcon::DelaySelect::kReverse:
      // T4-T3
      timing.delay = TimeBits<uint32_t>(event.timestamp_4 - event.timestamp_3);
      break;
  }
  return timing;
}

// Determines the new value used for fabric congestion window guarding.
// This guard is only used for decreasing the window.
template <typename T>
uint32_t GetFabricWindowTimeMarker(uint32_t now,
                                   uint32_t last_window_time_marker,
                                   uint32_t rtt, T last_congestion_window,
                                   T next_congestion_window,
                                   T min_congestion_window) {
  uint32_t next_window_time_marker;
  if ((next_congestion_window < last_congestion_window) ||
      (next_congestion_window == min_congestion_window)) {
    // Sets time_marker to current time
    next_window_time_marker = now;
  } else {
    uint32_t delta =
        falcon_rue::TimeBits<uint32_t>(now - last_window_time_marker);
    if (delta > rtt) {
      // Sets time_marker to trail current time by one RTT
      next_window_time_marker = falcon_rue::TimeBits<uint32_t>(now - rtt);
    } else {
      // No change to window time_marker
      next_window_time_marker = last_window_time_marker;
    }
  }
  return next_window_time_marker;
}

// Determines the new value used for nic congestion window guarding.
// This guard is used for both increasing and decreasing the window.
template <typename T>
NicWindowGuardInfo GetNicWindowGuardInfo(
    uint32_t now, uint32_t last_window_time_marker, uint32_t rtt,
    falcon::WindowDirection last_direction, T last_congestion_window,
    T next_congestion_window, T min_congestion_window,
    T max_congestion_window) {
  NicWindowGuardInfo info;
  if ((next_congestion_window < last_congestion_window) ||
      (next_congestion_window == min_congestion_window)) {
    // Sets time_marker to current time
    info.time_marker = now;
    info.direction = falcon::WindowDirection::kDecrease;
  } else if ((next_congestion_window > last_congestion_window) ||
             (next_congestion_window == max_congestion_window)) {
    // Sets time_marker to current time
    info.time_marker = now;
    info.direction = falcon::WindowDirection::kIncrease;
  } else {
    // Direction stays the same
    info.direction = last_direction;
    // Gets the delta from the last update
    uint32_t delta =
        falcon_rue::TimeBits<uint32_t>(now - last_window_time_marker);
    if (delta > rtt) {
      // Sets time_marker to trail current time by one RTT
      info.time_marker = falcon_rue::TimeBits<uint32_t>(now - rtt);
    } else {
      // No change to window time_marker
      info.time_marker = last_window_time_marker;
    }
  }
  return info;
}

}  // namespace falcon_rue

#endif  // ISEKAI_HOST_FALCON_RUE_UTIL_H_
