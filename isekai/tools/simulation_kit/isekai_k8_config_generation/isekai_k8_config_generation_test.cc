#include "isekai/tools/simulation_kit/isekai_k8_config_generation/isekai_k8_config_generation.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "isekai/common/file_util.h"
#include "isekai/common/status_util.h"
#include "isekai/tools/simulation_kit/common/config_generation_util.h"
#include "isekai/tools/simulation_kit/common/simulation_kit.pb.h"
#include "isekai/tools/simulation_kit/isekai_k8_config_generation/isekai_paths_storage.pb.h"

namespace isekai {

namespace {

constexpr double kSimTimeLimit = 2;

constexpr char kIniFileTestingTemplate[] = R"([General]
outputscalarmanager-class = "isekai::GoogleOutputScalarManager"
outputvectormanager-class = "isekai::GoogleOutputVectorManager"

network = $0
sim-time-limit = $1ms
simtime-resolution = ns

*.simulation_config = "./simulation_config_files/$2/simulation_config.pb.txt")";

absl::StatusOr<std::string> ReadFile(const std::string& file_path,
                                     const std::string& file_name) {
  auto input_file = FileJoinPath(file_path, file_name);
  if (!std::filesystem::exists(input_file)) {
    return absl::InternalError(absl::StrCat(input_file, " does not exist."));
  }

  std::ifstream ifs(input_file);
  std::ostringstream sstr;
  sstr << ifs.rdbuf();
  return sstr.str();
}

}  // namespace

// Test for the function GenerateIniFile.
TEST(IsekaiK8ConfigGenerationTest, GenerateIniFile) {
  // Sets the temporary working directory
  std::string file_path = testing::TempDir();

  // Sets the path needed to run the test.
  const std::string kConfigPath = FileJoinPath(file_path, "0");
  const std::string kSimConfigPath =
      FileJoinPath(file_path, "simulation_config.pb.txt");
  const std::string kSweepPath = FileJoinPath(file_path, "sweep.pb.txt");
  const std::string kRoutingPath = FileJoinPath(file_path, "routing");
  const std::string kNedPath = FileJoinPath(file_path, "ned_files");
  const std::string kSimResultsPath = FileJoinPath(file_path, "results");
  const std::string network_config_path =
      FileJoinPath(file_path, "network_node_config.pb.txt");

  // Creates the file structure needed to run the test.
  CHECK_OK(RecreateDir(kConfigPath));
  CHECK_OK(WriteStringToFile(kSimConfigPath, ""));
  CHECK_OK(WriteStringToFile(kSweepPath, ""));
  CHECK_OK(RecreateDir(kRoutingPath));
  CHECK_OK(RecreateDir(kNedPath));
  CHECK_OK(RecreateDir(kSimResultsPath));
  CHECK_OK(WriteStringToFile(network_config_path, ""));

  // Runs the function that is being tested.
  IsekaiK8ConfigGeneration ini_testing = IsekaiK8ConfigGeneration(
      kSweepPath, kRoutingPath, kNedPath, kSimTimeLimit, network_config_path);
  CHECK_OK(ini_testing.GenerateOmnestIniFiles(kConfigPath, "0"));

  ASSERT_OK_THEN_ASSIGN(const std::string ini_content,
                        ReadFile(kConfigPath, "omnetpp_simulation.ini"));

  const std::string ini_file_content = absl::Substitute(
      kIniFileTestingTemplate, "SimulationNetwork", kSimTimeLimit, "0");

  // Verifies if the ini created by the function is being written correctly.
  EXPECT_EQ(ini_content, ini_file_content);
}

TEST(IsekaiK8ConfigGenerationOutputTest, IsekaiK8ConfigGeneration) {
  // Sets the temporary working directory.
  std::string file_path = testing::TempDir();

  // Sets the path variables.
  const std::string kSimConfigPath = "simulation_config.pb.txt";
  const std::string kSweepPath = FileJoinPath(file_path, "sweep.pb.txt");
  const std::string kNedPath = FileJoinPath(file_path, "ned_files");
  const std::string kRoutingPath = FileJoinPath(file_path, "routing");
  const std::string kConfigOutputPath = FileJoinPath(file_path, "output");
  const std::string network_config_path =
      FileJoinPath(file_path, "network_node_config.pb.txt");

  // Creates the file structure needed to run the config generation
  CHECK_OK(RecreateDir(kRoutingPath));
  CHECK_OK(RecreateDir(kNedPath));
  CHECK_OK(RecreateDir(kConfigOutputPath));
  CHECK_OK(WriteStringToFile(network_config_path, ""));

  // Sets up the simulation sweep
  SimulationSweepConfig sweep;
  sweep.set_simulation_sweep_name("Sweep");
  sweep.set_config_output_path(kConfigOutputPath);
  SimulationSweepConfig_SimulationConfigParameter* param =
      sweep.add_simulation_config_parameters();
  param->set_name("network.host_configs.roce_configuration.rate_limit");
  param->set_value("true");

  // Creates the file for the sweep
  std::string sweep_textproto;
  google::protobuf::TextFormat::PrintToString(sweep, &sweep_textproto);
  CHECK_OK(WriteStringToFile(kSweepPath, sweep_textproto));

  // Runs the code to generate the config generation
  IsekaiK8ConfigGeneration config_output_testing = IsekaiK8ConfigGeneration(
      kSweepPath, kRoutingPath, kNedPath, kSimTimeLimit, network_config_path);
  CHECK_OK(config_output_testing.GenerateSimulationConfig());

  // Tests the results from the config generation
  const std::string proto_path =
      FileJoinPath(kConfigOutputPath, "config_generation_file_paths.pb.txt");
  const std::string config_folder_path = FileJoinPath(kConfigOutputPath, "0");

  EXPECT_TRUE(std::filesystem::exists(proto_path));

  ConfigGenerationCollection proto;
  proto.set_simultion_sweep_config_path(kSweepPath);
  proto.set_ned_file_path(kNedPath);
  proto.set_routing_file_path(kRoutingPath);
  ConfigGenerationPaths* test_data = proto.add_paths();
  test_data->set_ini_file_path(
      FileJoinPath(config_folder_path, "omnetpp_simulation.ini"));
  test_data->set_simulation_config_output_path(
      FileJoinPath(config_folder_path, kSimConfigPath));
  test_data->set_simulation_index(0);

  EXPECT_TRUE(std::filesystem::exists(
      FileJoinPath(config_folder_path, kSimConfigPath)));
  EXPECT_TRUE(std::filesystem::exists(
      FileJoinPath(config_folder_path, "omnetpp_simulation.ini")));

  ASSERT_OK_THEN_ASSIGN(
      const std::string proto_content,
      ReadFile(kConfigOutputPath, "config_generation_file_paths.pb.txt"));

  std::string proto_text;
  google::protobuf::TextFormat::PrintToString(proto, &proto_text);
  EXPECT_EQ(proto_text, proto_content);
}

}  // namespace isekai
