#include "isekai/host/rdma/rdma_roce_model.h"

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "internal/testing.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/simple_environment.h"
#include "isekai/common/status_util.h"
#include "isekai/host/rdma/rdma_component_interfaces.h"
#include "isekai/host/rdma/rdma_configuration.h"
#include "isekai/host/rdma/rdma_roce_cc_dcqcn.h"
#include "omnetpp/cmessage.h"

namespace isekai {

namespace {

using std::make_pair;
using std::pair;
using std::vector;

class FakePacketBuilder : public PacketBuilderInterface {
 public:
  explicit FakePacketBuilder(Environment* env) : env_(env) {}
  void EnqueuePacket(std::unique_ptr<Packet> packet) override {
    packet_log_.push_back(make_pair(env_->ElapsedTime(), *packet));
  }

  void ExtractAndHandleTransportPacket(
      omnetpp::cMessage* network_packet) override {}

  void EncapsulateAndSendTransportPacket(uint16_t priority) override {}

  vector<pair<absl::Duration, Packet>> packet_log_;
  Environment* const env_;
};

class FakeConnectionManager : public ConnectionManagerInterface {
 public:
  absl::StatusOr<std::tuple<QpId, QpId>> CreateConnection(
      absl::string_view host_id1, absl::string_view host_id2,
      uint8_t source_bifurcation_id, uint8_t destination_bifurcation_id,
      QpOptions& options, FalconConnectionOptions& connection_options,
      RdmaConnectedMode rc_mode) override {
    static uint32_t qp_id1 = 1;
    static uint32_t qp_id2 = 100;
    return std::make_tuple(qp_id1++, qp_id2++);
  }
  absl::StatusOr<std::tuple<QpId, QpId>> GetOrCreateBidirectionalConnection(
      absl::string_view host_id1, absl::string_view host_id2,
      uint8_t source_bifurcation_id, uint8_t destination_bifurcation_id,
      QpOptions& options, FalconConnectionOptions& connection_options,
      RdmaConnectedMode rc_mode) override {
    static uint32_t qp_id1 = 1;
    static uint32_t qp_id2 = 100;
    return std::make_tuple(qp_id1++, qp_id2++);
  }
  absl::StatusOr<std::tuple<QpId, QpId>> CreateUnidirectionalConnection(
      absl::string_view host_id1, absl::string_view host_id2,
      uint8_t source_bifurcation_id, uint8_t destination_bifurcation_id,
      QpOptions& options, FalconConnectionOptions& connection_options,
      RdmaConnectedMode rc_mode) override {
    static uint32_t qp_id1 = 1;
    static uint32_t qp_id2 = 100;
    return std::make_tuple(qp_id1++, qp_id2++);
  }

  void RegisterRdma(absl::string_view host_id,
                    RdmaBaseInterface* rdma) override {}
  void RegisterFalcon(absl::string_view host_id,
                      FalconInterface* falcon) override {}
  void PopulateHostInfo(const NetworkConfig& simulation_network) override {}
  absl::StatusOr<std::string> GetIpAddress(absl::string_view host_id) override {
    return "::1";
  }
};

Packet CreatePacket(Packet::Roce::Opcode opcode, uint32_t psn,
                    uint32_t dest_qp_id, uint32_t payload_length,
                    uint8_t ack_req) {
  Packet p;
  p.roce.is_roce = true;
  p.roce.opcode = opcode;
  p.roce.psn = psn;
  p.roce.dest_qp_id = dest_qp_id;
  p.roce.payload_length = payload_length;
  p.roce.ack_req = ack_req;
  return p;
}
Packet CreateReadRequest(uint32_t psn, uint32_t dest_qp_id,
                         uint32_t request_length) {
  auto p =
      CreatePacket(Packet::Roce::Opcode::kReadRequest, psn, dest_qp_id, 0, 0);
  p.roce.request_length = request_length;
  return p;
}
Packet CreateAck(uint32_t psn, uint32_t dest_qp_id) {
  auto p = CreatePacket(Packet::Roce::Opcode::kAck, psn, dest_qp_id, 0, 0);
  p.roce.ack.type = Packet::Roce::Ack::Type::kAck;
  return p;
}
Packet CreateNakSequenceError(uint32_t psn, uint32_t dest_qp_id) {
  auto p = CreatePacket(Packet::Roce::Opcode::kAck, psn, dest_qp_id, 0, 0);
  p.roce.ack.type = Packet::Roce::Ack::Type::kNak;
  p.roce.ack.nak_type = Packet::Roce::Ack::NakType::kPsnSequenceError;
  return p;
}

class RdmaRoceModelTest : public ::testing::Test {
 protected:
  RdmaRoceModelTest() : pb_(&env_) {
    rdma_ =
        new RdmaRoceModel(config_, roce_config_, 4198, &env_,
                          /*stats_collector=*/nullptr, &connection_manager_);
    rdma_->ConnectPacketBuilder(&pb_);
  }

  ~RdmaRoceModelTest() override { delete rdma_; }

