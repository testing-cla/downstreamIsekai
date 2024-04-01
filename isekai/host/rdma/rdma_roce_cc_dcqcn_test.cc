#include "isekai/host/rdma/rdma_roce_cc_dcqcn.h"

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
#include "gtest/gtest.h"
#include "isekai/common/common_util.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/simple_environment.h"
#include "isekai/common/status_util.h"
#include "isekai/host/rdma/rdma_base_model.h"
#include "isekai/host/rdma/rdma_component_interfaces.h"
#include "isekai/host/rdma/rdma_configuration.h"
#include "isekai/host/rdma/rdma_qp_manager.h"
#include "isekai/host/rdma/rdma_roce_model.h"
#include "omnetpp/cmessage.h"

namespace isekai {

namespace {

using std::make_pair;
using std::pair;
using std::vector;

class FakePacketBuilder : public PacketBuilderInterface {
 public:
  explicit FakePacketBuilder(Environment* env) : env_(env) {}
  void EnqueuePacket(std::unique_ptr<Packet> packet) override {}

  void ExtractAndHandleTransportPacket(
      omnetpp::cMessage* network_packet) override {}

  void EncapsulateAndSendTransportPacket(uint16_t priority) override {}

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
  void PopulateHostInfo(
      const isekai::NetworkConfig& simulation_network) override {}
  absl::StatusOr<std::string> GetIpAddress(absl::string_view host_id) override {
    return "::1";
  }
};

}  // namespace

TEST(RdmaRoceCcDcqcnTest, RateUpdate) {
  SimpleEnvironment env;

  RdmaConfig config = DefaultConfigGenerator::DefaultRdmaConfig();
  uint32_t mtu_size = 4096 + kEthernetHeader + kIpv6Header + kUdpHeader +
                      kPspHeader + kFalconHeader + kFalconOpHeader +
                      kRdmaHeader + kPspTrailer;
  FakeConnectionManager connection_manager;
  RoceConfig roce_config;
  RdmaRoceModel rdma(config, roce_config, mtu_size, &env,
                     /*stats_collector=*/nullptr, &connection_manager);

  FakePacketBuilder pb(&env);
  rdma.ConnectPacketBuilder(&pb);

  RdmaCcDcqcnOptions cc_options;
  cc_options.min_rate = 100;
  cc_options.type = RdmaCcOptions::Type::kDcqcn;
  rdma.SetupCc(&cc_options);

  QpOptions qp_options;
  qp_options.dst_ip = "::1";

  std::pair<QpId, QpId> qp1 = make_pair(1, 100);
  rdma.CreateRcQp(qp1.first, qp1.second, qp_options,
                  RdmaConnectedMode::kOrderedRc);

  RoceQpContext* context =
      down_cast<RoceQpContext*>(rdma.qp_manager_.DirectQpLookup(qp1.first));
  RdmaRoceCcDcqcn* cc = down_cast<RdmaRoceCcDcqcn*>(rdma.cc_.get());
  auto state = context->dcqcn;
  uint64_t rate = context->rate;

  vector<Packet> p(100);
  Packet& cnp = p[0];
  cnp.roce.opcode = Packet::Roce::Opcode::kCongestionNotification;

  // Test handle the first CNP.
  cc->HandleRxPacket(context, &cnp);
  CHECK_EQ(context->rate, cc_options.rate_to_set_on_first_cnp);
  CHECK_EQ(context->dcqcn.target_rate, cc_options.rate_to_set_on_first_cnp);
  CHECK_EQ(context->dcqcn.alpha, cc_options.initial_alpha_value);
  env.RunFor(absl::Nanoseconds(1));
  state = context->dcqcn;
  rate = context->rate;

  // Test timer-based rate increase.
  env.RunFor(absl::Microseconds(cc_options.time_reset));
  CHECK_EQ(context->dcqcn.target_rate, state.target_rate);
  CHECK_EQ(context->rate, (rate + state.target_rate) / 2);
  state = context->dcqcn;
  rate = context->rate;

  // Test alpha timer with CNP arrival.
  cc->HandleRxPacket(context, &cnp);
  env.RunFor(absl::Microseconds(cc_options.dce_alpha_update_period));
  CHECK_EQ(context->dcqcn.alpha, cc_options.dce_alpha_g * state.alpha / 1024 +
                                     1024 - cc_options.dce_alpha_g);
  state = context->dcqcn;
  rate = context->rate;

  // Test alpha timer without CNP arrival.
  env.RunFor(absl::Microseconds(cc_options.dce_alpha_update_period));
  CHECK_EQ(context->dcqcn.alpha, cc_options.dce_alpha_g * state.alpha / 1024);
  state = context->dcqcn;
  rate = context->rate;

  // Test the first rate reduction timer.
  env.RunFor(absl::Microseconds(cc_options.rate_reduce_monitor_period) -
             absl::Microseconds(cc_options.dce_alpha_update_period) * 2);
  CHECK_LT(context->rate, rate);
  CHECK_EQ(context->dcqcn.target_rate, rate);
  state = context->dcqcn;
  rate = context->rate;

  // Test consecutive rate reduction.
  cc->HandleRxPacket(context, &cnp);
  env.RunFor(absl::Microseconds(cc_options.rate_reduce_monitor_period));
  CHECK_LT(context->rate, rate);
  CHECK_EQ(context->dcqcn.target_rate, state.target_rate);
  state = context->dcqcn;
  rate = context->rate;

  // Test timer-based rate increase.
  env.RunFor(absl::Microseconds(cc_options.time_reset) - absl::Nanoseconds(2));
  CHECK_EQ(context->dcqcn.target_rate, state.target_rate);
  CHECK_EQ(context->rate, rate);
  env.RunFor(absl::Nanoseconds(2));
  CHECK_EQ(context->dcqcn.target_rate, state.target_rate);
  CHECK_EQ(context->rate, (rate + state.target_rate) / 2);
  CHECK_EQ(context->dcqcn.increase_stage_by_timer, 1);
  CHECK_EQ(context->dcqcn.increase_stage_by_byte, 0);
  state = context->dcqcn;
  rate = context->rate;

  // Test rate reduction after timer-based rate increase.
  cc->HandleRxPacket(context, &cnp);
  env.RunFor(absl::Microseconds(cc_options.rate_reduce_monitor_period));
  CHECK_EQ(context->dcqcn.target_rate, rate);
  CHECK_LT(context->rate, rate);
  CHECK_EQ(context->dcqcn.increase_stage_by_timer, 0);
  CHECK_EQ(context->dcqcn.increase_stage_by_byte, 0);
  CHECK_EQ(context->dcqcn.byte_count, 0);
  state = context->dcqcn;
  rate = context->rate;

  // Test byte-based rate increase.
  cc->SentBytes(context, cc_options.byte_reset * 64);
  CHECK_EQ(context->dcqcn.target_rate, state.target_rate);
  CHECK_EQ(context->rate, (rate + state.target_rate) / 2);
  CHECK_EQ(context->dcqcn.increase_stage_by_timer, 0);
  CHECK_EQ(context->dcqcn.increase_stage_by_byte, 1);
  state = context->dcqcn;
  rate = context->rate;

  // Test rate reduction after byte-based rate increase.
  cc->HandleRxPacket(context, &cnp);
  env.RunFor(absl::Microseconds(cc_options.rate_reduce_monitor_period));
  CHECK_EQ(context->dcqcn.target_rate, rate);
  CHECK_LT(context->rate, rate);
  CHECK_EQ(context->dcqcn.increase_stage_by_timer, 0);
  CHECK_EQ(context->dcqcn.increase_stage_by_byte, 0);
  CHECK_EQ(context->dcqcn.byte_count, 0);
  state = context->dcqcn;
  rate = context->rate;

  // Test additive increase.
  cc->SentBytes(context, cc_options.byte_reset * 64 * 2);
  CHECK_EQ(context->dcqcn.target_rate, state.target_rate + cc_options.ai_rate);
  CHECK_EQ(context->rate, ((rate + state.target_rate) / 2 + state.target_rate +
                           cc_options.ai_rate) /
                              2);
  CHECK_EQ(context->dcqcn.increase_stage_by_timer, 0);
  CHECK_EQ(context->dcqcn.increase_stage_by_byte, 2);
  state = context->dcqcn;
  rate = context->rate;

  // Test hyper increase.
  env.RunFor(absl::Microseconds(cc_options.time_reset));
  CHECK_EQ(context->dcqcn.target_rate, state.target_rate + cc_options.ai_rate);
  CHECK_EQ(context->rate, (rate + state.target_rate + cc_options.ai_rate) / 2);
  CHECK_EQ(context->dcqcn.increase_stage_by_timer, 1);
  CHECK_EQ(context->dcqcn.increase_stage_by_byte, 2);
  state = context->dcqcn;
  rate = context->rate;

  env.RunFor(absl::Microseconds(cc_options.time_reset));
  CHECK_EQ(context->dcqcn.target_rate, state.target_rate + cc_options.ai_rate);
  CHECK_EQ(context->rate, (rate + state.target_rate + cc_options.ai_rate) / 2);
  CHECK_EQ(context->dcqcn.increase_stage_by_timer, 2);
  CHECK_EQ(context->dcqcn.increase_stage_by_byte, 2);
  state = context->dcqcn;
  rate = context->rate;

  env.RunFor(absl::Microseconds(cc_options.time_reset));
  CHECK_EQ(context->dcqcn.target_rate, state.target_rate + cc_options.hai_rate);
  CHECK_EQ(context->rate, (rate + state.target_rate + cc_options.hai_rate) / 2);
  CHECK_EQ(context->dcqcn.increase_stage_by_timer, 3);
  CHECK_EQ(context->dcqcn.increase_stage_by_byte, 2);
  state = context->dcqcn;
  rate = context->rate;

  // Test rate reduction.
  cc->HandleRxPacket(context, &cnp);
  env.RunFor(absl::Microseconds(cc_options.rate_reduce_monitor_period));
  CHECK_EQ(context->dcqcn.target_rate, rate);
  CHECK_LT(context->rate, rate);
  CHECK_EQ(context->dcqcn.increase_stage_by_timer, 0);
  CHECK_EQ(context->dcqcn.increase_stage_by_byte, 0);
  CHECK_EQ(context->dcqcn.byte_count, 0);
  state = context->dcqcn;
  rate = context->rate;

  // Test minimum rate.
  for (int i = 0; i < 1000; i++) {
    cc->HandleRxPacket(context, &cnp);
    env.RunFor(absl::Microseconds(cc_options.dce_alpha_update_period));
    CHECK_EQ(context->dcqcn.target_rate, state.target_rate);
    CHECK_GE(context->rate, cc_options.min_rate);
    CHECK_EQ(context->dcqcn.increase_stage_by_timer, 0);
    CHECK_EQ(context->dcqcn.increase_stage_by_byte, 0);
    CHECK_EQ(context->dcqcn.byte_count, 0);
    state = context->dcqcn;
    rate = context->rate;
  }
}

}  // namespace isekai
