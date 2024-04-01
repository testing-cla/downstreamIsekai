#include <string>

#include "absl/strings/substitute.h"
#include "glog/logging.h"

#undef NaN
#include "gtest/gtest.h"
#include "isekai/testing/integration/omnest_embedded_kernel.h"

namespace isekai {
namespace {

constexpr double kSimulationTime = 0.001;  // in seconds.

// Uses a handcrafted routing table to test the functionalities of routing
// pipeline. host0 sends packets to host1 with offered load 10Gb/s.
TEST(OmnestRoceNetworkModelTest, TestPacketFlows) {
  OmnestEmbeddedKernel omnest_sim(
      /* ned_file_dir = */ kTestDir,
      /* ini_file = */ "omnetpp_roce_network_integration.ini",
      /* network_name = */ "TestRoceNetworkIntegration",
      /* simulation_time = */ kSimulationTime);
  omnest_sim.ExecuteSimulation();

  auto average_goodput_stat_name_0 =
      "traffic_generator.goodput.src_host0_0.dst_host1_0";

  double average_good_put =
      omnest_sim.GetStatistics(absl::StrCat(
          "TestRoceNetworkIntegration.host[0].", average_goodput_stat_name_0)) /
      kSimulationTime;
  LOG(INFO) << "average_good_put: " << average_good_put;

  double no_route_discards = omnest_sim.GetStatistics(
      "TestRoceNetworkIntegration.router.router.routing_pipeline.no_route_"
      "discards");
  LOG(INFO) << "no_route_discards: " << no_route_discards;
  double ttl_limit_discards = omnest_sim.GetStatistics(
      "TestRoceNetworkIntegration.router.router.routing_pipeline.ttl_limit_"
      "discards");
  LOG(INFO) << "ttl_limit_discards: " << ttl_limit_discards;
  double total_discards = omnest_sim.GetStatistics(
      "TestRoceNetworkIntegration.router.router.routing_pipeline.total_"
      "discards");
  LOG(INFO) << "total_discards: " << total_discards;

  double host0_total_tx_packets = omnest_sim.GetStatistics(
      "TestRoceNetworkIntegration.host[0].packet_builder.total_tx_packets");
  LOG(INFO) << "host0_total_tx_packets: " << host0_total_tx_packets;
  double host1_total_tx_packets = omnest_sim.GetStatistics(
      "TestRoceNetworkIntegration.host[1].packet_builder.total_tx_packets");
  LOG(INFO) << "host1_total_tx_packets: " << host1_total_tx_packets;
  double host0_total_rx_packets = omnest_sim.GetStatistics(
      "TestRoceNetworkIntegration.host[0].packet_builder.total_rx_packets");
  LOG(INFO) << "host0_total_rx_packets: " << host0_total_rx_packets;
  double host1_total_rx_packets = omnest_sim.GetStatistics(
      "TestRoceNetworkIntegration.host[1].packet_builder.total_rx_packets");
  LOG(INFO) << "host1_total_rx_packets: " << host1_total_rx_packets;

  ASSERT_NEAR(average_good_put, 10, 0.5);
  // No packet should be dropped.
  EXPECT_EQ(no_route_discards, 0);
  EXPECT_EQ(ttl_limit_discards, 0);
  EXPECT_EQ(total_discards, 0);
  // Host1 should ack the exact number of received packets to host0.
  EXPECT_EQ(host1_total_tx_packets, host1_total_rx_packets);

  // The router delay = routing pipeline delay + packet enqueue delay (in ns).
  double router_delay = (600 + 10) / 1e9;
  // The offered load is 10Gb/s and the packet payload is 1kB (the overhead is
  // ignored).
  int queueing_packets = router_delay * 1e10 / (1000 * 8);
  // Ideally, host[0].TX == host[1].RX. Note that the last generated TX packet
  // may not be transmitted to the network switch. Moreover, there may exist one
  // TX packet on the fly to its receiver, and one is under receiving.
  EXPECT_NEAR(host0_total_tx_packets, host1_total_rx_packets,
              3 + queueing_packets);
}

// Two RoCE hosts are connected via a complete network model. Uses explicit
// traffic pattern: host0 sends 10 packets to host1.
TEST(OmnestRoceNetworkModelTest, TestTwoRoceHostInterRack) {
  OmnestEmbeddedKernel omnest_sim(
      /* ned_file_dir = */ kTestDir,
      /* ini_file = */ "omnetpp_roce_inter_rack_simulation.ini",
      /* network_name = */ "TestRoceNetworkInterRack",
      /* simulation_time = */ kSimulationTime);
  omnest_sim.ExecuteSimulation();

  double host0_total_tx_packets = omnest_sim.GetStatistics(
      "TestRoceNetworkInterRack.host0.packet_builder.total_tx_packets");
  double host1_total_rx_packets = omnest_sim.GetStatistics(
      "TestRoceNetworkInterRack.host1.packet_builder.total_rx_packets");
  LOG(INFO) << "host0 tx: " << host0_total_tx_packets
            << " host1 rx: " << host1_total_rx_packets;

  // The router delay = routing pipeline delay + packet enqueue delay (in ns).
  double router_delay = (600 + 10) / 1e9;
  // The offered load is 10Gbps and the packet size is 1KB.
  int queueing_packets = router_delay * 1e10 / (1000 * 8);
  // host[0].TX == host[1].RX. Note that the last
  // generated TX packet may not be transmitted to the network switch. Moreover,
  // there may exist one TX packet on the fly to its receiver, and one is under
  // receiving.
  EXPECT_NEAR(host0_total_tx_packets, host1_total_rx_packets,
              3 + queueing_packets);

  // Checks if there is any packet drop in the network.
  for (int i = 0; i < 124; i++) {
    double no_route_discards = omnest_sim.GetStatistics(
        absl::Substitute("TestRoceNetworkInterRack.routers[$0].router.routing_"
                         "pipeline.no_route_discards",
                         i));
    EXPECT_EQ(no_route_discards, 0);

    double ttl_limit_discards = omnest_sim.GetStatistics(
        absl::Substitute("TestRoceNetworkInterRack.routers[$0].router.routing_"
                         "pipeline.ttl_limit_discards",
                         i));
    EXPECT_EQ(ttl_limit_discards, 0);

    double total_discards = omnest_sim.GetStatistics(
        absl::Substitute("TestRoceNetworkInterRack.routers[$0].router.routing_"
                         "pipeline.total_discards",
                         i));
    EXPECT_EQ(total_discards, 0);
  }
}

}  // namespace
}  // namespace isekai