  SimpleEnvironment env_;
  RdmaConfig config_ = DefaultConfigGenerator::DefaultRdmaConfig();
  RoceConfig roce_config_;
  FakeConnectionManager connection_manager_;
  RdmaRoceModel* rdma_;
  FakePacketBuilder pb_;
  uint32_t test_interval_ = 1000000;
};

class RdmaRoceModelNoRateLimitTest : public RdmaRoceModelTest {
 protected:
  static const uint32_t kInfiniteRate = 1000000000;
  RdmaRoceModelNoRateLimitTest() : RdmaRoceModelTest() {
    RdmaCcDcqcnOptions cc_options;
    cc_options.type = RdmaCcOptions::Type::kDcqcn;
    // Set to infinite to rule out the effect of cc.
    cc_options.max_rate = cc_options.min_rate = kInfiniteRate;
    rdma_->SetupCc(&cc_options);
  }
};

class RdmaRoceModelRateLimitTest : public RdmaRoceModelTest {
 protected:
  RdmaRoceModelRateLimitTest() : RdmaRoceModelTest() {
    RdmaCcDcqcnOptions cc_options;
    cc_options.type = RdmaCcOptions::Type::kDcqcn;
    rdma_->SetupCc(&cc_options);
  }
};

TEST_F(RdmaRoceModelNoRateLimitTest, WorkScheduler) {
  QpOptions qp_options;
  qp_options.dst_ip = "::1";

  std::pair<QpId, QpId> qp1 = make_pair(1, 100);
  rdma_->CreateRcQp(qp1.first, qp1.second, qp_options,
                    RdmaConnectedMode::kOrderedRc);
  std::pair<QpId, QpId> qp2 = make_pair(2, 101);
  rdma_->CreateRcQp(qp2.first, qp2.second, qp_options,
                    RdmaConnectedMode::kOrderedRc);

  vector<Packet> p(100);
  auto& p1 = p[0];
  p1 = CreateReadRequest(0, qp1.first, 10000);
  auto& p2 = p[1];
  p2 = CreateReadRequest(3, qp1.first, 1000);

  // ACK
  auto& a1 = p[2];
  a1 = CreateAck(0, qp1.first);
  auto& a2 = p[3];
  a2 = CreateAck(6, qp1.first);
  auto& a3 = p[4];
  a3 = CreateAck(2, qp2.first);

  // READ response
  auto& r1 = p[5];
  r1 = CreatePacket(Packet::Roce::Opcode::kReadResponseFirst, 3, qp1.first,
                    4096, 0);

  vector<pair<absl::Duration, uint32_t>> completion_log;

  // These ops should be scheduled round-robin between QPs, starting with QP 1.
  rdma_->PerformOp(
      /* qp_id = */
      qp1.first,
      /* opcode = */ RdmaOpcode::kWrite,
      /* sgl = */ {5000, 5000}, /* is_inline = */ false,
      /* completion_callback = */
      [&](Packet::Syndrome syndrome) {
        completion_log.push_back(make_pair(env_.ElapsedTime(), 0));
      },
      kInvalidQpId);
  rdma_->PerformOp(
      /* qp_id = */
      qp1.first,
      /* opcode = */ RdmaOpcode::kRead,
      /* sgl = */ {10000}, /* is_inline = */ false,
      /* completion_callback = */
      [&](Packet::Syndrome syndrome) {
        completion_log.push_back(make_pair(env_.ElapsedTime(), 1));
      },
      kInvalidQpId);
  rdma_->PerformOp(
      /* qp_id = */
      qp1.first,
      /* opcode = */ RdmaOpcode::kWrite,
      /* sgl = */ {1000}, /* is_inline = */ false,
      /* completion_callback = */
      [&](Packet::Syndrome syndrome) {
        completion_log.push_back(make_pair(env_.ElapsedTime(), 2));
      },
      kInvalidQpId);
  rdma_->PerformOp(
      /* qp_id = */
      qp2.first,
      /* opcode = */ RdmaOpcode::kWrite,
      /* sgl = */ {5000}, /* is_inline = */ false,
      /* completion_callback = */
      [&](Packet::Syndrome syndrome) {
        completion_log.push_back(make_pair(env_.ElapsedTime(), 3));
      },
      kInvalidQpId);
  rdma_->PerformOp(
      /* qp_id = */
      qp2.first,
      /* opcode = */ RdmaOpcode::kWrite,
      /* sgl = */ {500}, /* is_inline = */ false,
      /* completion_callback = */
      [&](Packet::Syndrome syndrome) {
        completion_log.push_back(make_pair(env_.ElapsedTime(), 4));
      },
      kInvalidQpId);
  rdma_->TransferRxPacket(std::make_unique<Packet>(p1), false);
  rdma_->TransferRxPacket(std::make_unique<Packet>(p2), false);

  EXPECT_OK(env_.ScheduleEvent(absl::Nanoseconds(10000), [&]() {
    rdma_->TransferRxPacket(std::make_unique<Packet>(a1), false);
  }));
  EXPECT_OK(env_.ScheduleEvent(absl::Nanoseconds(10100), [&]() {
    rdma_->TransferRxPacket(std::make_unique<Packet>(r1), false);
  }));
  EXPECT_OK(env_.ScheduleEvent(absl::Nanoseconds(10200), [&]() {
    rdma_->TransferRxPacket(std::make_unique<Packet>(a2), false);
  }));
  EXPECT_OK(env_.ScheduleEvent(absl::Nanoseconds(10300), [&]() {
    rdma_->TransferRxPacket(std::make_unique<Packet>(a3), false);
  }));

  EXPECT_OK(env_.ScheduleEvent(absl::Nanoseconds(test_interval_),
                               [&]() { env_.Stop(); }));
  env_.Run();

  EXPECT_EQ(pb_.packet_log_.size(), 14);

  Packet::Roce roce = pb_.packet_log_[0].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kWriteFirst &&
              roce.dest_qp_id == qp1.second && roce.payload_length == 4096 &&
              roce.psn == 0);
  roce = pb_.packet_log_[1].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kReadResponseFirst &&
              roce.dest_qp_id == qp1.second && roce.payload_length == 4096 &&
              roce.psn == 0);
  roce = pb_.packet_log_[2].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kWriteFirst &&
              roce.dest_qp_id == qp2.second && roce.payload_length == 4096 &&
              roce.psn == 0);
  roce = pb_.packet_log_[3].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kWriteMiddle &&
              roce.dest_qp_id == qp1.second && roce.payload_length == 4096 &&
              roce.psn == 1);
  roce = pb_.packet_log_[4].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kReadResponseMiddle &&
              roce.dest_qp_id == qp1.second && roce.payload_length == 4096 &&
              roce.psn == 1);
  roce = pb_.packet_log_[5].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kWriteLast &&
              roce.dest_qp_id == qp2.second &&
              roce.payload_length == 5000 - 4096 && roce.psn == 1);
  roce = pb_.packet_log_[6].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kWriteLast &&
              roce.dest_qp_id == qp1.second &&
              roce.payload_length == 10000 - 4096 * 2 && roce.psn == 2);
  roce = pb_.packet_log_[7].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kReadResponseLast &&
              roce.dest_qp_id == qp1.second &&
              roce.payload_length == 10000 - 4096 * 2 && roce.psn == 2);
  roce = pb_.packet_log_[8].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kWriteOnly &&
              roce.dest_qp_id == qp2.second && roce.payload_length == 500 &&
              roce.psn == 2);
  roce = pb_.packet_log_[9].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kReadRequest &&
              roce.dest_qp_id == qp1.second && roce.request_length == 10000 &&
              roce.psn == 3);
  roce = pb_.packet_log_[10].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kReadResponseOnly &&
              roce.dest_qp_id == qp1.second && roce.payload_length == 1000 &&
              roce.psn == 3);
  roce = pb_.packet_log_[11].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kWriteOnly &&
              roce.dest_qp_id == qp1.second && roce.payload_length == 1000 &&
              roce.psn == 6);
  roce = pb_.packet_log_[12].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kReadRequest &&
              roce.dest_qp_id == qp1.second &&
              roce.request_length == 10000 - 4096 && roce.psn == 4);
  roce = pb_.packet_log_[13].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kWriteOnly &&
              roce.dest_qp_id == qp1.second && roce.payload_length == 1000 &&
              roce.psn == 6);

  EXPECT_EQ(completion_log.size(), 3);
  EXPECT_TRUE(completion_log[0].first == absl::Nanoseconds(10100) &&
              completion_log[0].second == 0);
  EXPECT_TRUE(completion_log[1].first == absl::Nanoseconds(10300) &&
              completion_log[1].second == 3);
  EXPECT_TRUE(completion_log[2].first == absl::Nanoseconds(10300) &&
              completion_log[2].second == 4);
}

