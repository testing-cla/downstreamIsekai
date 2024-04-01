#ifndef ISEKAI_TOOLS_SIMULATION_KIT_ISEKAI_K8_CONFIG_GENERATION_ISEKAI_K8_CONFIG_GENERATION_H_
#define ISEKAI_TOOLS_SIMULATION_KIT_ISEKAI_K8_CONFIG_GENERATION_ISEKAI_K8_CONFIG_GENERATION_H_

#include <string>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "gtest/gtest_prod.h"
#include "isekai/tools/simulation_kit/common/simulation_kit.pb.h"
#include "isekai/tools/simulation_kit/isekai_k8_config_generation/isekai_paths_storage.pb.h"

namespace isekai {

// Creates the Configuration files for the Isekai Simulation (ini, and
// simulation_config) and stores them locally inside of their designated
// folders. The ned and routing files are already given. All these paths are
// stored in a proto file to help with creating the docker image. With this
// files we can then create the docker images for running of the simulation.
class IsekaiK8ConfigGeneration {
 public:
  IsekaiK8ConfigGeneration(absl::string_view simulation_sweep_config_path,
                           absl::string_view routing_file_path,
                           absl::string_view ned_file_path,
                           double simulation_time_limit_ms,
                           absl::string_view network_config_path)
      : simulation_sweep_config_path_(simulation_sweep_config_path),
        simulation_time_limit_ms_(simulation_time_limit_ms),
        network_config_path_(network_config_path) {
    generated_file_paths_.set_simultion_sweep_config_path(
        simulation_sweep_config_path.data());
    generated_file_paths_.set_ned_file_path(ned_file_path.data());
    generated_file_paths_.set_routing_file_path(routing_file_path.data());
  }

  // Runs the config generation by creating the simulation config and ini files.
  // It also creates a proto to store all the paths for each simulation.
  absl::Status GenerateSimulationConfig();

 private:
  FRIEND_TEST(IsekaiK8ConfigGenerationTest, GenerateIniFile);
  FRIEND_TEST(IsekaiK8ConfigGenerationOutputTest, IsekaiK8ConfigGeneration);
  // Path to the simulation sweep config.
  const std::string simulation_sweep_config_path_;
  // Simulation time limit.
  const double simulation_time_limit_ms_;
  // Path to the network config.
  const std::string network_config_path_;
  // Proto that stores all the paths needed for each simulation.
  ConfigGenerationCollection generated_file_paths_;
  // Stores all the paths for one simulation on the proto.
  absl::Status StorePaths(const absl::string_view& ini_path,
                          const absl::string_view& config_path,
                          const uint32_t& sim_index);
  // Generates the ini file locally using template.
  absl::Status GenerateOmnestIniFiles(const absl::string_view& config_path,
                                      const absl::string_view& sim_index);
};
}  // namespace isekai

#endif  // ISEKAI_TOOLS_SIMULATION_KIT_ISEKAI_K8_CONFIG_GENERATION_ISEKAI_K8_CONFIG_GENERATION_H_
