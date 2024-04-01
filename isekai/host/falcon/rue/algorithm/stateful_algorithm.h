#ifndef ISEKAI_HOST_FALCON_RUE_ALGORITHM_STATEFUL_ALGORITHM_H_
#define ISEKAI_HOST_FALCON_RUE_ALGORITHM_STATEFUL_ALGORITHM_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include "isekai/host/falcon/rue/algorithm/swift.h"
#include "isekai/host/falcon/rue/format_bna.h"

namespace isekai {
namespace rue {

// Enables the StatefulAlgorithm::Process() function to take either the stateful
// code path when this function returns true (i.e., a connection is multipath),
// or take the stateless path when it returns false.
template <typename EventT>
inline bool IsMultipathConnection(const EventT& event) {
  // Return false by default for DNA.
  return false;
}

template <>
inline bool IsMultipathConnection(const falcon_rue::Event_BNA& event) {
  // Return true for BNA events whose multipath_enable is set.
  return event.multipath_enable;
}

// Interface for stateful access pattern logic (required for benchmarking).
class StateAccessorInterface {
 public:
  using StateT = unsigned int;
  // Accesses the state, assuming it to be an array of StateT elements.
  virtual void AccessState(StateT* state) = 0;
  virtual ~StateAccessorInterface() = default;
};

// Default no-op accessor for the stateful algorithm.
class NoOpAccessor : public StateAccessorInterface {
 public:
  using StateT = StateAccessorInterface::StateT;
  void AccessState(StateT* state) override {}
};

// Structure to hold `StatefulAlgorithm` configuration.
struct StatefulAlgorithmConfig {
  // Size in bytes of the state held for each connection.
  // NOTE: this will eventually be fixed to the sizeof(structure) but as we
  // are also using this for benchmarking, we require the size as a runtime
  // variable.
  size_t per_connection_state_size{64};
};

// StatefulAlgorithm provides the same interface as the other algorithms
// (Bypass, Swift) but also accesses additional DRAM state through a
// DramStateManager before calling the parent class's Process function. The
// DramStateManager is a class that must have a `GetStateForEvent` method that
// takes an event as argument and returns a reference to the state associated
// with it. How and what state is obtained is the responsibility of the
// DramStateManager. Ensure that `set_dram_state_manager()`  has been called at
// least once before `Process()` is called. StatefulAlgorithm also takes a
// boolean template parameter `ForBenchmarking` that decides whether to perform
// benchmarking specific operations such as reading DRAM state who's size is
// specified at run time, and accessing the state via different access pattern
// accessors.
// The template Algorithm class must define a `Process(const EventT& event,
// ResponseT& response, uint32_t now)` and `ProcessMultipath(const EventT&
// event, ResponseT& response, StateT rue_connection_state, uint32_t now)`
// function.
template <typename Algorithm, typename DramStateManagerT,
          bool ForBenchmarking = true>  //
class StatefulAlgorithm : public Algorithm {
 public:
  using EventT = typename Algorithm::EventType;
  using ResponseT = typename Algorithm::ResponseType;

  template <typename... T>
  explicit StatefulAlgorithm(T... args) : Algorithm(args...) {
    set_state_accessor(std::make_unique<NoOpAccessor>());
  }

  void Process(const EventT& event, ResponseT& response, uint32_t now);
  void ProcessForBenchmarking(const EventT& event, ResponseT& response,
                              uint32_t now);

  void set_dram_state_manager(
      std::unique_ptr<DramStateManagerT> dram_state_manager) {
    dram_state_manager_ = std::move(dram_state_manager);
  }

  void set_stateful_config(const StatefulAlgorithmConfig& config) {
    stateful_config_ = config;
  }

  DramStateManagerT* get_dram_state_manager() const {
    return dram_state_manager_.get();
  }

  const StatefulAlgorithmConfig& stateful_config() const {
    return stateful_config_;
  }
  void set_state_accessor(
      std::unique_ptr<StateAccessorInterface> state_accessor) {
    state_accessor_ = std::move(state_accessor);
  }

 private:
  std::unique_ptr<DramStateManagerT> dram_state_manager_;
  StatefulAlgorithmConfig stateful_config_;
  std::unique_ptr<StateAccessorInterface> state_accessor_;
};

template <typename Algorithm, typename DramStateManagerT, bool ForBenchmarking>
void StatefulAlgorithm<Algorithm, DramStateManagerT, ForBenchmarking>::
    ProcessForBenchmarking(const EventT& event, ResponseT& response,
                           uint32_t now) {
  static_assert(ForBenchmarking == true);
  using StateT = StateAccessorInterface::StateT;
  StateT& state = dram_state_manager_->template GetStateForEvent<StateT>(event);
  StateT* state_ptr = reinterpret_cast<StateT*>(std::addressof(state));
  state_accessor_->AccessState(state_ptr);

  Algorithm::Process(event, response, now);
}

template <typename Algorithm, typename DramStateManagerT, bool ForBenchmarking>
void StatefulAlgorithm<Algorithm, DramStateManagerT, ForBenchmarking>::Process(
    const EventT& event, ResponseT& response, uint32_t now) {
  if constexpr (ForBenchmarking) {
    ProcessForBenchmarking(event, response, now);
  } else {
    if (!IsMultipathConnection(event)) {
      Algorithm::Process(event, response, now);
      return;
    }
    using StateT = RueConnectionState;
    StateT& state =
        dram_state_manager_->template GetStateForEvent<StateT>(event);
    Algorithm::ProcessMultipath(event, response, state, now);
  }
}

}  // namespace rue
}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_RUE_ALGORITHM_STATEFUL_ALGORITHM_H_
