#include "isekai/tools/simulation_kit/common/config_generation_util.h"

#include <filesystem>  //NOLINT
#include <memory>
#include <vector>

#include "absl/container/btree_map.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/text_format.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/file_util.h"

namespace isekai {

namespace {
// Name of the isekai::SimulationConfig text proto.
constexpr char kSimulationConfigTextProtoFileName[] =
    "simulation_config.pb.txt";

// Number of threads for writing/reading.
constexpr int kNumThread = 16;

// Number of CPU slots for writing/reading.
constexpr int kMaxCpuSlots = 4;

// The template of one simulation sweep parameter. $0 is the parameter name; $1
// is the parameter value.
constexpr char kParameterTemplate[] = R"(
  $0 : $1)";

// The separator for simulation parameters for different simulations.
constexpr char kSimulationSeparator[] = "\n\n";

// The template of the simulation sweep parameter for each simulation. $0 is the
// simulation index; $1 is for the paratemers.
constexpr char kSimulationSweepParameterTemplate[] =
    R"(simulation index $0 : {$1
})";

// The template for submessage in text proto. Used for constructing text proto.
// $0 is the field name; $1 is the content inside the submessage.
constexpr char kTextProtoSubmessageTemplate[] = R"($0 {
  $1
}
)";

// The template for field (not submessage) in text proto. Used for constructing
// text proto. $0 is the field name; $1 is the value of the field.
constexpr char kTextProtoFieldTemplate[] = R"($0: $1
)";

// Converts the hierarchy parameter name to SweepParameter map to splitted names
// to value map.
absl::flat_hash_map<std::vector<std::string>, SweepParameterValue>
GetHierarchyNameToSweepParameter(
    const absl::flat_hash_map<std::string, SweepParameterValue>&
        parameter_name_to_value) {
  absl::flat_hash_map<std::vector<std::string>, SweepParameterValue>
      hierarchy_names_to_value;
  hierarchy_names_to_value.reserve(parameter_name_to_value.size());

  for (const auto& entry : parameter_name_to_value) {
    std::vector<std::string> parameter_name_splits =
        absl::StrSplit(entry.first, '.');
    hierarchy_names_to_value[parameter_name_splits] = entry.second;
  }

  return hierarchy_names_to_value;
}

// Trie for constructing text proto from parameter hierarchy name to parameter
// value map. The parameter hierarchy name are stored in the internal node. The
// parameter value are stored in the leaf node. It does not support the repeated
// submessage.
class TextProtoTrie {
 public:
  // Node for the Trie storing parameter hierarchy name to value map.
  struct TextProtoTrieNode {
    // Indicates if the node is a leaf node or not.
    bool is_leaf = true;
    // Map from name segment to child node.
    absl::flat_hash_map<std::string, std::unique_ptr<TextProtoTrieNode>>
        children;
    // Is the value a submessage.
    bool is_submessage = false;
    // Value of the parameter.
    std::string value;
  };

  TextProtoTrie() { root_ = std::make_unique<TextProtoTrieNode>(); }

  // Inserts a (parameter hierarchy name, value) pair to the Trie.
  void Insert(const std::vector<std::string>& parameter_hierarchy_names,
              const SweepParameterValue& parameter_value);

  // Gets the text proto.
  std::string GetSimulationConfigTextProto();

 private:
  // Generates the text proto when traveling the Trie.
  std::string GenerateTextProto(const std::string& parameter_name,
                                const std::unique_ptr<TextProtoTrieNode>* root);

  // Root of the Trie.
  std::unique_ptr<TextProtoTrieNode> root_;
};

void TextProtoTrie::Insert(
    const std::vector<std::string>& parameter_hierarchy_names,
    const SweepParameterValue& parameter_value) {
  const int depth = parameter_hierarchy_names.size();
  std::unique_ptr<TextProtoTrieNode>* node = &root_;

  // Goes to the node according to the parameter_hierarchy_names. Creates the
  // node in the path if not exists.
  for (int i = 0; i < depth; ++i) {
    if (!(*node)->children.contains(parameter_hierarchy_names[i])) {
      if ((*node)->is_leaf) {
        (*node)->is_leaf = false;
      }
      (*node)->children[parameter_hierarchy_names[i]] =
          std::make_unique<TextProtoTrieNode>();
    }
    node = &(*node)->children[parameter_hierarchy_names[i]];
  }
  (*node)->value = parameter_value.value;
  (*node)->is_submessage = parameter_value.is_submessage;
}