// At target, send OOO WRITE packets, see how NAK are generated.
TEST_F(RdmaRoceModelNoRateLimitTest, TargetNakGenerationTest) {
  auto timeout = absl::Microseconds(100);
  QpOptions qp_options;
  qp_options.dst_ip = "::1";
  qp_options.ack_coalescing_threshold = 3;
  qp_options.ack_coalesing_timeout = timeout;
  qp_options.retx_timeout = timeout;

  std::pair<QpId, QpId> qp1 = make_pair(1, 100);
  rdma_->CreateRcQp(qp1.first, qp1.second, qp_options,
                    RdmaConnectedMode::kOrderedRc);

  const int n_packet = 12, lost_psn = 4;
  absl::Duration t = absl::ZeroDuration();
  vector<Packet> p(n_packet);
  p[0] = CreatePacket(Packet::Roce::Opcode::kWriteFirst, 0, qp1.first, 4096, 1);
  for (int i = 1; i < n_packet - 1; i++)
    p[i] =
        CreatePacket(Packet::Roce::Opcode::kWriteMiddle, i, qp1.first, 4096, 1);
  p[n_packet - 1] = CreatePacket(Packet::Roce::Opcode::kWriteLast, n_packet - 1,
                                 qp1.first, 4096, 1);

  // Initial transmission.
  for (int i = 0; i < n_packet; i++) {
    t += absl::Nanoseconds(100);
    if (i == lost_psn) continue;
    EXPECT_OK(env_.ScheduleEvent(t, [this, &p, i]() {
      rdma_->TransferRxPacket(std::make_unique<Packet>(p[i]), false);
    }));
  }

  // Retransmission.
  for (int i = lost_psn; i < n_packet; i++) {
    t += absl::Nanoseconds(100);
    EXPECT_OK(env_.ScheduleEvent(t, [this, &p, i]() {
      rdma_->TransferRxPacket(std::make_unique<Packet>(p[i]), false);
    }));
  }
  env_.RunFor(absl::Nanoseconds(test_interval_));

  EXPECT_EQ(pb_.packet_log_.size(), 5);

  // First coalesced ACK.
  Packet::Roce roce = pb_.packet_log_[0].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kAck);
  EXPECT_EQ(roce.ack.type, Packet::Roce::Ack::Type::kAck);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, 2);

  // Send a NAK.
  roce = pb_.packet_log_[1].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kAck);
  EXPECT_EQ(roce.ack.type, Packet::Roce::Ack::Type::kNak);
  EXPECT_EQ(roce.ack.nak_type, Packet::Roce::Ack::NakType::kPsnSequenceError);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, 3);

  // After retransmission, coalesce from 4, first ACK at 6.
  roce = pb_.packet_log_[2].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kAck);
  EXPECT_EQ(roce.ack.type, Packet::Roce::Ack::Type::kAck);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, 6);

  // Then ACK at 9.
  roce = pb_.packet_log_[3].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kAck);
  EXPECT_EQ(roce.ack.type, Packet::Roce::Ack::Type::kAck);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, 9);

  // The last coalesced ACK should be triggered by timeout (ack 2 packets).
  roce = pb_.packet_log_[4].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kAck);
  EXPECT_EQ(roce.ack.type, Packet::Roce::Ack::Type::kAck);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, 11);
  EXPECT_EQ(pb_.packet_log_[4].first, t - absl::Nanoseconds(100) + timeout);
}

