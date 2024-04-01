#include <string>

#include "glog/logging.h"
#include "isekai/common/model_interfaces.h"

#undef NaN
#include "gtest/gtest.h"
#include "isekai/testing/integration/omnest_embedded_kernel.h"

namespace isekai {
namespace {

constexpr double kSimulationTime = 0.01;  // in seconds.

// Uses a handcrafted network topology to trigger PFC.
// The traffic pattern is 1-to-1 communication. The sender sends packets to the
// receiver via a single switch. The speed of link1 (between sender and the
// switch) is 100Gbps, while the speed of link2 (between receiver and the
// switch) is 1Gbps. The offered load is 10Gbps, and the payload size is a fixed
// value 1KB.
TEST(OmnestRocePfcTest, TriggerXonAndXoff) {
  OmnestEmbeddedKernel omnest_sim(
      /* ned_file_dir = */ kTestDir,
      /* ini_file = */ "omnetpp_roce_pfc.ini",
      /* network_name = */ "TestRocePfcNetwork",
      /* simulation_time = */ kSimulationTime);
  omnest_sim.ExecuteSimulation();

  double tx_packets = omnest_sim.GetStatistics(
      "TestRocePfcNetwork.host[0].packet_builder.total_tx_packets");
  LOG(INFO) << "tx_packets: " << tx_packets;
  double rx_packets = omnest_sim.GetStatistics(
      "TestRocePfcNetwork.host[1].packet_builder.total_rx_packets");
  LOG(INFO) << "rx_packets: " << rx_packets;
  EXPECT_LT(rx_packets, tx_packets);

  // Given the offered load is 10Gbps, and the payload size is 1KB, the expected
  // number of tx packets is 10Gbps * kSimulationTime / 1KB.
  double expected_tx_packets = 10 * kGiga * kSimulationTime / (1000 * 8);
  EXPECT_LT(tx_packets, expected_tx_packets);
}

}  // namespace
}  // namespace isekai
