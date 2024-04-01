#include "isekai/testing/integration/omnest_roce_host.h"

#include <memory>
#include <string>
#include <utility>

#include "absl/status/status.h"
#include "glog/logging.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/file_util.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/status_util.h"
#include "isekai/host/rdma/rdma_configuration.h"
#include "isekai/host/rdma/rdma_roce_cc_dcqcn.h"
#include "isekai/host/rdma/rdma_roce_model.h"
#include "isekai/host/rnic/connection_manager.h"
#include "isekai/host/rnic/rnic.h"
#include "isekai/host/traffic/traffic_generator.h"

// Registers RoceHost module into OMNest.
// This macro prevents putting the code input namespace isekai.
Define_Module(RoceHost);

void RoceHost::initialize(int stage) {
  if (stage == 0) {
    LOG(INFO) << "init RoCE components (stage 0).";
    initialize();
    return;
  }

  if (stage == 1) {
    LOG(INFO) << "init RDMA traffics (stage 1).";
    isekai::SimulationConfig simulation;
    std::string config_file =
        getSystemModule()->par("simulation_config").stringValue();
    CHECK_OK(isekai::ReadTextProtoFromFile(config_file, &simulation));

    if (InitializeTrafficGenerator(
            isekai::ConnectionManager::GetConnectionManager(),
            simulation.traffic_pattern())
            .ok()) {
      LOG(INFO) << get_host_id() << " creates traffic generator.";
    }
    get_traffic_generator()->GenerateRdmaOp();
    LOG(INFO) << get_host_id() << " starts to generate traffic.";
    return;
  }

  LOG(FATAL) << "unsupported initialization stage!";
}

void RoceHost::initialize() {
  // Loads the host configurations.
  isekai::SimulationConfig simulation;
  std::string simulation_config_file =
      getSystemModule()->par("simulation_config").stringValue();
  CHECK(isekai::ReadTextProtoFromFile(simulation_config_file, &simulation).ok())
      << "fails to load simulation config proto text.";

  // Loads the host id/ip information.
  InitializeHostIdAndIp(simulation);
  LOG(INFO) << get_host_id() << " has index: " << this->getIndex();

  InitializeOmnestEnvironment();
  if (get_omnest_env() != nullptr) {
    LOG(INFO) << get_host_id() << " creates omnest env.";
  }
  InitializeStatisticCollection(simulation);

  // Initializes connection manager with the simulation network configurations.
  auto connection_manager = isekai::ConnectionManager::GetConnectionManager();
  connection_manager->PopulateHostInfo(simulation.network());

  // Initializes the RDMA RoCE model.
  isekai::RdmaConfig rdma_options =
      isekai::DefaultConfigGenerator::DefaultRdmaConfig();
  GetRdmaConfig(simulation, rdma_options);

  isekai::RoceConfig roce_configs =
      isekai::DefaultConfigGenerator::DefaultRoceConfig();
  GetRoceConfig(simulation, roce_configs);
  auto rdma_roce = std::make_unique<isekai::RdmaRoceModel>(
      rdma_options, roce_configs, simulation.network().mtu(), get_omnest_env(),
      get_stats_collection(), connection_manager);

  isekai::RdmaCcDcqcnOptions cc_options;
  cc_options.type = isekai::RdmaCcOptions::Type::kDcqcn;
  if (!roce_configs.rate_limit()) {
    cc_options.max_rate = cc_options.min_rate = -1;
  }
  rdma_roce->SetupCc(&cc_options);

  // Initializes the RNIC for RoCE host. For now, the roce in included in
  // RdmaRoceModel, so we only need to pass the rdma model to RNIC.
  InitializeRNIC(/* hif = */ nullptr, /* rdma = */ std::move(rdma_roce),
                 /* roce = */ nullptr,
                 /* traffic_shaper = */ nullptr, /* falcon = */ nullptr);
  if (get_rnic()->get_rdma_model() != nullptr) {
    LOG(INFO) << get_host_id() << " creates rdma.";
  }
  if (get_rnic()->get_roce_model() != nullptr) {
    LOG(INFO) << get_host_id() << " creates roce.";
  }
  get_rnic()->RegisterToConnectionManager(get_host_id(), connection_manager);

  if (InitializePacketBuilder().ok()) {
    LOG(INFO) << get_host_id() << " creates packet builder.";
  }
  get_rnic()->get_roce_model()->ConnectPacketBuilder(get_packet_builder());
}