TEST_F(RdmaRoceModelNoRateLimitTest, InitiatorNakHandlingTest) {
  auto timeout = absl::Microseconds(100);
  QpOptions qp_options;
  qp_options.dst_ip = "::1";
  qp_options.retx_timeout = timeout;

  std::pair<QpId, QpId> qp1 = make_pair(1, 100);
  rdma_->CreateRcQp(qp1.first, qp1.second, qp_options,
                    RdmaConnectedMode::kOrderedRc);

  vector<pair<absl::Duration, uint32_t>> completion_log;
  rdma_->PerformOp(
      /* qp_id = */
      qp1.first,
      /* opcode = */ RdmaOpcode::kWrite,
      /* sgl = */ {12000}, /* is_inline = */ false,
      /* completion_callback = */
      [&](Packet::Syndrome syndrome) {
        completion_log.push_back(make_pair(env_.ElapsedTime(), 0));
      },
      kInvalidQpId);

  vector<Packet> p(100);
  p[0] = CreateNakSequenceError(0, qp1.first);
  p[1] = CreateAck(2, qp1.first);
  EXPECT_OK(env_.ScheduleEvent(absl::Nanoseconds(100), [&]() {
    rdma_->TransferRxPacket(std::make_unique<Packet>(p[0]), false);
  }));
  EXPECT_OK(env_.ScheduleEvent(timeout + absl::Nanoseconds(500), [&]() {
    rdma_->TransferRxPacket(std::make_unique<Packet>(p[1]), false);
  }));

  env_.RunFor(absl::Nanoseconds(test_interval_));

  EXPECT_EQ(pb_.packet_log_.size(), 7);
  // First 3 original packets.
  Packet::Roce roce = pb_.packet_log_[0].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kWriteFirst);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, 0);

  roce = pb_.packet_log_[1].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kWriteMiddle);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, 1);

  roce = pb_.packet_log_[2].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kWriteLast);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, 2);

  // Retransmission 1 and 2 triggered by NAK.
  roce = pb_.packet_log_[3].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kWriteMiddle);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, 1);

  roce = pb_.packet_log_[4].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kWriteLast);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, 2);

  // Retransmission 1 and 2 triggered by RTO.
  roce = pb_.packet_log_[5].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kWriteMiddle);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, 1);

  roce = pb_.packet_log_[6].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kWriteLast);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, 2);
}

