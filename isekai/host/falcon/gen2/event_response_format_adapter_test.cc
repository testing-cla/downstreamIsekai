#include "isekai/host/falcon/event_response_format_adapter.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>

#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "internal/testing.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_model.h"
#include "isekai/host/falcon/falcon_testing_helpers.h"
#include "isekai/host/falcon/gen2/falcon_types.h"
#include "isekai/host/falcon/rue/bits.h"
#include "isekai/host/falcon/rue/fixed.h"
#include "isekai/host/falcon/rue/format.h"
#include "isekai/host/falcon/rue/format_bna.h"

namespace isekai {
namespace {

constexpr int kFalconVersion2 = 2;
constexpr uint32_t kCid1 = 123;
constexpr uint32_t kCid2 = 234;
constexpr uint8_t kFlowId0 = 0;
constexpr uint8_t kFlowId1 = 1;
constexpr uint32_t kFlowLabel0 = 0x120 | kFlowId0;
constexpr uint32_t kFlowLabel1 = 0x120 | kFlowId1;

void PopulateNackPacket(Packet* packet) {
  packet->nack.code = falcon::NackCode::kRxResourceExhaustion;
  packet->nack.timestamp_1 = absl::Microseconds(1);
  packet->nack.timestamp_2 = absl::Microseconds(2);
  packet->timestamps.sent_timestamp = absl::Microseconds(3);
  packet->timestamps.received_timestamp = absl::Microseconds(4);
  packet->nack.forward_hops = 1;
  packet->nack.rx_buffer_level = 3;
  packet->nack.cc_metadata = 2;
}

void PopulateAckPacket(Packet* packet) {
  packet->ack.timestamp_1 = absl::Microseconds(1);
  packet->ack.timestamp_2 = absl::Microseconds(2);
  packet->timestamps.sent_timestamp = absl::Microseconds(3);
  packet->timestamps.received_timestamp = absl::Microseconds(4);
  packet->ack.forward_hops = 1;
  packet->ack.rx_buffer_level = 3;
  packet->ack.cc_metadata = 2;
}

// Test fixture for the Gen2 EventResponseFormatAdapter class.
class Gen2FormatAdapterTest : public FalconTestingHelpers::FalconTestSetup,
                              public ::testing::Test {
 protected:
  void SetUp() override {
    FalconConfig config =
        DefaultConfigGenerator::DefaultFalconConfig(kFalconVersion2);
    InitFalcon(config);
    ConnectionState::ConnectionMetadata metadata =
        FalconTestingHelpers::InitializeConnectionMetadata(falcon_.get(),
                                                           kCid1);
    connection_state_single_path_ =
        FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                        metadata);
    // Multipath connection with degree_of_multipathing = 4.
    metadata.degree_of_multipathing = 4;
    metadata.scid = kCid2;
    connection_state_multipath_ =
        FalconTestingHelpers::InitializeConnectionState(falcon_.get(),
                                                        metadata);
    format_adapter_ = std::make_unique<EventResponseFormatAdapter<
        falcon_rue::Event_BNA, falcon_rue::Response_BNA>>(falcon_.get());
  }