std::string TextProtoTrie::GetSimulationConfigTextProto() {
  return GenerateTextProto("", &root_);
}

std::string TextProtoTrie::GenerateTextProto(
    const std::string& parameter_name,
    const std::unique_ptr<TextProtoTrieNode>* root) {
  std::string text_proto;
  if ((*root)->is_leaf) {
    if ((*root)->is_submessage) {
      text_proto = absl::Substitute(kTextProtoSubmessageTemplate,
                                    parameter_name, (*root)->value);
    } else {
      text_proto = absl::Substitute(kTextProtoFieldTemplate, parameter_name,
                                    (*root)->value);
    }
  } else {
    for (const auto& entry : (*root)->children) {
      absl::StrAppend(&text_proto,
                      GenerateTextProto(entry.first, &entry.second));
    }
    if (root != &root_) {
      text_proto = absl::Substitute(kTextProtoSubmessageTemplate,
                                    parameter_name, text_proto);
    }
  }

  return text_proto;
}

// Worker for writing isekai::SimulationConfig to files. Invoked by
// WriteSimulationConfig().
absl::Status WriteSimulationConfigWorker(
    absl::string_view output_path,
    const std::vector<std::string>& parameter_names,
    const std::vector<std::vector<SweepParameterValue>>&
        parameter_value_cartesian_product,
    const std::vector<int>& simulation_task_indexes,
    std::optional<std::vector<std::string>*> sim_path,
    const std::optional<std::string> network_config_path) {
  for (const int task_index : simulation_task_indexes) {
    const std::string output_dir =
        FileJoinPath(output_path, absl::StrCat(task_index));
    RETURN_IF_ERROR(RecreateDir(output_dir));
    if (sim_path.has_value()) {
      sim_path.value()->push_back(output_dir);
    }
    absl::flat_hash_map<std::string, SweepParameterValue>
        parameter_name_to_value;
    for (int i = 0; i < parameter_names.size(); ++i) {
      parameter_name_to_value.insert(
          std::make_pair(parameter_names[i],
                         parameter_value_cartesian_product[task_index][i]));
    }
    ASSIGN_OR_RETURN(auto simulation_config,
                     ConstructSimulationConfig(parameter_name_to_value));
    if (network_config_path.has_value()) {
      SimulationConfig network_config;
      RETURN_IF_ERROR(
          ReadTextProtoFromFile(network_config_path.value(), &network_config));
      simulation_config.MergeFrom(network_config);
    }
    const std::string file_output_path =
        FileJoinPath(output_dir, kSimulationConfigTextProtoFileName);
    std::string simulation_config_textproto;
    google::protobuf::TextFormat::PrintToString(simulation_config,
                                                &simulation_config_textproto);
    RETURN_IF_ERROR(
        WriteStringToFile(file_output_path, simulation_config_textproto));
  }

  return absl::OkStatus();
}

}  // namespace

std::string GenerateSimulationParameterDifference(
    const std::vector<std::string>& parameter_names,
    const std::vector<std::vector<SweepParameterValue>>& parameter_values,
    const std::vector<std::vector<SweepParameterValue>>& cartesian_product) {
  std::vector<int> multivalue_parameter_index;
  for (int i = 0; i < parameter_values.size(); i++) {
    if (parameter_values[i].size() > 1) {
      multivalue_parameter_index.push_back(i);
    }
  }

  std::string simulation_parameter_difference;
  for (int simulation_idx = 0; simulation_idx < cartesian_product.size();
       simulation_idx++) {
    std::string current_simulation_parameter;
    for (const int parameter_index : multivalue_parameter_index) {
      absl::StrAppend(
          &current_simulation_parameter,
          absl::Substitute(
              kParameterTemplate, parameter_names[parameter_index],
              cartesian_product[simulation_idx][parameter_index].value));
    }
    if (simulation_idx != 0) {
      absl::StrAppend(&simulation_parameter_difference, kSimulationSeparator);
    }
    absl::StrAppend(
        &simulation_parameter_difference,
        absl::Substitute(kSimulationSweepParameterTemplate, simulation_idx,
                         current_simulation_parameter));
  }

  return simulation_parameter_difference;
}