TEST_F(RdmaRoceModelNoRateLimitTest, InitiatorReadResponseDropTest) {
  auto timeout = absl::Microseconds(100);
  QpOptions qp_options;
  qp_options.dst_ip = "::1";
  qp_options.retx_timeout = timeout;

  std::pair<QpId, QpId> qp1 = make_pair(1, 100);
  rdma_->CreateRcQp(qp1.first, qp1.second, qp_options,
                    RdmaConnectedMode::kOrderedRc);

  uint32_t size = 40000, n_responses = (size - 1) / 4096 + 1;
  vector<pair<absl::Duration, uint32_t>> completion_log;
  rdma_->PerformOp(
      /* qp_id = */
      qp1.first,
      /* opcode = */ RdmaOpcode::kRead,
      /* sgl = */ {size}, /* is_inline = */ false,
      /* completion_callback = */
      [&](Packet::Syndrome syndrome) {
        completion_log.push_back(make_pair(env_.ElapsedTime(), 0));
      },
      kInvalidQpId);

  vector<Packet> p(100);
  p[0] = CreatePacket(Packet::Roce::Opcode::kReadResponseFirst, 0, qp1.first,
                      4096, 0);
  for (int i = 1; i < n_responses - 1; i++)
    p[i] = CreatePacket(Packet::Roce::Opcode::kReadResponseMiddle, i, qp1.first,
                        4096, 0);
  p[n_responses - 1] =
      CreatePacket(Packet::Roce::Opcode::kReadResponseLast, n_responses - 1,
                   qp1.first, size - (n_responses - 1) * 4096, 0);

  // Wait for generate the READ request.
  env_.RunFor(absl::Nanoseconds(1000));

  EXPECT_EQ(pb_.packet_log_.size(), 1);
  Packet::Roce roce = pb_.packet_log_[0].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kReadRequest);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, 0);

  // Send responses to initiator. Lost a packet.
  uint32_t lost_psn = 5;
  for (int i = 0; i < n_responses; i++) {
    if (i == lost_psn) continue;
    EXPECT_OK(env_.ScheduleEvent(absl::ZeroDuration(), [this, &p, i]() {
      rdma_->TransferRxPacket(std::make_unique<Packet>(p[i]), false);
    }));
    env_.RunFor(absl::Nanoseconds(100));
    if (i < lost_psn) {
      EXPECT_EQ(pb_.packet_log_.size(), 1);
    } else if (i > lost_psn) {
      // After loss, there should be a retry of the READ with psn = lost psn.
      EXPECT_EQ(pb_.packet_log_.size(), 2);
      roce = pb_.packet_log_[1].second.roce;
      EXPECT_TRUE(roce.is_roce);
      EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kReadRequest);
      EXPECT_EQ(roce.dest_qp_id, qp1.second);
      EXPECT_EQ(roce.psn, lost_psn);
      EXPECT_EQ(roce.request_length, size - 4096 * lost_psn);
    }
  }

  p[lost_psn].roce.opcode = Packet::Roce::Opcode::kReadResponseFirst;

  // Send responses to initiator. Lost another packet.
  uint32_t lost_psn2 = 8;
  for (int i = lost_psn; i < n_responses; i++) {
    if (i == lost_psn2) continue;
    EXPECT_OK(env_.ScheduleEvent(absl::ZeroDuration(), [this, &p, i]() {
      rdma_->TransferRxPacket(std::make_unique<Packet>(p[i]), false);
    }));
    env_.RunFor(absl::Nanoseconds(100));
    if (i < lost_psn2) {
      EXPECT_EQ(pb_.packet_log_.size(), 2);
    } else if (i > lost_psn2) {
      EXPECT_EQ(pb_.packet_log_.size(), 3);
      roce = pb_.packet_log_[2].second.roce;
      EXPECT_TRUE(roce.is_roce);
      EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kReadRequest);
      EXPECT_EQ(roce.dest_qp_id, qp1.second);
      EXPECT_EQ(roce.psn, lost_psn2);
      EXPECT_EQ(roce.request_length, size - 4096 * lost_psn2);
    }
  }

  // No response, will trigger timeout.
  env_.RunFor(timeout);
  EXPECT_EQ(pb_.packet_log_.size(), 4);
  roce = pb_.packet_log_[3].second.roce;
  EXPECT_TRUE(roce.is_roce);
  EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kReadRequest);
  EXPECT_EQ(roce.dest_qp_id, qp1.second);
  EXPECT_EQ(roce.psn, lost_psn2);
  EXPECT_EQ(roce.request_length, size - 4096 * lost_psn2);

  // Finish.
  p[lost_psn2].roce.opcode = Packet::Roce::Opcode::kReadResponseFirst;
  for (int i = lost_psn2; i < n_responses; i++) {
    EXPECT_OK(env_.ScheduleEvent(absl::ZeroDuration(), [this, &p, i]() {
      rdma_->TransferRxPacket(std::make_unique<Packet>(p[i]), false);
    }));
    EXPECT_EQ(completion_log.size(), 0);
    env_.RunFor(absl::Nanoseconds(100));
  }

  EXPECT_EQ(completion_log.size(), 1);
}

// At target, if NAK is triggered while READ response is ongoing, NAK should be
// delayed after READ finishes. READ1, WRITE(lost), READ2, retry WRITE, retry
// READ2.
TEST_F(RdmaRoceModelNoRateLimitTest, TargetNakDuringRead) {
  auto timeout = absl::Microseconds(100);
  QpOptions qp_options;
  qp_options.dst_ip = "::1";
  qp_options.ack_coalescing_threshold = 3;
  qp_options.ack_coalesing_timeout = timeout;
  qp_options.retx_timeout = timeout;

  std::pair<QpId, QpId> qp1 = make_pair(1, 100);
  rdma_->CreateRcQp(qp1.first, qp1.second, qp_options,
                    RdmaConnectedMode::kOrderedRc);

  const int n_packet = 3, read1_size = 40000,
            read1_npkt = (read1_size - 1) / 4096 + 1;
  vector<Packet> p(n_packet);
  p[0] = CreateReadRequest(0, qp1.first, read1_size);
  p[1] = CreatePacket(Packet::Roce::Opcode::kWriteOnly, read1_npkt, qp1.first,
                      100, 1);
  p[2] = CreateReadRequest(read1_npkt + 1, qp1.first, 1000);

  // READ1 and READ2 delivered, but WRITE1 lost.
  EXPECT_OK(env_.ScheduleEvent(absl::ZeroDuration(), [this, &p]() {
    rdma_->TransferRxPacket(std::make_unique<Packet>(p[0]), false);
  }));
  EXPECT_OK(env_.ScheduleEvent(absl::Nanoseconds(35), [this, &p]() {
    rdma_->TransferRxPacket(std::make_unique<Packet>(p[2]), false);
  }));
  env_.RunFor(absl::Microseconds(50));

  // Expect responses to READ1, and a NAK.
  EXPECT_EQ(pb_.packet_log_.size(), read1_npkt + 1);
  for (int i = 0; i < read1_npkt; i++) {
    auto& roce = pb_.packet_log_[i].second.roce;
    EXPECT_TRUE(roce.is_roce);
    if (i == 0)
      EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kReadResponseFirst);
    else if (i == read1_npkt - 1)
      EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kReadResponseLast);
    else
      EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kReadResponseMiddle);
    EXPECT_EQ(roce.dest_qp_id, qp1.second);
    EXPECT_EQ(roce.psn, i);
  }
  {
    auto& roce = pb_.packet_log_[read1_npkt].second.roce;
    EXPECT_TRUE(roce.is_roce);
    EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kAck);
    EXPECT_EQ(roce.ack.type, Packet::Roce::Ack::Type::kNak);
    EXPECT_EQ(roce.ack.nak_type, Packet::Roce::Ack::NakType::kPsnSequenceError);
    EXPECT_EQ(roce.dest_qp_id, qp1.second);
    EXPECT_EQ(roce.psn, read1_npkt - 1);
    EXPECT_EQ(pb_.packet_log_[read1_npkt].first,
              pb_.packet_log_[read1_npkt - 1].first);
  }

  // Retry WRITE and READ2.
  pb_.packet_log_.clear();
  EXPECT_OK(env_.ScheduleEvent(absl::Nanoseconds(0), [this, &p]() {
    rdma_->TransferRxPacket(std::make_unique<Packet>(p[1]), false);
  }));
  EXPECT_OK(env_.ScheduleEvent(absl::Nanoseconds(5), [this, &p]() {
    rdma_->TransferRxPacket(std::make_unique<Packet>(p[2]), false);
  }));
  env_.RunFor(absl::Microseconds(50));

  // Expect ACK and response to READ2.
  EXPECT_EQ(pb_.packet_log_.size(), 2);
  {
    auto roce = pb_.packet_log_[0].second.roce;
    EXPECT_TRUE(roce.is_roce);
    EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kAck);
    EXPECT_EQ(roce.ack.type, Packet::Roce::Ack::Type::kAck);
    EXPECT_EQ(roce.dest_qp_id, qp1.second);
    EXPECT_EQ(roce.psn, read1_npkt);

    roce = pb_.packet_log_[1].second.roce;
    EXPECT_TRUE(roce.is_roce);
    EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kReadResponseOnly);
    EXPECT_EQ(roce.dest_qp_id, qp1.second);
    EXPECT_EQ(roce.psn, read1_npkt + 1);
    EXPECT_EQ(roce.payload_length, 1000);
  }
}

