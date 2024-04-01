#include "isekai/host/falcon/gen2/sram_dram_reorder_engine.h"

#include <sys/types.h>

#include <cstdint>

#include "absl/time/time.h"
#include "gtest/gtest.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_testing_helpers.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/gen2/falcon_model.h"
#include "isekai/host/falcon/gen2/falcon_types.h"

namespace isekai {

namespace {

class Gen2SramDramReorderEngineTest
    : public FalconTestingHelpers::FalconTestSetup,
      public ::testing::Test {};

TEST_F(Gen2SramDramReorderEngineTest, PrefetchBufferAccounting) {
  // Setup up the necessary Falcon configuration fields.
  constexpr int kFalconVersion = 2;
  constexpr int kPrefetchBufferSize = 250;
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(kFalconVersion);
  config.mutable_gen2_config_options()
      ->mutable_on_nic_dram_config()
      ->set_per_host_prefetch_buffer_size_bytes(kPrefetchBufferSize);
  InitFalcon(config);

  // Get a handle on the reorder engine.
  auto gen2falcon = static_cast<Gen2FalconModel*>(falcon_.get());
  auto reorder_engine = gen2falcon->get_sram_dram_reorder_engine();

  // Reorder engine should not be able to prefetch a packet size > buffer size.
  uint32_t packet_payload_size = kPrefetchBufferSize + 200;
  EXPECT_FALSE(reorder_engine->CanPrefetchData(1, packet_payload_size));

  // Reorder engine should be able to prefetch a packet size <= buffer size.
  packet_payload_size = kPrefetchBufferSize;
  EXPECT_TRUE(reorder_engine->CanPrefetchData(1, packet_payload_size));

  // Initialize and get a handle on the connection state with cid = 1.
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(
          falcon_.get(), /*scid=*/1, OrderingMode::kOrdered);
  auto connection_state = FalconTestingHelpers::InitializeConnectionState(
      falcon_.get(), connection_metadata);

  // Setup an incoming transaction with the necessary metadata.
  FalconTestingHelpers::SetupIncomingTransactionWithConnectionState(
      connection_state,
      /*transaction_type=*/TransactionType::kPushUnsolicited,
      /*transaction_location=*/TransactionLocation::kTarget,
      /*state=*/TransactionState::kPushUnsolicitedDataNtwkRx,
      /*packet_metadata_type=*/falcon::PacketType::kPushUnsolicitedData,
      /*rsn=*/0,
      /*psn=*/0);

  // Setup the fields to indicate packet in DRAM and its size, and enqueue.
  auto location = PacketBufferLocation::kDram;
  reorder_engine->EnqueuePacket(
      /*bifurcation_id=*/1,
      BufferFetchWorkId(/*cid=*/1, /*rsn=*/0,
                        falcon::PacketType::kPushUnsolicitedData),
      location, packet_payload_size);

  // Reorder engine should not be able to prefetch another packet as its full.
  uint32_t another_packet_payload_size = 1;
  EXPECT_FALSE(reorder_engine->CanPrefetchData(1, another_packet_payload_size));

  reorder_engine->RefundPrefetchBufferSpace(1, packet_payload_size);
  // Reorder engine should be able to prefetch as space has been refunded.
  EXPECT_TRUE(reorder_engine->CanPrefetchData(1, another_packet_payload_size));
}

TEST_F(Gen2SramDramReorderEngineTest, HoLSramPacketProcessing) {
  // Setup up the necessary Falcon configuration fields.
  constexpr int kFalconVersion = 2;
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(kFalconVersion);
  InitFalcon(config);

  // Get a handle on the reorder engine.
  auto gen2falcon = static_cast<Gen2FalconModel*>(falcon_.get());
  auto reorder_engine = gen2falcon->get_sram_dram_reorder_engine();

  // Initialize and get a handle on the connection state with cid = 1.
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(
          falcon_.get(), /*scid=*/1, OrderingMode::kOrdered);
  auto connection_state = FalconTestingHelpers::InitializeConnectionState(
      falcon_.get(), connection_metadata);

  // Setup an incoming transaction with the necessary metadata.
  FalconTestingHelpers::SetupIncomingTransactionWithConnectionState(
      connection_state,
      /*transaction_type=*/TransactionType::kPushUnsolicited,
      /*transaction_location=*/TransactionLocation::kTarget,
      /*state=*/TransactionState::kPushUnsolicitedDataNtwkRx,
      /*packet_metadata_type=*/falcon::PacketType::kPushUnsolicitedData,
      /*rsn=*/0,
      /*psn=*/0);

  // Setup the fields to indicate packet in SRAM and its size, and enqueue.
  auto location = PacketBufferLocation::kSram;
  uint32_t packet_payload_size = 200;
  reorder_engine->EnqueuePacket(
      /*bifurcation_id=*/1,
      BufferFetchWorkId(/*cid=*/1, /*rsn=*/0,
                        falcon::PacketType::kPushUnsolicitedData),
      location, packet_payload_size);
  env_.RunFor(absl::Nanoseconds(config.inter_host_rx_scheduling_tick_ns()));

  // Given that packet is HoL SRAM, it should be sent to the ULP.
  EXPECT_EQ(connection_state
                ->GetTransaction(
                    {/*rsn=*/0,
                     /*transaction_location=*/TransactionLocation::kTarget})
                .value()
                ->state,
            TransactionState::kPushUnsolicitedDataUlpTx);
}

TEST_F(Gen2SramDramReorderEngineTest, DramPacketProcessing) {
  // Setup up the necessary Falcon configuration fields.
  constexpr int kFalconVersion = 2;
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(kFalconVersion);
  InitFalcon(config);

  // Get a handle on the reorder engine.
  auto gen2falcon = static_cast<Gen2FalconModel*>(falcon_.get());
  auto reorder_engine = gen2falcon->get_sram_dram_reorder_engine();

  // Initialize and get a handle on the connection state with cid = 1.
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(
          falcon_.get(), /*scid=*/1, OrderingMode::kOrdered);
  auto connection_state = FalconTestingHelpers::InitializeConnectionState(
      falcon_.get(), connection_metadata);

  // Setup an incoming transaction with the necessary metadata.
  constexpr TransactionState kStartingTransactionState =
      TransactionState::kPushUnsolicitedDataNtwkRx;
  FalconTestingHelpers::SetupIncomingTransactionWithConnectionState(
      connection_state,
      /*transaction_type=*/TransactionType::kPushUnsolicited,
      /*transaction_location=*/TransactionLocation::kTarget,
      /*state=*/kStartingTransactionState,
      /*packet_metadata_type=*/falcon::PacketType::kPushUnsolicitedData,
      /*rsn=*/0,
      /*psn=*/0);

  // Setup the fields to indicate packet in DRAM and its size, and enqueue.
  auto location = PacketBufferLocation::kDram;
  uint32_t packet_payload_size = 200;
  reorder_engine->EnqueuePacket(
      /*bifurcation_id=*/1,
      BufferFetchWorkId(/*cid=*/1, /*rsn=*/0,
                        falcon::PacketType::kPushUnsolicitedData),
      location, packet_payload_size);

  // Run for scheduler tick time and still expect packet to remain in same state
  // as DRAM fetch completion is not done.
  env_.RunFor(absl::Nanoseconds(config.inter_host_rx_scheduling_tick_ns()));
  EXPECT_EQ(connection_state
                ->GetTransaction(
                    {/*rsn=*/0,
                     /*transaction_location=*/TransactionLocation::kTarget})
                .value()
                ->state,
            kStartingTransactionState);

  // Indicate DRAM fetch completion.
  auto work_id = BufferFetchWorkId(/*cid=*/1, /*rsn=*/0,
                                   falcon::PacketType::kPushUnsolicitedData);
  reorder_engine->OnDramFetchCompletion(/*bifurcation_id=*/1, work_id);

  // Run scheduler for tick time and expect state change to indicate packet sent
  // to ULP.
  env_.RunFor(absl::Nanoseconds(config.inter_host_rx_scheduling_tick_ns()));
  EXPECT_EQ(connection_state
                ->GetTransaction(
                    {/*rsn=*/0,
                     /*transaction_location=*/TransactionLocation::kTarget})
                .value()
                ->state,
            TransactionState::kPushUnsolicitedDataUlpTx);
}

TEST_F(Gen2SramDramReorderEngineTest, UnorderedNonHoLPacketProcessing) {
  // Setup up the necessary Falcon configuration fields.
  constexpr int kFalconVersion = 2;
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(kFalconVersion);
  InitFalcon(config);

  // Get a handle on the reorder engine.
  auto gen2falcon = static_cast<Gen2FalconModel*>(falcon_.get());
  auto reorder_engine = gen2falcon->get_sram_dram_reorder_engine();

  // Initialize and get a handle on the connection state with cid = 1.
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(
          falcon_.get(), /*scid=*/1, OrderingMode::kUnordered);
  auto connection_state = FalconTestingHelpers::InitializeConnectionState(
      falcon_.get(), connection_metadata);

  // Setup two incoming transaction with the necessary metadata.
  constexpr TransactionState kStartingTransactionState =
      TransactionState::kPushUnsolicitedDataNtwkRx;
  for (int i = 0; i < 2; ++i) {
    FalconTestingHelpers::SetupIncomingTransactionWithConnectionState(
        connection_state,
        /*transaction_type=*/TransactionType::kPushUnsolicited,
        /*transaction_location=*/TransactionLocation::kTarget,
        /*state=*/kStartingTransactionState,
        /*packet_metadata_type=*/falcon::PacketType::kPushUnsolicitedData,
        /*rsn=*/i,
        /*psn=*/i);
  }

  // Setup the fields to indicate first packet in DRAM and its size, and
  // enqueue.
  auto location = PacketBufferLocation::kDram;
  uint32_t packet_payload_size = 200;
  reorder_engine->EnqueuePacket(
      /*bifurcation_id=*/1,
      BufferFetchWorkId(/*cid=*/1, /*rsn=*/0,
                        falcon::PacketType::kPushUnsolicitedData),
      location, packet_payload_size);

  // Setup the fields to indicate second packet in SRAM, and enqueue.
  auto second_pkt_location = PacketBufferLocation::kSram;
  reorder_engine->EnqueuePacket(
      /*bifurcation_id=*/1,
      BufferFetchWorkId(/*cid=*/1, /*rsn=*/1,
                        falcon::PacketType::kPushUnsolicitedData),
      second_pkt_location, packet_payload_size);

  // Run for scheduler tick time.
  env_.RunFor(absl::Nanoseconds(config.inter_host_rx_scheduling_tick_ns()));
  // Expect 1st packet to not change state as its waiting for DRAM completion.
  EXPECT_EQ(connection_state
                ->GetTransaction(
                    {/*rsn=*/0,
                     /*transaction_location=*/TransactionLocation::kTarget})
                .value()
                ->state,
            kStartingTransactionState);
  // Expect 2nd packet to change state indicating its reach ULP.
  EXPECT_EQ(connection_state
                ->GetTransaction(
                    {/*rsn=*/1,
                     /*transaction_location=*/TransactionLocation::kTarget})
                .value()
                ->state,
            TransactionState::kPushUnsolicitedDataUlpTx);
}

TEST_F(Gen2SramDramReorderEngineTest, OrderedNonHoLPacketProcessing) {
  // Setup up the necessary Falcon configuration fields.
  constexpr int kFalconVersion = 2;
  FalconConfig config =
      DefaultConfigGenerator::DefaultFalconConfig(kFalconVersion);
  InitFalcon(config);

  // Get a handle on the reorder engine.
  auto gen2falcon = static_cast<Gen2FalconModel*>(falcon_.get());
  auto reorder_engine = gen2falcon->get_sram_dram_reorder_engine();

  // Initialize and get a handle on the connection state with cid = 1.
  ConnectionState::ConnectionMetadata connection_metadata =
      FalconTestingHelpers::InitializeConnectionMetadata(
          falcon_.get(), /*scid=*/1, OrderingMode::kOrdered);
  auto connection_state = FalconTestingHelpers::InitializeConnectionState(
      falcon_.get(), connection_metadata);

  // Setup two incoming transaction with the necessary metadata.
  constexpr TransactionState kStartingTransactionState =
      TransactionState::kPushUnsolicitedDataNtwkRx;
  for (int i = 0; i < 2; ++i) {
    FalconTestingHelpers::SetupIncomingTransactionWithConnectionState(
        connection_state,
        /*transaction_type=*/TransactionType::kPushUnsolicited,
        /*transaction_location=*/TransactionLocation::kTarget,
        /*state=*/kStartingTransactionState,
        /*packet_metadata_type=*/falcon::PacketType::kPushUnsolicitedData,
        /*rsn=*/i,
        /*psn=*/i);
    auto& recv_pkt_ctxs =
        connection_state->rx_reliability_metadata.received_packet_contexts;
    const TransactionKey pkt_ctx_key(i, TransactionLocation::kTarget);
    recv_pkt_ctxs.emplace(pkt_ctx_key, ReceivedPacketContext());
    recv_pkt_ctxs[pkt_ctx_key].psn = i;
    recv_pkt_ctxs[pkt_ctx_key].type = falcon::PacketType::kPushUnsolicitedData;
    if (i == 0) {
      recv_pkt_ctxs[pkt_ctx_key].location = PacketBufferLocation::kDram;
    } else {
      recv_pkt_ctxs[pkt_ctx_key].location = PacketBufferLocation::kSram;
    }
  }

  // Setup the fields to indicate first packet in DRAM and its size, and
  // enqueue.
  auto location = PacketBufferLocation::kDram;
  uint32_t packet_payload_size = 200;
  reorder_engine->EnqueuePacket(
      /*bifurcation_id=*/1,
      BufferFetchWorkId(/*cid=*/1, /*rsn=*/0,
                        falcon::PacketType::kPushUnsolicitedData),
      location, packet_payload_size);

  // Setup the fields to indicate second packet in SRAM, and enqueue.
  auto second_pkt_location = PacketBufferLocation::kSram;
  reorder_engine->EnqueuePacket(
      /*bifurcation_id=*/1,
      BufferFetchWorkId(/*cid=*/1, /*rsn=*/1,
                        falcon::PacketType::kPushUnsolicitedData),
      second_pkt_location, packet_payload_size);

  // Run for scheduler tick time.
  env_.RunFor(absl::Nanoseconds(config.inter_host_rx_scheduling_tick_ns()));
  // Expect 1st packet to not change state as its waiting for DRAM completion.
  // 2nd packet has no state change as it is blocked by the 1st packet.
  for (uint32_t i = 0; i < 2; ++i) {
    EXPECT_EQ(connection_state
                  ->GetTransaction(
                      {/*rsn=*/0,
                       /*transaction_location=*/TransactionLocation::kTarget})
                  .value()
                  ->state,
              kStartingTransactionState);
  }

  // Indicate DRAM fetch completion for 1st packet.
  auto work_id = BufferFetchWorkId(/*cid=*/1, /*rsn=*/0,
                                   falcon::PacketType::kPushUnsolicitedData);
  reorder_engine->OnDramFetchCompletion(/*bifurcation_id=*/1, work_id);

  // Run two scheduler ticks time and expect state change indicating both
  // packets sent to ULP.
  env_.RunFor(absl::Nanoseconds(config.inter_host_rx_scheduling_tick_ns() * 2));
  for (uint32_t i = 0; i < 2; ++i) {
    EXPECT_EQ(connection_state
                  ->GetTransaction(
                      {/*rsn=*/i,
                       /*transaction_location=*/TransactionLocation::kTarget})
                  .value()
                  ->state,
              TransactionState::kPushUnsolicitedDataUlpTx);
  }
}

}  // namespace

}  // namespace isekai
