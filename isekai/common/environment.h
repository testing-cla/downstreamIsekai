#ifndef ISEKAI_COMMON_ENVIRONMENT_H_
#define ISEKAI_COMMON_ENVIRONMENT_H_

#include <random>

#include "absl/functional/any_invocable.h"
#include "absl/status/status.h"
#include "absl/time/time.h"

namespace isekai {

// Abstract interface to the discrete event simulator environment.
class Environment {
 public:
  virtual ~Environment() {}

  // Runs a callback after a simulation-time delay. The given delay must be
  // nonnegative (i.e. events can't be scheduled in the past).
  virtual absl::Status ScheduleEvent(absl::Duration delay,
                                     absl::AnyInvocable<void()> callback) = 0;

  // Returns the amount of time passed since the start of the simulation.
  // absl::Duration has (at least) nanosecond precision, so it can exactly
  // represent something's master timer.
  virtual absl::Duration ElapsedTime() const = 0;

  // Gets a pseudorandom number generator.
  // In order to reproduce simulation results, we use mt19937, which is
  // deterministic and is seeded by a centralized RNG (e.g., provided by
  // OMNest).
  virtual std::mt19937* GetPrng() const = 0;
};

}  // namespace isekai

#endif  // ISEKAI_COMMON_ENVIRONMENT_H_
