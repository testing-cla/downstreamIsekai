#include "isekai/tools/simulation_kit/isekai_k8_config_generation/isekai_k8_config_generation.h"

#include <filesystem>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"  // IWYU pragma: keep
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/text_format.h"
#include "isekai/common/file_util.h"
#include "isekai/tools/simulation_kit/common/config_generation_util.h"
#include "isekai/tools/simulation_kit/common/simulation_kit.pb.h"

namespace isekai {

namespace {

// File name of the simulation sweep parameter differences.
constexpr char kSimulationSweepParameterFileName[] =
    "simulation_sweep_parameter_difference.txt";

// File name for the output file of the config generation that contains the
// paths of the generated files.
constexpr char kConfigGenerationPathProtoName[] =
    "config_generation_file_paths.pb.txt";

// Template for ini file for OMNest simulation. $0 is the simulation network
// name which should be unique; $1 is the simulation time in millisecond; $2 is
// the index for the simulation. This generated ini file is used inside the GKE
// container. For this reason, the simulation config is pre-fixed.
constexpr char kIniFileTemplate[] = R"([General]
outputscalarmanager-class = "isekai::GoogleOutputScalarManager"
outputvectormanager-class = "isekai::GoogleOutputVectorManager"

network = $0
sim-time-limit = $1ms
simtime-resolution = ns

*.simulation_config = "./simulation_config_files/$2/simulation_config.pb.txt")";

}  // namespace

absl::Status IsekaiK8ConfigGeneration::StorePaths(
    const absl::string_view& ini_path, const absl::string_view& config_path,
    const uint32_t& sim_index) {
  ConfigGenerationPaths* paths = generated_file_paths_.add_paths();
  paths->set_simulation_config_output_path(
      FileJoinPath(config_path, "simulation_config.pb.txt"));
  paths->set_ini_file_path(ini_path.data());
  paths->set_simulation_index(sim_index);
  return absl::OkStatus();
}

absl::Status IsekaiK8ConfigGeneration::GenerateOmnestIniFiles(
    const absl::string_view& config_path, const absl::string_view& sim_index) {
  const std::string ini_file_content =
      absl::Substitute(kIniFileTemplate, "SimulationNetwork",
                       simulation_time_limit_ms_, sim_index);

  RETURN_IF_ERROR(WriteStringToFile(
      FileJoinPath(config_path, "omnetpp_simulation.ini"), ini_file_content));
  return absl::OkStatus();
}

absl::Status IsekaiK8ConfigGeneration::GenerateSimulationConfig() {
  SimulationSweepConfig simulation_sweep_config;

  // This method is needed because we need to store the sweep text proto into a
  // proto file.
  RETURN_IF_ERROR(ReadTextProtoFromFile(simulation_sweep_config_path_,
                                        &simulation_sweep_config));

  const absl::string_view config_output_path =
      simulation_sweep_config.config_output_path();

  // This call creates the local directory for the config output path even if
  // path exists.
  RETURN_IF_ERROR(RecreateDir(config_output_path));

  // Stores the parameter names from the sweep for the simulation config.
  std::vector<std::string> parameter_names;
  // Stores the parameter values from the sweep for the simulation config.
  std::vector<std::vector<SweepParameterValue>> parameter_values;

  RETURN_IF_ERROR(ExtractSimulationSweepParameters(
      simulation_sweep_config, &parameter_names, &parameter_values));

  ASSIGN_OR_RETURN(
      auto cartesian_product_generator,
      CartesianProductGenerator<SweepParameterValue>::Create(parameter_values));
  const std::vector<std::vector<SweepParameterValue>>& cartesian_product =
      cartesian_product_generator->GetCartesianProduct();

  std::vector<std::string> simulation_config_paths;

  RETURN_IF_ERROR(WriteSimulationConfig(
      config_output_path, parameter_names, cartesian_product,
      &simulation_config_paths, network_config_path_));

  if (cartesian_product.size() > 1) {
    RETURN_IF_ERROR(WriteStringToFile(
        FileJoinPath(config_output_path, kSimulationSweepParameterFileName),
        GenerateSimulationParameterDifference(parameter_names, parameter_values,
                                              cartesian_product)));
  }

  // Generates the ini files and stores the paths in the proto that stores all
  // the paths.
  for (const std::string& config_path : simulation_config_paths) {
    const std::vector<std::string> sim_index_folders =
        absl::StrSplit(config_path, '/');
    const std::string sim_index =
        sim_index_folders[sim_index_folders.size() - 1];
    RETURN_IF_ERROR(GenerateOmnestIniFiles(config_path, sim_index));
    RETURN_IF_ERROR(
        StorePaths(FileJoinPath(config_path, "omnetpp_simulation.ini"),
                   config_path, std::stoi(sim_index)));
  }

  std::string generated_file_paths_textproto;
  google::protobuf::TextFormat::PrintToString(generated_file_paths_,
                                              &generated_file_paths_textproto);
  RETURN_IF_ERROR(WriteStringToFile(
      FileJoinPath(config_output_path, kConfigGenerationPathProtoName),
      generated_file_paths_textproto));

  return absl::OkStatus();
}

}  // namespace isekai
