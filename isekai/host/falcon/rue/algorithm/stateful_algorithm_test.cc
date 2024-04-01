#include "isekai/host/falcon/rue/algorithm/stateful_algorithm.h"

#include <cstdint>
#include <memory>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "internal/testing.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/rue/algorithm/bypass.h"
#include "isekai/host/falcon/rue/algorithm/bypass.pb.h"
#include "isekai/host/falcon/rue/algorithm/dram_state_manager.h"
#include "isekai/host/falcon/rue/algorithm/swift.h"
#include "isekai/host/falcon/rue/bits.h"
#include "isekai/host/falcon/rue/fixed.h"
#include "isekai/host/falcon/rue/format_bna.h"

namespace isekai {
namespace rue {
namespace {

// Test to ensure that existing Bypass creation API is unaffected.
TEST(StatefulTests, BypassAPIUnaffected) {
  EXPECT_OK(BypassDna::Create(BypassConfiguration()));
}

// Test to ensure that existing Swift creation API is unaffected.
TEST(StatefulTests, SwiftAPIUnaffected) {
  EXPECT_OK(isekai::rue::SwiftBna::Create(
      isekai::rue::SwiftBna::DefaultConfiguration()));
}

// Dummy algorithm with a no-op process for testing.
template <typename EventT, typename ResponseT>
class DummyAlgorithm {
 public:
  using EventType = EventT;
  using ResponseType = ResponseT;

  void Process(const EventT& event, ResponseT& response, uint32_t now) {}
  void ProcessMultipath(const EventT& event, ResponseT& response,
                        RueConnectionState& rue_connection_state,
                        uint32_t now) {}
};

// Dummy manager as we require some dram state manager to instantiate
// the stateful algorithm. We use a DummyDramStateManager which accepts a
// pointer and increments it when state is updated. We use a pointer as we
// cannot directly access the count or have a helper function here as ownership
// of the manager object is transferred to the stateful algorithm.
template <typename EventT>
class DummyDramStateManager
    : public DramStateManagerInterface<DummyDramStateManager<EventT>, EventT> {
 public:
  explicit DummyDramStateManager(int* state_access_count)
      : state_access_count_(state_access_count) {}
  void* GetRawStateForEvent(const EventT& event) {
    ++(*state_access_count_);
    return reinterpret_cast<void*>(&state_);
  }
  StatefulAlgorithmConfig GetStatefulConfig() {
    return StatefulAlgorithmConfig{
        .per_connection_state_size = sizeof(decltype(state_)),
    };
  }

 private:
  int* state_access_count_{nullptr};
  int state_{0};
};

// Tests that the Stateful Algorithm does indeed access state.
TEST(StatefulTests, EnsureStateIsAccessed) {
  using StateT = int;
  using EventT = int;
  using ResponseT = int;
  using DramStateManagerT = DummyDramStateManager<EventT>;

  EventT event{};
  ResponseT response{};
  StatefulAlgorithm<DummyAlgorithm<EventT, ResponseT>, DramStateManagerT>
      algorithm;
  int state_access_count = 0;
  auto manager = std::make_unique<DramStateManagerT>(&state_access_count);
  algorithm.set_stateful_config(manager->GetStatefulConfig());
  algorithm.set_dram_state_manager(std::move(manager));
  EXPECT_EQ(state_access_count, 0);

  algorithm.Process(event, response, 0);
  EXPECT_EQ(state_access_count, 1);
  algorithm.Process(event, response, 0);
  EXPECT_EQ(state_access_count, 2);
}

// Tests that the Stateful Bypass Algorithm can be created and accesses state.
TEST(StatefulTests, EnsureStateIsAccessedInBypass) {
  using StateT = int;
  using EventT = falcon_rue::Event_BNA;
  using ResponseT = falcon_rue::Response_BNA;
  using DramStateManagerT = DummyDramStateManager<EventT>;
  using AlgorithmT = Bypass<EventT, ResponseT>;
  using StatefulAlgorithmT = StatefulAlgorithm<AlgorithmT, DramStateManagerT>;
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<StatefulAlgorithmT> bypass,
                       AlgorithmT::Create<StatefulAlgorithmT>(
                           isekai::rue::BypassConfiguration()));

  EventT event{};
  ResponseT response{};
  int state_access_count = 0;
  auto mgr = std::make_unique<DramStateManagerT>(&state_access_count);
  bypass->set_stateful_config(mgr->GetStatefulConfig());
  bypass->set_dram_state_manager(std::move(mgr));
  EXPECT_EQ(state_access_count, 0);

  bypass->Process(event, response, 0);
  EXPECT_EQ(state_access_count, 1);

  bypass->Process(event, response, 0);
  EXPECT_EQ(state_access_count, 2);
}

// Tests that the Stateful Swift Algorithm can be created and accesses state.
TEST(StatefulTests, EnsureStateIsAccessedInSwift) {
  using StateT = int;
  using EventT = falcon_rue::Event_BNA;
  using ResponseT = falcon_rue::Response_BNA;
  using DramStateManagerT = DummyDramStateManager<EventT>;
  using AlgorithmT = Swift<EventT, ResponseT>;
  using StatefulAlgorithmT = StatefulAlgorithm<AlgorithmT, DramStateManagerT>;
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<StatefulAlgorithmT> swift,
                       AlgorithmT::Create<StatefulAlgorithmT>(
                           AlgorithmT::DefaultConfiguration()));

  EventT event{};
  event.connection_id = 1234;
  event.event_type = falcon::RueEventType::kAck;
  event.timestamp_1 = 100000;
  event.timestamp_2 = 100440;
  event.timestamp_3 = 100600;
  event.timestamp_4 = 100800;
  event.retransmit_count = 0;
  event.retransmit_reason = falcon::RetransmitReason::kTimeout;
  event.nack_code = falcon::NackCode::kNotANack;
  event.forward_hops = 5;
  event.cc_metadata = 0;
  event.fabric_congestion_window = falcon_rue::FloatToFixed<double, uint32_t>(
      4.0, falcon_rue::kFractionalBits);
  event.num_packets_acked = 3;
  event.event_queue_select = 0;
  event.delay_select = falcon::DelaySelect::kForward;
  event.fabric_window_time_marker = 99000;  // < now - rtt_state
  event.base_delay = SwiftBna::MakeBaseDelayField(
      /*profile_index=*/0);
  event.delay_state = 150;
  event.rtt_state = 710;
  event.cc_opaque = 3;
  event.gen_bit = 0;
  ResponseT response{};
  int state_access_count = 0;
  auto mgr = std::make_unique<DramStateManagerT>(&state_access_count);
  swift->set_stateful_config(mgr->GetStatefulConfig());
  swift->set_dram_state_manager(std::move(mgr));
  EXPECT_EQ(state_access_count, 0);

  swift->Process(event, response, 0);
  EXPECT_EQ(state_access_count, 1);

  swift->Process(event, response, 0);
  EXPECT_EQ(state_access_count, 2);
}
}  // namespace
}  // namespace rue
}  // namespace isekai
