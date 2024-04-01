#include <filesystem>

#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "isekai/common/status_util.h"
#include "isekai/tools/simulation_kit/isekai_k8_config_generation/isekai_k8_config_generation.h"

ABSL_FLAG(std::string, simulation_sweep_config_path, "",
          "Path to the text proto SimulationSweepConfig.");

ABSL_FLAG(std::string, routing_file_path, "", "Path to the routing files.");

ABSL_FLAG(std::string, ned_file_path, "", "Path to the ned file.");

//
ABSL_FLAG(double, simulation_time_limit_ms, 1, "Simulation time limit in ms.");

ABSL_FLAG(std::string, network_config_path, "", "Path to the network config.");

namespace isekai {
absl::Status ConfigGenerationMain() {
  const std::string simulation_sweep_config_path =
      absl::GetFlag(FLAGS_simulation_sweep_config_path);
  if (simulation_sweep_config_path.empty())
    return absl::InternalError("simulation_sweep_config_path must be set.");
  if (!std::filesystem::exists(simulation_sweep_config_path)) {
    return absl::InternalError(
        absl::StrCat(simulation_sweep_config_path, " does not exist."));
  }

  const std::string routing_file_path = absl::GetFlag(FLAGS_routing_file_path);
  if (routing_file_path.empty())
    return absl::InternalError("routing_file_path must be set.");
  if (!std::filesystem::exists(routing_file_path)) {
    return absl::InternalError(
        absl::StrCat(routing_file_path, " does not exist."));
  }

  const std::string ned_file_path = absl::GetFlag(FLAGS_ned_file_path);
  if (ned_file_path.empty())
    return absl::InternalError("ned_file_path must be set.");
  if (!std::filesystem::exists(ned_file_path)) {
    return absl::InternalError(absl::StrCat(ned_file_path, " does not exist."));
  }

  const std::string network_config_path =
      absl::GetFlag(FLAGS_network_config_path);
  if (network_config_path.empty())
    return absl::InternalError("network_config_path must be set.");
  if (!std::filesystem::exists(network_config_path)) {
    return absl::InternalError(
        absl::StrCat(network_config_path, " does not exist."));
  }

  const int simulation_time_limit_ms =
      absl::GetFlag(FLAGS_simulation_time_limit_ms);
  IsekaiK8ConfigGeneration config = IsekaiK8ConfigGeneration(
      simulation_sweep_config_path, routing_file_path, ned_file_path,
      simulation_time_limit_ms, network_config_path);
  return config.GenerateSimulationConfig();
}
}  // namespace isekai

int main(int argc, char** argv) {
  absl::Status status = isekai::ConfigGenerationMain();
  if (!status.ok()) {
    LOG(FATAL) << "Unexpected error: " << status;
  }

  return 0;
}