// Deletes and creates directory.
absl::Status RecreateDir(absl::string_view file_path) {
  if (std::filesystem::exists(file_path)) {
    if (std::filesystem::remove_all(file_path) ==
        static_cast<std::uintmax_t>(-1)) {
      return absl::InternalError(
          absl::StrCat("Fail to remove dir: ", file_path));
    }
  }
  if (!std::filesystem::create_directories(file_path)) {
    return absl::InternalError(absl::StrCat("Fail to create dir: ", file_path));
  }

  return absl::OkStatus();
}

std::vector<int> GenerateShuffledIndexes(int n) {
  std::vector<int> indexes;
  indexes.reserve(n);
  for (int i = 0; i < n; i++) {
    indexes.push_back(i);
  }
  RandomShuffleVector(&indexes);

  return indexes;
}

absl::StatusOr<isekai::SimulationConfig> ConstructSimulationConfig(
    const absl::flat_hash_map<std::string, SweepParameterValue>&
        parameter_name_to_value) {
  absl::flat_hash_map<std::vector<std::string>, SweepParameterValue>
      hierarchy_names_to_parameter =
          GetHierarchyNameToSweepParameter(parameter_name_to_value);

  TextProtoTrie parameter_trie;
  for (const auto& entry : hierarchy_names_to_parameter) {
    parameter_trie.Insert(entry.first, entry.second);
  }

  isekai::SimulationConfig simulation_config;
  google::protobuf::TextFormat::ParseFromString(
      parameter_trie.GetSimulationConfigTextProto(), &simulation_config);

  return simulation_config;
}

absl::Status WriteSimulationConfig(
    absl::string_view output_path,
    const std::vector<std::string>& parameter_names,
    const std::vector<std::vector<SweepParameterValue>>&
        parameter_value_cartesian_product,
    std::optional<std::vector<std::string>*> sim_path,
    const std::optional<std::string> network_config_path) {
  const int num_simulations = parameter_value_cartesian_product.size();
  std::vector<int> simulation_task_indexes =
      GenerateShuffledIndexes(num_simulations);
  const int chunk_length =
      static_cast<int>(std::ceil(static_cast<double>(num_simulations) /
                                 std::min(num_simulations, kNumThread)));
  ASSIGN_OR_RETURN(const auto simulation_index_chunks,
                   SplitVectorToChunks(simulation_task_indexes, chunk_length));

  absl::Status status = absl::OkStatus();
  std::vector<std::thread> bundle_thread;
  bundle_thread.reserve(simulation_index_chunks.size());
  std::vector<absl::Status> bundle_status;
  bundle_status.reserve(simulation_index_chunks.size());

  for (int i = 0; i < simulation_index_chunks.size(); ++i) {
    bundle_thread.push_back(
        std::thread([i, output_path, &bundle_status, &simulation_index_chunks,
                     &parameter_names, &parameter_value_cartesian_product,
                     &sim_path, &network_config_path]() {
          bundle_status[i] = WriteSimulationConfigWorker(
              output_path, parameter_names, parameter_value_cartesian_product,
              simulation_index_chunks[i], sim_path, network_config_path);
        }));
  }

  for (auto& worker : bundle_thread) {
    if (worker.joinable()) {
      worker.join();
    }
  }
  for (const auto& worker_status : bundle_status) {
    if (!worker_status.ok()) {
      status = worker_status;
    }
  }

  return status;
}

absl::Status ExtractSimulationSweepParameters(
    const SimulationSweepConfig& simulation_sweep_config,
    std::vector<std::string>* parameter_names,
    std::vector<std::vector<SweepParameterValue>>* parameter_values) {
  absl::btree_map<std::string, std::vector<SweepParameterValue>>
      parameter_name_to_values;
  for (const auto& parameter :
       simulation_sweep_config.simulation_config_parameters()) {
    parameter_name_to_values[parameter.name()].push_back(
        SweepParameterValue(parameter.value(), parameter.is_submessage()));
  }
  for (const auto& entry : parameter_name_to_values) {
    parameter_names->push_back(entry.first);
    parameter_values->push_back(entry.second);
  }

  return absl::OkStatus();
}

}  // namespace isekai