  std::unique_ptr<EventResponseFormatAdapter<falcon_rue::Event_BNA,
                                             falcon_rue::Response_BNA>>
      format_adapter_;
  ConnectionState* connection_state_multipath_;
  ConnectionState* connection_state_single_path_;
};

// Tests that the RTO event is populated properly.
TEST_F(Gen2FormatAdapterTest, FillTimeoutRetransmittedEvent) {
  falcon_rue::Event_BNA event;
  memset(&event, 0, sizeof(falcon_rue::Event_BNA));
  Gen2RueKey rue_key(kCid1, kFlowId1);
  Packet packet;
  packet.metadata.flow_label = kFlowLabel1;

  format_adapter_->FillTimeoutRetransmittedEvent(
      event, &rue_key, &packet,
      connection_state_multipath_->congestion_control_metadata, 4);

  ConnectionState::CongestionControlMetadata& ccmeta =
      connection_state_multipath_->congestion_control_metadata;
  EXPECT_EQ(event.connection_id, rue_key.scid);
  EXPECT_EQ(event.flow_label, packet.metadata.flow_label);
  EXPECT_EQ(event.multipath_enable, true);
  EXPECT_EQ(event.event_type, falcon::RueEventType::kRetransmit);
  EXPECT_EQ(event.retransmit_count, 4);
  EXPECT_EQ(event.retransmit_reason, falcon::RetransmitReason::kTimeout);
  EXPECT_EQ(event.nack_code, falcon::NackCode::kNotANack);
  EXPECT_EQ(event.timestamp_1, 0);
  EXPECT_EQ(event.timestamp_2, 0);
  EXPECT_EQ(event.timestamp_3, 0);
  EXPECT_EQ(event.timestamp_4, 0);
  EXPECT_EQ(event.forward_hops, 0);
  EXPECT_EQ(event.rx_buffer_level, 0);
  EXPECT_EQ(event.cc_metadata, 0);
  EXPECT_EQ(event.num_packets_acked, 0);
  EXPECT_EQ(event.event_queue_select, 0);
  EXPECT_EQ(event.fabric_congestion_window, ccmeta.fabric_congestion_window);
  EXPECT_EQ(event.nic_congestion_window,
            (falcon_rue::UintToFixed<uint32_t, uint32_t>(
                ccmeta.nic_congestion_window, falcon_rue::kFractionalBits)));
  EXPECT_EQ(event.delay_select, ccmeta.delay_select);
  EXPECT_EQ(event.fabric_window_time_marker, ccmeta.fabric_window_time_marker);
  EXPECT_EQ(event.nic_window_time_marker, ccmeta.nic_window_time_marker);
  EXPECT_EQ(event.base_delay, ccmeta.base_delay);
  EXPECT_EQ(event.delay_state, ccmeta.delay_state);
  EXPECT_EQ(event.rtt_state, ccmeta.rtt_state);
  EXPECT_EQ(event.cc_opaque, ccmeta.cc_opaque);
  EXPECT_EQ(event.eack, false);
  EXPECT_EQ(event.eack_drop, false);
  EXPECT_EQ(event.eack_own, 0);
  EXPECT_EQ(event.reserved_0, 0);
  EXPECT_EQ(event.reserved_1, 0);
  EXPECT_EQ(event.gen_bit, 0);
  EXPECT_EQ(event.plb_state, ccmeta.gen2_plb_state);

  // Testing the FillTimeoutRetransmittedEvent() function with the single path
  // connection.
  rue_key = Gen2RueKey(
      connection_state_single_path_->connection_metadata.scid,
      kFlowId0);  // can only use kFlowId0 for single path connections.
  packet.metadata.flow_label = kFlowLabel0;
  format_adapter_->FillTimeoutRetransmittedEvent(
      event, &rue_key, &packet,
      connection_state_single_path_->congestion_control_metadata, 4);
  ccmeta = connection_state_single_path_->congestion_control_metadata;
  EXPECT_EQ(event.connection_id, rue_key.scid);
  EXPECT_EQ(event.flow_label, packet.metadata.flow_label);
  EXPECT_EQ(event.multipath_enable, false);
}

// Tests that the NACK event is populated properly.
TEST_F(Gen2FormatAdapterTest, FillNackEvent) {
  falcon_rue::Event_BNA event;
  memset(&event, 0, sizeof(falcon_rue::Event_BNA));
  Gen2RueKey rue_key(kCid1, kFlowId1);
  Packet packet;
  PopulateNackPacket(&packet);
  packet.metadata.flow_label = kFlowLabel1;

  format_adapter_->FillNackEvent(
      event, &rue_key, &packet,
      connection_state_multipath_->congestion_control_metadata, 4);

  ConnectionState::CongestionControlMetadata& ccmeta =
      connection_state_multipath_->congestion_control_metadata;
  EXPECT_EQ(event.connection_id, rue_key.scid);
  EXPECT_EQ(event.flow_label, packet.metadata.flow_label);
  EXPECT_EQ(event.multipath_enable, true);
  EXPECT_EQ(event.event_type, falcon::RueEventType::kNack);
  EXPECT_EQ(event.timestamp_1,
            falcon_->get_rate_update_engine()->ToFalconTimeUnits(
                packet.nack.timestamp_1));
  EXPECT_EQ(event.timestamp_2,
            falcon_->get_rate_update_engine()->ToFalconTimeUnits(
                packet.nack.timestamp_2));
  EXPECT_EQ(event.timestamp_3,
            falcon_->get_rate_update_engine()->ToFalconTimeUnits(
                packet.timestamps.sent_timestamp));
  EXPECT_EQ(event.timestamp_4,
            falcon_->get_rate_update_engine()->ToFalconTimeUnits(
                packet.timestamps.received_timestamp));
  EXPECT_EQ(event.retransmit_count, 0);
  EXPECT_EQ(event.retransmit_reason, falcon::RetransmitReason::kEarly);
  EXPECT_EQ(event.nack_code, packet.nack.code);
  EXPECT_EQ(event.forward_hops, packet.nack.forward_hops);
  EXPECT_EQ(event.rx_buffer_level, packet.nack.rx_buffer_level);
  EXPECT_EQ(event.cc_metadata, packet.nack.cc_metadata);
  EXPECT_EQ(event.num_packets_acked, 4);
  EXPECT_EQ(event.event_queue_select, 0);
  EXPECT_EQ(event.fabric_congestion_window, ccmeta.fabric_congestion_window);
  EXPECT_EQ(event.nic_congestion_window,
            (falcon_rue::UintToFixed<uint32_t, uint32_t>(
                ccmeta.nic_congestion_window, falcon_rue::kFractionalBits)));
  EXPECT_EQ(event.delay_select, ccmeta.delay_select);
  EXPECT_EQ(event.fabric_window_time_marker, ccmeta.fabric_window_time_marker);
  EXPECT_EQ(event.nic_window_time_marker, ccmeta.nic_window_time_marker);
  EXPECT_EQ(event.base_delay, ccmeta.base_delay);
  EXPECT_EQ(event.delay_state, ccmeta.delay_state);
  EXPECT_EQ(event.rtt_state, ccmeta.rtt_state);
  EXPECT_EQ(event.cc_opaque, ccmeta.cc_opaque);
  EXPECT_EQ(event.eack, false);
  EXPECT_EQ(event.eack_drop, false);
  EXPECT_EQ(event.eack_own, 0);
  EXPECT_EQ(event.reserved_0, 0);
  EXPECT_EQ(event.reserved_1, 0);
  EXPECT_EQ(event.gen_bit, 0);
  EXPECT_EQ(event.plb_state, ccmeta.gen2_plb_state);

  // Testing the FillNackEvent() function with the single path connection.
  rue_key = Gen2RueKey(
      connection_state_single_path_->connection_metadata.scid,
      kFlowId0);  // can only use kFlowId0 for single path connections.
  packet.metadata.flow_label = kFlowLabel0;
  format_adapter_->FillNackEvent(
      event, &rue_key, &packet,
      connection_state_single_path_->congestion_control_metadata, 4);
  ccmeta = connection_state_single_path_->congestion_control_metadata;
  EXPECT_EQ(event.connection_id, rue_key.scid);
  EXPECT_EQ(event.flow_label, packet.metadata.flow_label);
  EXPECT_EQ(event.multipath_enable, false);
}

// Tests that the explicit ACK event is populated properly.
TEST_F(Gen2FormatAdapterTest, FillExplicitAckEvent) {
  falcon_rue::Event_BNA event;
  memset(&event, 0, sizeof(falcon_rue::Event_BNA));
  Gen2RueKey rue_key(kCid1, kFlowId1);
  Packet packet;
  PopulateAckPacket(&packet);
  packet.metadata.flow_label = kFlowLabel1;

  format_adapter_->FillExplicitAckEvent(
      event, &rue_key, &packet,
      connection_state_multipath_->congestion_control_metadata, 4, true, false);

  ConnectionState::CongestionControlMetadata& ccmeta =
      connection_state_multipath_->congestion_control_metadata;
  EXPECT_EQ(event.connection_id, rue_key.scid);
  EXPECT_EQ(event.flow_label, packet.metadata.flow_label);
  EXPECT_EQ(event.multipath_enable, true);
  EXPECT_EQ(event.event_type, falcon::RueEventType::kAck);
  EXPECT_EQ(event.timestamp_1,
            falcon_->get_rate_update_engine()->ToFalconTimeUnits(
                packet.ack.timestamp_1));
  EXPECT_EQ(event.timestamp_2,
            falcon_->get_rate_update_engine()->ToFalconTimeUnits(
                packet.ack.timestamp_2));
  EXPECT_EQ(event.timestamp_3,
            falcon_->get_rate_update_engine()->ToFalconTimeUnits(
                packet.timestamps.sent_timestamp));
  EXPECT_EQ(event.timestamp_4,
            falcon_->get_rate_update_engine()->ToFalconTimeUnits(
                packet.timestamps.received_timestamp));
  EXPECT_EQ(event.retransmit_count, 0);
  EXPECT_EQ(event.retransmit_reason, falcon::RetransmitReason::kEarly);
  EXPECT_EQ(event.nack_code, falcon::NackCode::kNotANack);
  EXPECT_EQ(event.forward_hops, packet.ack.forward_hops);
  EXPECT_EQ(event.rx_buffer_level, packet.ack.rx_buffer_level);
  EXPECT_EQ(event.cc_metadata, packet.ack.cc_metadata);
  EXPECT_EQ(event.num_packets_acked, 4);
  EXPECT_EQ(event.event_queue_select, 0);
  EXPECT_EQ(event.fabric_congestion_window, ccmeta.fabric_congestion_window);
  EXPECT_EQ(event.nic_congestion_window,
            (falcon_rue::UintToFixed<uint32_t, uint32_t>(
                ccmeta.nic_congestion_window, falcon_rue::kFractionalBits)));
  EXPECT_EQ(event.delay_select, ccmeta.delay_select);
  EXPECT_EQ(event.fabric_window_time_marker, ccmeta.fabric_window_time_marker);
  EXPECT_EQ(event.nic_window_time_marker, ccmeta.nic_window_time_marker);
  EXPECT_EQ(event.base_delay, ccmeta.base_delay);
  EXPECT_EQ(event.delay_state, ccmeta.delay_state);
  EXPECT_EQ(event.rtt_state, ccmeta.rtt_state);
  EXPECT_EQ(event.cc_opaque, ccmeta.cc_opaque);
  EXPECT_EQ(event.eack, true);
  EXPECT_EQ(event.eack_drop, false);
  EXPECT_EQ(event.eack_own, 0);
  EXPECT_EQ(event.reserved_0, 0);
  EXPECT_EQ(event.reserved_1, 0);
  EXPECT_EQ(event.gen_bit, 0);
  EXPECT_EQ(event.plb_state, ccmeta.gen2_plb_state);

  // Testing the FillExplicitAckEvent() function with the single path
  // connection.
  rue_key = Gen2RueKey(
      connection_state_single_path_->connection_metadata.scid,
      kFlowId0);  // can only use kFlowId0 for single path connections.
  packet.metadata.flow_label = kFlowLabel0;
  format_adapter_->FillExplicitAckEvent(
      event, &rue_key, &packet,
      connection_state_single_path_->congestion_control_metadata, 4, true,
      false);
  ccmeta = connection_state_single_path_->congestion_control_metadata;
  EXPECT_EQ(event.connection_id, rue_key.scid);
  EXPECT_EQ(event.flow_label, packet.metadata.flow_label);
  EXPECT_EQ(event.multipath_enable, false);
}

// Tests the IsRandomizePath() function.
TEST_F(Gen2FormatAdapterTest, IsRandomizePath) {
  falcon_rue::Response_BNA response;
  memset(&response, 0, sizeof(falcon_rue::Response));
  EXPECT_EQ(format_adapter_->IsRandomizePath(&response), false);

  response.flow_label_1_valid = true;
  EXPECT_EQ(format_adapter_->IsRandomizePath(&response), true);

  memset(&response, 0, sizeof(falcon_rue::Response));
  response.flow_label_2_valid = true;
  EXPECT_EQ(format_adapter_->IsRandomizePath(&response), true);

  memset(&response, 0, sizeof(falcon_rue::Response));
  response.flow_label_3_valid = true;
  EXPECT_EQ(format_adapter_->IsRandomizePath(&response), true);

  memset(&response, 0, sizeof(falcon_rue::Response));
  response.flow_label_4_valid = true;
  EXPECT_EQ(format_adapter_->IsRandomizePath(&response), true);
}

// Tests that the connection Xoff metadata is updated as expected by
// the response.
TEST_F(Gen2FormatAdapterTest, UpdateConnectionStateFromResponseXoffMetadata) {
  falcon_rue::Response_BNA response;
  memset(&response, 0, sizeof(falcon_rue::Response));
  response.alpha_request = 1;
  response.alpha_response = 2;

  format_adapter_->UpdateConnectionStateFromResponse(
      connection_state_single_path_, &response);
  EXPECT_EQ(
      connection_state_single_path_->connection_xoff_metadata.alpha_request, 1);
  EXPECT_EQ(
      connection_state_single_path_->connection_xoff_metadata.alpha_response,
      2);
}

// Tests that the congestion control metadata is updated as expected by the
// response for Gen2 multipath connections.
TEST_F(Gen2FormatAdapterTest, UpdateConnectionStateFromResponseMultipath) {
  falcon_rue::Response_BNA response;
  memset(&response, 0, sizeof(falcon_rue::Response));
  response.flow_label_1 = kFlowLabel0;
  response.flow_label_1_valid = true;
  response.rtt_state = 1000;
  response.retransmit_timeout = 1200;
  response.nic_congestion_window = 3;  // in fixed format
  response.plb_state = 10;

  // Expect failure when all flow weights are zero.
  EXPECT_DEATH(format_adapter_->UpdateConnectionStateFromResponse(
                   connection_state_multipath_, &response),
               "");

  response.flow_label_2_weight = 1;

  format_adapter_->UpdateConnectionStateFromResponse(
      connection_state_multipath_, &response);
  EXPECT_EQ(connection_state_multipath_->congestion_control_metadata
                .gen2_flow_labels[0],
            kFlowLabel0);
  EXPECT_EQ(connection_state_multipath_->congestion_control_metadata
                .gen2_flow_weights[1],
            1);
  EXPECT_EQ(connection_state_multipath_->congestion_control_metadata
                .retransmit_timeout,
            falcon_->get_rate_update_engine()->FromFalconTimeUnits(1200));
  EXPECT_EQ(connection_state_multipath_->congestion_control_metadata.rtt_state,
            1000);
  EXPECT_EQ(
      connection_state_multipath_->congestion_control_metadata.gen2_plb_state,
      10);
  EXPECT_EQ(connection_state_multipath_->congestion_control_metadata
                .nic_congestion_window,
            std::max<uint32_t>(1, falcon_rue::FixedToUint<uint32_t, uint32_t>(
                                      response.nic_congestion_window,
                                      falcon_rue::kFractionalBits)));
}

// Tests that the congestion control metadata is updated as expected by the
// response for Gen2 single path connections.
TEST_F(Gen2FormatAdapterTest, UpdateConnectionStateFromResponseSinglePath) {
  falcon_rue::Response_BNA response;
  memset(&response, 0, sizeof(falcon_rue::Response));
  response.flow_label_1 = kFlowLabel0;
  response.flow_label_1_valid = true;
  response.flow_label_2 = kFlowLabel1;
  response.flow_label_2_valid = true;
  response.rtt_state = 1000;
  response.retransmit_timeout = 1200;
  response.nic_congestion_window = 3;  // in fixed format
  response.plb_state = 10;

  connection_state_single_path_->congestion_control_metadata
      .gen2_flow_labels[0] = 0;
  // No failure even when all flow weights are zero.
  format_adapter_->UpdateConnectionStateFromResponse(
      connection_state_single_path_, &response);
  EXPECT_EQ(connection_state_single_path_->congestion_control_metadata
                .gen2_flow_labels[0],
            kFlowLabel0);  // only flow label 0 is updated
  EXPECT_EQ(connection_state_single_path_->congestion_control_metadata
                .retransmit_timeout,
            falcon_->get_rate_update_engine()->FromFalconTimeUnits(1200));
  EXPECT_EQ(
      connection_state_single_path_->congestion_control_metadata.rtt_state,
      1000);
  EXPECT_EQ(
      connection_state_single_path_->congestion_control_metadata.gen2_plb_state,
      10);
  EXPECT_EQ(connection_state_single_path_->congestion_control_metadata
                .nic_congestion_window,
            std::max<uint32_t>(1, falcon_rue::FixedToUint<uint32_t, uint32_t>(
                                      response.nic_congestion_window,
                                      falcon_rue::kFractionalBits)));
}

}  // namespace
}  // namespace isekai
