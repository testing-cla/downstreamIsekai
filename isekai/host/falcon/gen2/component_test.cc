#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>

#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/constants.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_test_infrastructure.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/gen2/falcon_utils.h"

// Tests in this file are for Gen2 only.
namespace isekai {
namespace {

constexpr uint8_t kDegreeOfMultipathing = 4;

// Test a group of unsolicited writes, where the 1st and 129th requests are
// dropped.
TEST_P(FalconComponentTest, TestGen2MultipleUnsolicitedWriteDrop) {
  const int num_message = 135;
  const int second_dropped_packet_rsn = 128;
  // In this test, we want to set ack_coalescing time to be higher than network
  // delay to get a simpler packet deliver sequence.
  int ack_coalescing_num_ack = 100;
  int ack_coalescing_ns = 15000;
  absl::Duration kAckCoalescingTimer = absl::Nanoseconds(ack_coalescing_ns);
  absl::Duration inter_host_rx_scheduler_tick =
      absl::Nanoseconds(std::get<1>(GetParam()));

  FalconConfig falcon_config =
      DefaultConfigGenerator::DefaultFalconConfig(GetFalconVersion());
  // Set cwnd to 256 so that the sender can send more packets than ack packet
  // bitmap can carry information for (128 for data).
  falcon_config.mutable_rue()->set_initial_fcwnd(256);
  falcon_config.mutable_rue()->set_initial_ncwnd(256);
  falcon_config.mutable_ack_coalescing_thresholds()->set_count(
      ack_coalescing_num_ack);
  falcon_config.mutable_ack_coalescing_thresholds()->set_timeout_ns(
      ack_coalescing_ns);
  // Initialize both the Falcon hosts.
  InitFalconHosts(falcon_config);

  // Create and submit RDMA Write request from host1 to host2.
  SubmitMultipleUnsolicitedWriteRequests(0, num_message);

  // Drop the 1st and 129th requests.
  int dest_cid = network_->Peek()->falcon.dest_cid;
  network_->Drop();
  network_->DropByRsn(dest_cid, second_dropped_packet_rsn);
  // Deliver all non-dropped request packets to host2 (target)
  network_->DeliverAll(kNetworkDelay);
  // Wait enough time so that
  // 1) packets are all delivered and processed, and
  // 2) an ack is generated (reaching ack_coalescing timer).
  absl::Duration waiting_time = std::max(kAckCoalescingTimer, kNetworkDelay) +
                                kFalconProcessingDelay * (num_message - 2);
  env_->RunFor(waiting_time);
  // We expect to have one Eack generated
  EXPECT_EQ(network_->HeldPacketCount(), 1);
  auto first_eack = network_->Peek();
  EXPECT_EQ(first_eack->packet_type, falcon::PacketType::kAck);
  EXPECT_EQ(first_eack->ack.received_bitmap.FirstHoleIndex(), 0);

  // Deliver the eack from Host2 to Host1.
  network_->Deliver(kNetworkDelay);
  env_->RunFor(kNetworkDelay + kFalconProcessingDelay);
  // Host1 Falcon receives the Eack and retransmits the 1st packet.
  EXPECT_EQ(network_->HeldPacketCount(), 1);
  auto retransmitted_packet = network_->Peek();
  EXPECT_EQ(retransmitted_packet->packet_type,
            falcon::PacketType::kPushUnsolicitedData);
  EXPECT_EQ(retransmitted_packet->rdma.rsn, 0);

  // Deliver the retransmitted packet.
  network_->Deliver(kNetworkDelay);
  env_->RunFor(kNetworkDelay + kFalconProcessingDelay);
  // Host2 delivers all packets before second_dropped_packet_id to RDMA.
  TargetFalconDeliverPacketsToRdma(second_dropped_packet_rsn,
                                   inter_host_rx_scheduler_tick);
  // Wait for ack coalescing timeout to guarantee there is at least one ACK is
  // generated.
  env_->RunFor(kAckCoalescingTimer + kFalconProcessingDelay);

  // We expect to have EACK generated for the 129th packet. Host2 might generate
  // multiple ACKs due to based on AR-bit configuration.
  int num_ack = network_->HeldPacketCount();
  EXPECT_GT(num_ack, 0);
  // Deliver all packets from Host2 to Host1 and let it process.
  network_->DeliverAll(kNetworkDelay);
  env_->RunFor(kNetworkDelay + kFalconProcessingDelay * num_ack);

  // RDMA on host1 should get completions from Falcon. First 128 packets should
  // have been delivered.
  absl::StatusOr<std::pair<QpId, uint32_t>> completion_status;
  for (int i = kRsn; i < second_dropped_packet_rsn; ++i) {
    completion_status = host1_->GetCompletion();
    ASSERT_OK(completion_status);
    EXPECT_EQ(completion_status.value().first, kHost1QpId);
    EXPECT_EQ(completion_status.value().second, i);
  }

  // The retransmitted packet for 129th packet should be out.
  EXPECT_EQ(network_->HeldPacketCount(), 1);
  // Deliver all retransmitted packets (129th) to Host2.
  network_->DeliverAll(kNetworkDelay);
  env_->RunFor(kNetworkDelay + kFalconProcessingDelay);

  // Host2 delivers all packets >= second_dropped_packet_id to RDMA.
  TargetFalconDeliverPacketsToRdma(num_message - second_dropped_packet_rsn,
                                   inter_host_rx_scheduler_tick);
  env_->RunFor(kAckCoalescingTimer + kFalconProcessingDelay);

  // Host2 send acks back to Host1.
  EXPECT_GT(network_->HeldPacketCount(), 0);
  const Packet* p = network_->Peek();
  ASSERT_EQ(p->packet_type, falcon::PacketType::kAck);
  // Deliver ack from host2 to host1, and let it process.
  network_->DeliverAll(kNetworkDelay);
  env_->RunFor(kNetworkDelay);
  // Complete all packets at Host1.
  for (int i = second_dropped_packet_rsn; i < num_message; ++i) {
    env_->RunFor(kFalconProcessingDelay);
    completion_status = host1_->GetCompletion();
    ASSERT_OK(completion_status);
    EXPECT_EQ(completion_status.value().first, kHost1QpId);
    EXPECT_EQ(completion_status.value().second, i);
  }
}

// Test the behavior of bursty drops, where all packets within ack packet data
// window size (128) are dropped, together with one extra drop out of ack bitmap
// range.
TEST_P(FalconComponentTest, TestGen2UnsolicitedWriteBurstDropData) {
  const int num_message = 135;
  // When dropping 128 or 129 lead to different loss recovery behavior.
  // Both cases needs RTO so that rsn = 0 can be retransmitted. After rsn = 0
  // retx reaches the target:
  // 1) if last dropped rsn = 127, target sends back an EACK. 125 packets get
  //    retransmistted. It needs 2 RTT to retransmit all dropped packets, but no
  //    spurious retransmission.
  // 2) if last dropped rsn = 128, target sends back an ACK. All packets inside
  //    tx window will be retransmitted. it needs only 1 RTT to retransmit all
  //    dropped packets, but will have spurious retransmission (can be up to 127
  //    depending on the num_messages we send.)
  const int last_dropped_packet_rsn = 128;
  // In this test, we want to set ack_coalescing time to be higher than network
  // delay to get a simpler packet deliver sequence.
  int ack_coalescing_num_ack = 100;
  int ack_coalescing_ns = 15000;
  absl::Duration kAckCoalescingTimer = absl::Nanoseconds(ack_coalescing_ns);
  absl::Duration inter_host_rx_scheduler_tick =
      absl::Nanoseconds(std::get<1>(GetParam()));

  FalconConfig falcon_config =
      DefaultConfigGenerator::DefaultFalconConfig(GetFalconVersion());
  // Set cwnd to 256 so that the sender can send more packets than ack packet
  // bitmap can carry information for (128 for data).
  falcon_config.mutable_rue()->set_initial_fcwnd(256);
  falcon_config.mutable_rue()->set_initial_ncwnd(256);
  falcon_config.mutable_ack_coalescing_thresholds()->set_count(
      ack_coalescing_num_ack);
  falcon_config.mutable_ack_coalescing_thresholds()->set_timeout_ns(
      ack_coalescing_ns);
  // Initialize both the Falcon hosts.
  InitFalconHosts(falcon_config);

  // Create and submit RDMA Write request from host1 to host2.
  SubmitMultipleUnsolicitedWriteRequests(0, num_message);

  // Drop the 1st to 129th  requests.
  network_->DropMultipleWithIndexRange(0, last_dropped_packet_rsn + 1);
  // Deliver all the non-dropped packets.
  int num_remaining = num_message - last_dropped_packet_rsn - 1;
  EXPECT_EQ(network_->HeldPacketCount(), num_remaining);
  network_->DeliverAll(kNetworkDelay);
  env_->RunFor(kNetworkDelay + num_remaining * kFalconProcessingDelay);

  // After ack_coalescing timeout, Host 2 will generate an EACK. The bitmaps in
  // the EACK will be all-zeros since the data bitmap is only 128-bit and none
  // of the first 129 data packets gets delivered.
  env_->RunFor(kAckCoalescingTimer + kFalconProcessingDelay);
  EXPECT_EQ(network_->HeldPacketCount(), 1);
  EXPECT_EQ(network_->Peek()->packet_type, falcon::PacketType::kAck);
  // Deliver the ACK packet. Since the bitmap is all zero, it will not satisfy
  // OOO distance criteria so no retransmission will happen.
  network_->Deliver(kNetworkDelay);
  env_->RunFor(kNetworkDelay + kFalconProcessingDelay);
  EXPECT_EQ(network_->HeldPacketCount(), 0);

  // Wait for retransmission timeout. At this time, only the 1st packet will be
  // retransmitted since the 2nd packet has not reached the RTO yet.
  env_->RunFor(kRetransmissionDelay - kAckCoalescingTimer - kNetworkDelay -
               2 * kFalconProcessingDelay);
  EXPECT_EQ(network_->HeldPacketCount(), 1);
  // Deliver the retransmitted 1st packet to host2 and let it process.
  network_->Deliver(kNetworkDelay);
  env_->RunFor(kNetworkDelay + kFalconProcessingDelay);

  // Host2 receives the 1st packet and transmits an ack packet.
  std::unique_ptr<Packet> received_packet =
      std::move(host2_->GetSubmittedTransaction().value());
  host2_->SubmitAckTransaction(kHost2Scid, received_packet->rdma.rsn,
                               Packet::Syndrome::kAck);
  env_->RunFor(kAckCoalescingTimer + kFalconProcessingDelay);
  ASSERT_EQ(network_->HeldPacketCount(), 1);
  ASSERT_EQ(network_->Peek()->packet_type, falcon::PacketType::kAck);

  // Deliver ack from host2 to host1, and let it process.
  network_->Deliver(kNetworkDelay);
  env_->RunFor(kNetworkDelay + kFalconProcessingDelay);

  // Host1 should be able to complete the 1st packet.
  auto completion_status = host1_->GetCompletion();
  ASSERT_OK(completion_status);
  EXPECT_EQ(completion_status.value().first, kHost1QpId);
  EXPECT_EQ(completion_status.value().second, 0);

  // After the 1st packet gets acked. All other packets should be sent out.
  // Even though we only retransmitted the 1st packet, all packets in the
  // tx_window are now all RTO retx eligible, so all packets will be
  // retransmitted. This contains spurious retransmission, but is behaving as
  // designed.
  env_->RunFor((kPerConnectionInterOpGap + kFalconProcessingDelay) *
               last_dropped_packet_rsn);
  EXPECT_EQ(network_->HeldPacketCount(), num_message - 1);
  for (int i = 0; i < num_message - 1; ++i) {
    EXPECT_EQ(network_->PeekAt(i)->packet_type,
              falcon::PacketType::kPushUnsolicitedData);
    EXPECT_EQ(network_->PeekAt(i)->rdma.rsn, i + 1);
  }
  // Deliver all retransmitted packets to Host2.
  network_->DeliverAll(kNetworkDelay);
  env_->RunFor(kNetworkDelay +
               (kFalconProcessingDelay + kPerConnectionInterOpGap) *
                   (num_message - 1));
  TargetFalconDeliverPacketsToRdma(num_message - 1,
                                   inter_host_rx_scheduler_tick);
  // Wait at least for one ack_coalescing time so that at least one ack is
  // generated.
  env_->RunFor(kAckCoalescingTimer + kFalconProcessingDelay);
  EXPECT_GT(network_->HeldPacketCount(), 0);
  const Packet* p = network_->Peek();
  ASSERT_EQ(p->packet_type, falcon::PacketType::kAck);

  // Deliver all acks from host2 to host1, and let it process.
  network_->DeliverAll(kNetworkDelay);
  env_->RunFor(kNetworkDelay);
  // Host 1 should get completions of all packets.
  for (int i = 1; i < num_message; ++i) {
    env_->RunFor(kFalconProcessingDelay);
    completion_status = host1_->GetCompletion();
    ASSERT_OK(completion_status);
    EXPECT_EQ(completion_status.value().first, kHost1QpId);
    EXPECT_EQ(completion_status.value().second, i);
  }
}

// Tests a successful end-to-end message delivery with Gen2 multipathing and
// with no drops. The packets should be transmitted in a round-robin manner
// across all 4 flows in the connection, and the ACKs should use the same flow
// label as the initial packets.
TEST_P(FalconComponentTest, TestGen2MultipathingNoDrops) {
  constexpr int kNumMessages = 32;
  // In this test, we want to set ack_coalescing time to be higher than network
  // delay to get a simpler packet deliver sequence.
  constexpr int kAckCoalescingNumAck = 100;  // more than kNumMessages
  constexpr int kAckCoalescingNs = 15000;    // 15us
  absl::Duration kAckCoalescingTimer = absl::Nanoseconds(kAckCoalescingNs);
  absl::Duration inter_host_rx_scheduler_tick =
      absl::Nanoseconds(std::get<1>(GetParam()));

  FalconConfig falcon_config =
      DefaultConfigGenerator::DefaultFalconConfig(GetFalconVersion());
  falcon_config.mutable_ack_coalescing_thresholds()->set_count(
      kAckCoalescingNumAck);
  falcon_config.mutable_ack_coalescing_thresholds()->set_timeout_ns(
      kAckCoalescingNs);
  falcon_config.mutable_gen2_config_options()
      ->mutable_multipath_config()
      ->set_path_selection_policy(
          FalconConfig::Gen2ConfigOptions::MultipathConfig::ROUND_ROBIN);
  falcon_config.set_enable_ack_request_bit(false);
  // Initialize both the Falcon hosts.
  InitFalconHosts(falcon_config, kDegreeOfMultipathing);

  // Create and submit RDMA Write request from host1 to host2.
  SubmitMultipleUnsolicitedWriteRequests(kRsn, kNumMessages);
  // Inspect the packets in the network queue.
  for (int i = 0; i < kNumMessages; ++i) {
    const Packet* p = network_->PeekAt(i);
    ASSERT_EQ(p->packet_type, falcon::PacketType::kPushUnsolicitedData);
    // Expect flow labels in round-robin fashion across the 4 flows.
    ASSERT_EQ(
        GetFlowIdFromFlowLabel(p->metadata.flow_label, kDegreeOfMultipathing),
        i % kDegreeOfMultipathing);
    EXPECT_EQ(p->rdma.rsn, i);
  }

  // Deliver all data packets to host2 (target).
  network_->DeliverAll(kNetworkDelay);
  env_->RunFor(kNetworkDelay);
  TargetFalconDeliverPacketsToRdma(kNumMessages, inter_host_rx_scheduler_tick);
  // Wait enough time so that an ACK is generated (reaching ack_coalescing
  // timer).
  env_->RunFor(kAckCoalescingTimer);

  // We expect to have 4 ACKs generated, one for each flow.
  EXPECT_EQ(network_->HeldPacketCount(), 4);
  for (int i = 0; i < network_->HeldPacketCount(); ++i) {
    const Packet* p = network_->PeekAt(i);
    ASSERT_EQ(p->packet_type, falcon::PacketType::kAck);
    ASSERT_EQ(
        GetFlowIdFromFlowLabel(p->metadata.flow_label, kDegreeOfMultipathing),
        i % kDegreeOfMultipathing);
    ASSERT_EQ(p->ack.rdbpsn, kNumMessages);
  }

  // Deliver all the ACKs from Host2 to Host1.
  network_->DeliverAll(kNetworkDelay);
  env_->RunFor(kNetworkDelay + 4 * kFalconProcessingDelay);

  // RDMA on host1 should get completions from Falcon.
  absl::StatusOr<std::pair<QpId, uint32_t>> completion_status;
  for (int i = 0; i < kNumMessages; ++i) {
    completion_status = host1_->GetCompletion();
    ASSERT_OK(completion_status);
    EXPECT_EQ(completion_status.value().first, kHost1QpId);
    EXPECT_EQ(completion_status.value().second, i);
  }
}

// Tests a successful end-to-end message delivery with Gen2 multipathing and
// with drops. The packets should be transmitted in a round-robin manner
// across all 4 flows in the connection, and the ACKs should use the same
// flow label as the initial packets.
TEST_P(FalconComponentTest, TestGen2MultipathingWithDrops) {
  const int num_message = 32;
  // In this test, we want to set ack_coalescing time to be higher than network
  // delay to get a simpler packet deliver sequence.
  int ack_coalescing_num_ack = 100;  // more than kNumMessages
  int ack_coalescing_ns = 15000;     // 15us
  absl::Duration kAckCoalescingTimer = absl::Nanoseconds(ack_coalescing_ns);
  absl::Duration inter_host_rx_scheduler_tick =
      absl::Nanoseconds(std::get<1>(GetParam()));

  FalconConfig falcon_config =
      DefaultConfigGenerator::DefaultFalconConfig(GetFalconVersion());
  falcon_config.mutable_ack_coalescing_thresholds()->set_count(
      ack_coalescing_num_ack);
  falcon_config.mutable_ack_coalescing_thresholds()->set_timeout_ns(
      ack_coalescing_ns);
  falcon_config.mutable_gen2_config_options()
      ->mutable_multipath_config()
      ->set_path_selection_policy(
          FalconConfig::Gen2ConfigOptions::MultipathConfig::ROUND_ROBIN);
  falcon_config.set_enable_ack_request_bit(false);
  // Initialize both the Falcon hosts.
  InitFalconHosts(falcon_config, kDegreeOfMultipathing);

  // Create and submit RDMA Write request from host1 to host2.
  SubmitMultipleUnsolicitedWriteRequests(kRsn, num_message);
  for (int i = 0; i < num_message; ++i) {
    const Packet* p = network_->PeekAt(i);
    ASSERT_EQ(p->packet_type, falcon::PacketType::kPushUnsolicitedData);
    ASSERT_EQ(
        GetFlowIdFromFlowLabel(p->metadata.flow_label, kDegreeOfMultipathing),
        i % kDegreeOfMultipathing);
    EXPECT_EQ(p->rdma.rsn, i);
  }

  // Deliver all data packets to host2 (target).
  network_->Drop();
  network_->DeliverAll(kNetworkDelay);
  // Wait enough time so that
  // 1) Packets are all delivered and processed, and
  // 2) An ACK is generated (reaching ack_coalescing timer).
  env_->RunFor(kNetworkDelay + num_message * kFalconProcessingDelay +
               kAckCoalescingTimer);

  // We expect to have 4 EACKs generated, one for each flow.
  EXPECT_EQ(network_->HeldPacketCount(), 4);
  for (int i = 0; i < network_->HeldPacketCount(); ++i) {
    const Packet* p = network_->PeekAt(i);
    ASSERT_EQ(p->packet_type, falcon::PacketType::kAck);
    EXPECT_EQ(p->ack.received_bitmap.FirstHoleIndex(),
              0);  // First packet in the window was dropped.
    EXPECT_EQ(
        GetFlowIdFromFlowLabel(p->metadata.flow_label, kDegreeOfMultipathing),
        (i + 1) % kDegreeOfMultipathing);
    EXPECT_EQ(p->ack.rdbpsn, 0);
  }

  // Deliver the 4 EACKs from Host2 to Host1.
  network_->DeliverAll(kNetworkDelay);
  env_->RunFor(kNetworkDelay + 4 * kFalconProcessingDelay);

  // Host1 Falcon receives the EACK and retransmits the 1st packet.
  EXPECT_EQ(network_->HeldPacketCount(), 1);
  auto retransmitted_packet = network_->Peek();
  EXPECT_EQ(retransmitted_packet->packet_type,
            falcon::PacketType::kPushUnsolicitedData);
  EXPECT_EQ(GetFlowIdFromFlowLabel(retransmitted_packet->metadata.flow_label,
                                   kDegreeOfMultipathing),
            0);
  EXPECT_EQ(retransmitted_packet->rdma.rsn, 0);

  // Deliver the retx packet from Host 1 to Host 2.
  network_->DeliverAll(kNetworkDelay);
  env_->RunFor(kNetworkDelay + kFalconProcessingDelay);
  TargetFalconDeliverPacketsToRdma(num_message, inter_host_rx_scheduler_tick);

  // Host2 Falcon generates the last ACK.
  env_->RunFor(kAckCoalescingTimer);
  EXPECT_EQ(network_->HeldPacketCount(), 1);
  auto last_ack = network_->Peek();
  EXPECT_EQ(last_ack->packet_type, falcon::PacketType::kAck);
  EXPECT_EQ(last_ack->ack.rdbpsn, num_message);
  // Last ACK belongs to flow ID 0 which the retx packet belongs to.
  EXPECT_EQ(GetFlowIdFromFlowLabel(last_ack->metadata.flow_label,
                                   kDegreeOfMultipathing),
            0);

  // Deliver the last ACK from Host2 to Host1.
  network_->DeliverAll(kNetworkDelay);
  env_->RunFor(kNetworkDelay + num_message * kFalconProcessingDelay);

  // RDMA on host1 should get completions from Falcon.
  absl::StatusOr<std::pair<QpId, uint32_t>> completion_status;
  for (int i = 0; i < num_message; ++i) {
    completion_status = host1_->GetCompletion();
    ASSERT_OK(completion_status);
    EXPECT_EQ(completion_status.value().first, kHost1QpId);
    EXPECT_EQ(completion_status.value().second, i);
  }
}

INSTANTIATE_TEST_SUITE_P(
    ComponentTests, FalconComponentTest,
    testing::Combine(
        testing::Values(FalconConfig::FIRST_PHASE_RESERVATION,
                        FalconConfig::DELAYED_RESERVATION,
                        FalconConfig::ON_DEMAND_RESERVATION),
        /*inter_host_rx_scheduler_tick_ns=*/testing::Values(0, 3, 5),
        /*version=*/testing::Values(2),
        /*connection_scheduler_variant=*/
        testing::Values(FalconConfig::BUSY_POLL_SCHEDULER,
                        FalconConfig::EVENT_BASED_SCHEDULER)),
    [](const testing::TestParamInfo<FalconComponentTest::ParamType>& info) {
      const FalconConfig::ResourceReservationMode rsc_reservation_mode =
          std::get<0>(info.param);
      const int rx_tick_ns = std::get<1>(info.param);
      const int version = std::get<2>(info.param);
      const int conn_sched_variant = static_cast<int>(std::get<3>(info.param));
      return absl::StrCat("RscMode", rsc_reservation_mode, "_RxTick",
                          rx_tick_ns, "_Gen", version, "_ConnSched",
                          conn_sched_variant);
    });

}  // namespace
}  // namespace isekai
