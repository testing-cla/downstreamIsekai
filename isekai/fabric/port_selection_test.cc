#include "isekai/fabric/port_selection.h"

#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>

#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3Tools.h"
#include "inet/networklayer/ipv6/Ipv6Header.h"
#include "internal/testing.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/status_util.h"  // IWYU pragma: keep
#include "isekai/fabric/constants.h"
#include "isekai/fabric/network_routing_table.h"
#include "isekai/fabric/routing_test_util.h"
#include "omnetpp/cmessage.h"
#include "omnetpp/cownedobject.h"
#include "omnetpp/csimulation.h"

namespace isekai {

namespace {

// Uses different packets to test if the selected ports will be different.
TEST(PortSelectionTest, TestSelectOutputPortForPacketWcmp) {
  omnetpp::cStaticFlag dummy;
  omnetpp::cSimulation* dummy_simulator = SetupDummyOmnestSimulation();

  const std::string input_path = absl::StrCat(testing::TempDir(), "input_path");
  if (!std::filesystem::exists(input_path)) {
    ASSERT_TRUE(std::filesystem::create_directories(input_path));
  }
  const std::string input_file =
      absl::StrCat(input_path, "/routing_0.recordio");
  ASSERT_OK(WriteTestRoutingConfigToFile(input_file));

  ASSERT_OK_THEN_ASSIGN(auto network_routing_table,
                        NetworkRoutingTable::Create(input_file));

  const std::unique_ptr<isekai::OutputPortSelection> output_port_selection =
      isekai::OutputPortSelectionFactory::GetOutputPortSelectionScheme(
          RouterConfigProfile::WCMP);

  auto test_packet_1 = GenerateTestInetPacket();
  ASSERT_OK_THEN_ASSIGN(const auto* table_output_options_1,
                        network_routing_table->GetOutputOptionsAndModifyPacket(
                            test_packet_1.get()));

  const std::string dst_ipv6_address = "2001:db8:85a3::1";
  auto test_packet_2 =
      GenerateTestInetPacket(/* src_ipv6_address = */ kDefaultSrcIpv6Address,
                             /* dst_ipv6_address = */ dst_ipv6_address);
  ASSERT_OK_THEN_ASSIGN(const auto* table_output_options_2,
                        network_routing_table->GetOutputOptionsAndModifyPacket(
                            test_packet_2.get()));

  // The output options for test_packet_1 and test_packet_2 are different, and
  // thus the selected output port should be different too. This is to test that
  // given different input to the SelectOutputPort(), the output may vary.
  EXPECT_NE(output_port_selection->SelectOutputPort(*table_output_options_1,
                                                    test_packet_1.get(), 1),
            output_port_selection->SelectOutputPort(*table_output_options_2,
                                                    test_packet_2.get(), 1));

  CloseOmnestSimulation(dummy_simulator);
}

TEST(PortSelectionTest, TestHashPolarization) {
  omnetpp::cStaticFlag dummy;
  omnetpp::cSimulation* dummy_simulator = SetupDummyOmnestSimulation();

  const std::string input_path = absl::StrCat(testing::TempDir(), "input_path");
  if (!std::filesystem::exists(input_path)) {
    ASSERT_TRUE(std::filesystem::create_directories(input_path));
  }
  const std::string input_file =
      absl::StrCat(input_path, "/routing_0.recordio");
  ASSERT_OK(WriteTestRoutingConfigToFile(input_file));

  ASSERT_OK_THEN_ASSIGN(auto network_routing_table,
                        NetworkRoutingTable::Create(input_file));

  const std::unique_ptr<isekai::OutputPortSelection> output_port_selection =
      isekai::OutputPortSelectionFactory::GetOutputPortSelectionScheme(
          RouterConfigProfile::WCMP);

  const std::string dst_ipv6_address = "2001:db8:85a3::1";
  auto test_packet =
      GenerateTestInetPacket(/* src_ipv6_address = */ kDefaultSrcIpv6Address,
                             /* dst_ipv6_address = */ dst_ipv6_address,
                             /* flow_src_mac = */ kFlowSrcMacVrfUp);
  ASSERT_OK_THEN_ASSIGN(const auto* table_output_options,
                        network_routing_table->GetOutputOptionsAndModifyPacket(
                            test_packet.get()));

  // Given the same output options and packet, with hash bit rotation, the
  // selected output port should be different for S1 and S2 routers.
  EXPECT_NE(output_port_selection->SelectOutputPort(*table_output_options,
                                                    test_packet.get(), 1),
            output_port_selection->SelectOutputPort(*table_output_options,
                                                    test_packet.get(), 2));

  CloseOmnestSimulation(dummy_simulator);
}

}  // namespace

}  // namespace isekai