// This class tests response and ack generation at target. We predefine 7
// packets (WRITEs and READs). Then we test different arrival time of these
// packets at target.
class TargetResponseAndAckGenerationNoRateLimitTest
    : public RdmaRoceModelNoRateLimitTest {
 protected:
  TargetResponseAndAckGenerationNoRateLimitTest()
      : p_(100),
        w0_(p_[0]),
        w1_(p_[1]),
        w2_(p_[2]),
        r0_(p_[3]),
        w3_(p_[4]),
        w4_(p_[5]),
        r1_(p_[6]) {
    QpOptions qp_options;
    qp_options.dst_ip = "::1";
    qp_options.ack_coalescing_threshold = 2;
    qp_options.ack_coalesing_timeout = absl::Nanoseconds(100000);

    qp1_ = make_pair(1, 100);
    rdma_->CreateRcQp(qp1_.first, qp1_.second, qp_options,
                      RdmaConnectedMode::kOrderedRc);

    w0_ =
        CreatePacket(Packet::Roce::Opcode::kWriteFirst, 0, qp1_.first, 4096, 1);
    w1_ = CreatePacket(Packet::Roce::Opcode::kWriteMiddle, 1, qp1_.first, 4096,
                       0);
    w2_ =
        CreatePacket(Packet::Roce::Opcode::kWriteLast, 2, qp1_.first, 4096, 1);
    r0_ = CreateReadRequest(3, qp1_.first, 10000);
    w3_ =
        CreatePacket(Packet::Roce::Opcode::kWriteFirst, 6, qp1_.first, 4096, 1);
    w4_ =
        CreatePacket(Packet::Roce::Opcode::kWriteLast, 7, qp1_.first, 4096, 0);
    r1_ = CreateReadRequest(8, qp1_.first, 1000);
  }

  // Set the arrival time of different packets.
  void RxPackets(const vector<uint64_t>& times) {
    CHECK_LE(times.size(), 7);
    for (int i = 0; i < times.size(); i++) {
      EXPECT_OK(env_.ScheduleEvent(absl::Nanoseconds(times[i]), [this, i]() {
        rdma_->TransferRxPacket(std::make_unique<Packet>(p_[i]), false);
      }));
    }
  }
  ~TargetResponseAndAckGenerationNoRateLimitTest() override {}

  vector<Packet> p_;
  std::pair<QpId, QpId> qp1_;
  Packet &w0_, &w1_, &w2_, &r0_, &w3_, &w4_, &r1_;
};

