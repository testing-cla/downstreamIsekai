#include "isekai/host/falcon/rue/algorithm/bypass.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <tuple>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "internal/testing.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/rue/algorithm/bypass.pb.h"
#include "isekai/host/falcon/rue/format_bna.h"
#include "isekai/host/falcon/rue/format_dna_a.h"
#include "isekai/host/falcon/rue/format_dna_b.h"
#include "isekai/host/falcon/rue/format_dna_c.h"

template <typename EventT, typename ResponseT>
class GenericBypassTest : public testing::Test {
 public:
  GenericBypassTest() {
    memset(&event_, 0, sizeof(event_));
    memset(&response_, 0, sizeof(response_));
  }

  ~GenericBypassTest() override = default;
  EventT event_;
  ResponseT response_;
};

template <typename TypeParam>
class DnaBypassTest : public GenericBypassTest<
                          typename std::tuple_element<0, TypeParam>::type,
                          typename std::tuple_element<1, TypeParam>::type> {};

using Event_Response_Types = ::testing::Types<
    std::tuple<falcon_rue::Event_DNA_A, falcon_rue::Response_DNA_A>,
    std::tuple<falcon_rue::Event_DNA_B, falcon_rue::Response_DNA_B>,
    std::tuple<falcon_rue::Event_DNA_C, falcon_rue::Response_DNA_C>>;

TYPED_TEST_SUITE(DnaBypassTest, Event_Response_Types);

TYPED_TEST(DnaBypassTest, NoCongestionControl) {
  typedef typename std::tuple_element<0, TypeParam>::type EventT;
  typedef typename std::tuple_element<1, TypeParam>::type ResponseT;
  using BypassTyped = isekai::rue::Bypass<EventT, ResponseT>;

  ASSERT_OK_AND_ASSIGN(std::unique_ptr<BypassTyped> bypass,
                       BypassTyped::Create(isekai::rue::BypassConfiguration()));
  int now = 100400;

  this->event_.connection_id = 1234;
  this->event_.event_type = falcon::RueEventType::kAck;
  this->event_.timestamp_1 = 100000;
  this->event_.timestamp_2 = 100153;
  this->event_.timestamp_3 = 100200;
  this->event_.timestamp_4 = 100315;
  this->event_.retransmit_count = 0;
  this->event_.retransmit_reason = falcon::RetransmitReason::kTimeout;
  this->event_.nack_code = falcon::NackCode::kNotANack;
  this->event_.forward_hops = 5;
  this->event_.cc_metadata = 0;
  this->event_.fabric_congestion_window = 8642;
  this->event_.inter_packet_gap = 0;
  this->event_.retransmit_timeout = 1000;
  this->event_.num_packets_acked = 3;
  this->event_.event_queue_select = 0;
  this->event_.delay_select = falcon::DelaySelect::kForward;
  this->event_.fabric_window_time_marker = 99999;
  this->event_.base_delay = 100;
  this->event_.delay_state = 150;
  this->event_.rtt_state = 310;
  this->event_.cc_opaque = 3;
  this->event_.gen_bit = 0;

  bypass->Process(this->event_, this->response_, now);

  EXPECT_EQ(this->response_.connection_id, 1234);
  EXPECT_EQ(this->response_.randomize_path, false);
  EXPECT_EQ(this->response_.cc_metadata, 0);
  EXPECT_EQ(this->response_.fabric_congestion_window, 8642);
  EXPECT_EQ(this->response_.inter_packet_gap, 0);
  EXPECT_EQ(this->response_.retransmit_timeout, 1000);
  EXPECT_EQ(this->response_.fabric_window_time_marker, 0);
  EXPECT_EQ(this->response_.event_queue_select, 0);
  EXPECT_EQ(this->response_.delay_select, falcon::DelaySelect::kForward);
  EXPECT_EQ(this->response_.base_delay, 100);
  EXPECT_EQ(this->response_.delay_state, 150);
  EXPECT_EQ(this->response_.rtt_state, 310);
  EXPECT_EQ(this->response_.cc_opaque, 3);
}

class BnaBypassTest : public GenericBypassTest<falcon_rue::Event_BNA,
                                               falcon_rue::Response_BNA> {};

TEST_F(BnaBypassTest, NoCongestionControl) {
  using BypassTyped =
      isekai::rue::Bypass<falcon_rue::Event_BNA, falcon_rue::Response_BNA>;
  ASSERT_OK_AND_ASSIGN(std::unique_ptr<BypassTyped> bypass,
                       BypassTyped::Create(isekai::rue::BypassConfiguration()));
  int now = 100400;

  this->event_.connection_id = 1234;
  this->event_.multipath_enable = true;
  this->event_.flow_label = 0x12;
  this->event_.event_type = falcon::RueEventType::kAck;
  this->event_.timestamp_1 = 100000;
  this->event_.timestamp_2 = 100153;
  this->event_.timestamp_3 = 100200;
  this->event_.timestamp_4 = 100315;
  this->event_.retransmit_count = 0;
  this->event_.retransmit_reason = falcon::RetransmitReason::kTimeout;
  this->event_.nack_code = falcon::NackCode::kNotANack;
  this->event_.forward_hops = 5;
  this->event_.cc_metadata = 0;
  this->event_.fabric_congestion_window = 8642;
  this->event_.num_packets_acked = 3;
  this->event_.event_queue_select = 0;
  this->event_.delay_select = falcon::DelaySelect::kForward;
  this->event_.fabric_window_time_marker = 99999;
  this->event_.base_delay = 100;
  this->event_.delay_state = 150;
  this->event_.rtt_state = 310;
  this->event_.cc_opaque = 3;
  this->event_.gen_bit = 0;

  bypass->Process(this->event_, this->response_, now);

  EXPECT_EQ(this->response_.connection_id, 1234);
  EXPECT_EQ(this->response_.cc_metadata, 0);
  EXPECT_EQ(this->response_.fabric_congestion_window, 8642);
  EXPECT_EQ(this->response_.fabric_window_time_marker, 0);
  EXPECT_EQ(this->response_.event_queue_select, 0);
  EXPECT_EQ(this->response_.delay_select, falcon::DelaySelect::kForward);
  EXPECT_EQ(this->response_.base_delay, 100);
  EXPECT_EQ(this->response_.delay_state, 150);
  EXPECT_EQ(this->response_.rtt_state, 310);
  EXPECT_EQ(this->response_.cc_opaque, 3);
  EXPECT_EQ(this->response_.flow_id, 2);
  EXPECT_EQ(this->response_.flow_label_1_weight, 1);
  EXPECT_EQ(this->response_.flow_label_2_weight, 1);
  EXPECT_EQ(this->response_.flow_label_3_weight, 1);
  EXPECT_EQ(this->response_.flow_label_4_weight, 1);
  EXPECT_EQ(this->response_.flow_label_1_valid, false);
  EXPECT_EQ(this->response_.flow_label_2_valid, false);
  EXPECT_EQ(this->response_.flow_label_3_valid, false);
  EXPECT_EQ(this->response_.flow_label_4_valid, false);
  constexpr double kFalconUnitTimeUs = 0.131072;
  const uint32_t k1ms = std::round(1000 / kFalconUnitTimeUs);  // ~1ms
  EXPECT_EQ(this->response_.retransmit_timeout, k1ms);

  // Test an event for a non-multipath connection.
  this->event_.multipath_enable = false;
  bypass->Process(this->event_, this->response_, now);
  EXPECT_EQ(this->response_.flow_id, 0);
}