TEST_F(TargetResponseAndAckGenerationNoRateLimitTest, Test0) {
  // WRITE, READ start, READ finish, WRITE.
  RxPackets({100, 120, 140, 160, 500, 520});
  env_.RunFor(absl::Nanoseconds(test_interval_));

  EXPECT_GE(pb_.packet_log_.size(), 1);
  Packet::Roce roce = pb_.packet_log_[0].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kAck &&
              roce.ack.type == Packet::Roce::Ack::Type::kAck &&
              roce.dest_qp_id == qp1_.second && roce.psn == w1_.roce.psn);
  EXPECT_EQ(pb_.packet_log_[0].first, absl::Nanoseconds(120));

  EXPECT_GE(pb_.packet_log_.size(), 2);
  roce = pb_.packet_log_[1].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kAck &&
              roce.ack.type == Packet::Roce::Ack::Type::kAck &&
              roce.dest_qp_id == qp1_.second && roce.psn == w2_.roce.psn);
  EXPECT_EQ(pb_.packet_log_[1].first, absl::Nanoseconds(160));

  EXPECT_GE(pb_.packet_log_.size(), 3);
  roce = pb_.packet_log_[2].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kReadResponseFirst &&
              roce.dest_qp_id == qp1_.second && roce.payload_length == 4096 &&
              roce.psn == r0_.roce.psn);

  EXPECT_GE(pb_.packet_log_.size(), 4);
  roce = pb_.packet_log_[3].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kReadResponseMiddle &&
              roce.dest_qp_id == qp1_.second && roce.payload_length == 4096 &&
              roce.psn == r0_.roce.psn + 1);

  EXPECT_GE(pb_.packet_log_.size(), 5);
  roce = pb_.packet_log_[4].second.roce;
  EXPECT_TRUE(
      roce.is_roce && roce.opcode == Packet::Roce::Opcode::kReadResponseLast &&
      roce.dest_qp_id == qp1_.second &&
      roce.payload_length == 10000 - 2 * 4096 && roce.psn == r0_.roce.psn + 2);

  EXPECT_GE(pb_.packet_log_.size(), 6);
  roce = pb_.packet_log_[5].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kAck &&
              roce.ack.type == Packet::Roce::Ack::Type::kAck &&
              roce.dest_qp_id == qp1_.second && roce.psn == w4_.roce.psn);
  EXPECT_EQ(pb_.packet_log_[5].first, absl::Nanoseconds(520));

  EXPECT_EQ(pb_.packet_log_.size(), 6);
}

TEST_F(TargetResponseAndAckGenerationNoRateLimitTest, Test1) {
  // WRITE, READ start, WRITE (trigger ACK), READ finish.
  RxPackets({100, 120, 140, 160, 165, 170});
  env_.RunFor(absl::Nanoseconds(test_interval_));

  EXPECT_GE(pb_.packet_log_.size(), 1);
  Packet::Roce roce = pb_.packet_log_[0].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kAck &&
              roce.ack.type == Packet::Roce::Ack::Type::kAck &&
              roce.dest_qp_id == qp1_.second && roce.psn == w1_.roce.psn);
  EXPECT_EQ(pb_.packet_log_[0].first, absl::Nanoseconds(120));

  EXPECT_GE(pb_.packet_log_.size(), 2);
  roce = pb_.packet_log_[1].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kAck &&
              roce.ack.type == Packet::Roce::Ack::Type::kAck &&
              roce.dest_qp_id == qp1_.second && roce.psn == w2_.roce.psn);
  EXPECT_EQ(pb_.packet_log_[1].first, absl::Nanoseconds(160));

  EXPECT_GE(pb_.packet_log_.size(), 3);
  roce = pb_.packet_log_[2].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kReadResponseFirst &&
              roce.dest_qp_id == qp1_.second && roce.payload_length == 4096 &&
              roce.psn == r0_.roce.psn);

  EXPECT_GE(pb_.packet_log_.size(), 4);
  roce = pb_.packet_log_[3].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kReadResponseMiddle &&
              roce.dest_qp_id == qp1_.second && roce.payload_length == 4096 &&
              roce.psn == r0_.roce.psn + 1);

  EXPECT_GE(pb_.packet_log_.size(), 5);
  roce = pb_.packet_log_[4].second.roce;
  EXPECT_TRUE(
      roce.is_roce && roce.opcode == Packet::Roce::Opcode::kReadResponseLast &&
      roce.dest_qp_id == qp1_.second &&
      roce.payload_length == 10000 - 2 * 4096 && roce.psn == r0_.roce.psn + 2);

  EXPECT_GE(pb_.packet_log_.size(), 6);
  roce = pb_.packet_log_[5].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kAck &&
              roce.ack.type == Packet::Roce::Ack::Type::kAck &&
              roce.dest_qp_id == qp1_.second && roce.psn == w4_.roce.psn);
  EXPECT_EQ(pb_.packet_log_[5].first, pb_.packet_log_[4].first);

  EXPECT_EQ(pb_.packet_log_.size(), 6);
}

TEST_F(TargetResponseAndAckGenerationNoRateLimitTest, Test2) {
  // WRITE, READ1 start, WRITE (trigger ACK), READ2 arrive, READ1 finish,
  // READ2 finish.
  RxPackets({100, 120, 140, 160, 165, 170, 175});
  env_.RunFor(absl::Nanoseconds(test_interval_));

  EXPECT_GE(pb_.packet_log_.size(), 1);
  Packet::Roce roce = pb_.packet_log_[0].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kAck &&
              roce.ack.type == Packet::Roce::Ack::Type::kAck &&
              roce.dest_qp_id == qp1_.second && roce.psn == w1_.roce.psn);
  EXPECT_EQ(pb_.packet_log_[0].first, absl::Nanoseconds(120));

  EXPECT_GE(pb_.packet_log_.size(), 2);
  roce = pb_.packet_log_[1].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kAck &&
              roce.ack.type == Packet::Roce::Ack::Type::kAck &&
              roce.dest_qp_id == qp1_.second && roce.psn == w2_.roce.psn);
  EXPECT_EQ(pb_.packet_log_[1].first, absl::Nanoseconds(160));

  EXPECT_GE(pb_.packet_log_.size(), 3);
  roce = pb_.packet_log_[2].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kReadResponseFirst &&
              roce.dest_qp_id == qp1_.second && roce.payload_length == 4096 &&
              roce.psn == r0_.roce.psn);

  EXPECT_GE(pb_.packet_log_.size(), 4);
  roce = pb_.packet_log_[3].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kReadResponseMiddle &&
              roce.dest_qp_id == qp1_.second && roce.payload_length == 4096 &&
              roce.psn == r0_.roce.psn + 1);

  EXPECT_GE(pb_.packet_log_.size(), 5);
  roce = pb_.packet_log_[4].second.roce;
  EXPECT_TRUE(
      roce.is_roce && roce.opcode == Packet::Roce::Opcode::kReadResponseLast &&
      roce.dest_qp_id == qp1_.second &&
      roce.payload_length == 10000 - 2 * 4096 && roce.psn == r0_.roce.psn + 2);

  EXPECT_GE(pb_.packet_log_.size(), 6);
  roce = pb_.packet_log_[5].second.roce;
  EXPECT_TRUE(roce.is_roce &&
              roce.opcode == Packet::Roce::Opcode::kReadResponseOnly &&
              roce.dest_qp_id == qp1_.second && roce.payload_length == 1000 &&
              roce.psn == r1_.roce.psn);

  EXPECT_GE(pb_.packet_log_.size(), 7);
  roce = pb_.packet_log_[6].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kAck &&
              roce.ack.type == Packet::Roce::Ack::Type::kAck &&
              roce.dest_qp_id == qp1_.second && roce.psn == r1_.roce.psn);
  EXPECT_EQ(pb_.packet_log_[6].first, pb_.packet_log_[5].first);

  EXPECT_EQ(pb_.packet_log_.size(), 7);
}

TEST_F(TargetResponseAndAckGenerationNoRateLimitTest, Test3) {
  // W0, timeout, W1, W2, timeout.
  RxPackets({100, 100120, 200140});
  env_.RunFor(absl::Nanoseconds(test_interval_));

  EXPECT_GE(pb_.packet_log_.size(), 1);
  Packet::Roce roce = pb_.packet_log_[0].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kAck &&
              roce.ack.type == Packet::Roce::Ack::Type::kAck &&
              roce.dest_qp_id == qp1_.second && roce.psn == w0_.roce.psn);
  EXPECT_EQ(pb_.packet_log_[0].first, absl::Nanoseconds(100 + 100000));

  EXPECT_GE(pb_.packet_log_.size(), 2);
  roce = pb_.packet_log_[1].second.roce;
  EXPECT_TRUE(roce.is_roce && roce.opcode == Packet::Roce::Opcode::kAck &&
              roce.ack.type == Packet::Roce::Ack::Type::kAck &&
              roce.dest_qp_id == qp1_.second && roce.psn == w2_.roce.psn);
  EXPECT_EQ(pb_.packet_log_[1].first, absl::Nanoseconds(200140 + 100000));

  EXPECT_EQ(pb_.packet_log_.size(), 2);
}

TEST_F(RdmaRoceModelRateLimitTest, RateLimitTest) {
  QpOptions qp_options;
  qp_options.dst_ip = "::1";

  std::pair<QpId, QpId> qp1 = make_pair(1, 100);
  rdma_->CreateRcQp(qp1.first, qp1.second, qp_options,
                    RdmaConnectedMode::kOrderedRc);

  uint32_t n_response = 102;
  vector<Packet> p(100);
  auto& p1 = p[0];
  p1 = CreateReadRequest(0, qp1.first, 4096 * n_response);

  rdma_->TransferRxPacket(std::make_unique<Packet>(p1), false);
  env_.RunFor(absl::Milliseconds(10));

  EXPECT_EQ(pb_.packet_log_.size(), n_response);
  uint32_t packet_size =
      4096 + rdma_->GetHeaderSize(Packet::Roce::Opcode::kReadResponseMiddle) +
      roce_config_.outer_header_size();
  uint32_t packet_time = packet_size * 8 * 1000 / 100000;
  // Because the work scheduler run at 10ns granularity, the final time is in a
  // range.
  EXPECT_LT(pb_.packet_log_.back().first - pb_.packet_log_[1].first,
            absl::Nanoseconds((packet_time + 10) * (n_response - 2)));
  EXPECT_GE(pb_.packet_log_.back().first - pb_.packet_log_[1].first,
            absl::Nanoseconds(packet_time * (n_response - 2)));
}

TEST_F(RdmaRoceModelRateLimitTest, CnpGenerationTest) {
  QpOptions qp_options;
  qp_options.dst_ip = "::1";

  std::pair<QpId, QpId> qp1 = make_pair(1, 100);
  rdma_->CreateRcQp(qp1.first, qp1.second, qp_options,
                    RdmaConnectedMode::kOrderedRc);

  auto p = CreatePacket(Packet::Roce::Opcode::kWriteOnly, 0, qp1.first, 100, 0);
  rdma_->TransferRxPacket(std::make_unique<Packet>(p), true);
  p.roce.psn = 1;
  rdma_->TransferRxPacket(std::make_unique<Packet>(p), true);
  env_.RunFor(absl::Milliseconds(10));

  EXPECT_EQ(pb_.packet_log_.size(), 2);
  for (auto packet : pb_.packet_log_) {
    auto roce = packet.second.roce;
    EXPECT_TRUE(roce.is_roce);
    EXPECT_EQ(roce.opcode, Packet::Roce::Opcode::kCongestionNotification);
    EXPECT_EQ(roce.psn, 0);
    EXPECT_EQ(roce.dest_qp_id, qp1.second);
  }
}

}  // namespace

}  // namespace isekai
