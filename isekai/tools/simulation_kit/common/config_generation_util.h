#ifndef ISEKAI_TOOLS_SIMULATION_KIT_COMMON_CONFIG_GENERATION_UTIL_H_
#define ISEKAI_TOOLS_SIMULATION_KIT_COMMON_CONFIG_GENERATION_UTIL_H_

#include <optional>
#include <random>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/file_util.h"
#include "isekai/common/status_util.h"
#include "isekai/tools/simulation_kit/common/simulation_kit.pb.h"
#include "isekai/tools/simulation_kit/isekai_k8_config_generation/isekai_paths_storage.pb.h"

namespace isekai {

// Tuple defined struct for value of sweep parammeter for Simulation Kit.
struct SweepParameterValue {
  std::string value;
  bool is_submessage;
  inline bool operator==(const SweepParameterValue& a) const {
    return a.value == value && a.is_submessage == is_submessage;
  }
  SweepParameterValue() {}
  SweepParameterValue(const std::string& value, bool is_submessage)
      : value(value), is_submessage(is_submessage) {}
  explicit SweepParameterValue(const std::string& value)
      : value(value), is_submessage(false) {}
};

// Class for generating the Cartesian product for parameters. Please use
// ASSIGN_OR_RETURN(auto cartesian_product_generator,
// CartesianProductGenerator::Create(parameter_value_space)); to get the
// std::unique_ptr to the instance of CartesianProductGenerator.
// GetsCartesianProduct() returns the const reference to the Cartesian
// product.
template <typename T>
class CartesianProductGenerator {
 public:
  // Creates and returns CartesianProductGenerator.
  static absl::StatusOr<std::unique_ptr<CartesianProductGenerator>> Create(
      const std::vector<std::vector<T>>& parameter_value_space);

  // Gets the Cartesian product.
  const std::vector<std::vector<T>>& GetCartesianProduct() const;

 private:
  explicit CartesianProductGenerator(
      const std::vector<std::vector<T>>& cartesian_product);

  // Generates the Cartesian product from parameter_value_space.
  static absl::StatusOr<std::vector<std::vector<T>>> GenerateCartesianProduct(
      const std::vector<std::vector<T>>& parameter_value_space,
      const int cartesian_product_size);

  // Cartesian Product.
  const std::vector<std::vector<T>> cartesian_product_;
};

template <typename T>
absl::StatusOr<std::unique_ptr<CartesianProductGenerator<T>>>
CartesianProductGenerator<T>::Create(
    const std::vector<std::vector<T>>& parameter_value_space) {
  if (parameter_value_space.empty()) {
    return absl::InternalError("parameter_value_space should be not empty.");
  }

  int cartesian_product_size = 1;
  for (const auto& values : parameter_value_space) {
    if (values.empty()) {
      return absl::InternalError(
          "The values for each parameter should be not empty.");
    }
    cartesian_product_size *= values.size();
  }

  ASSIGN_OR_RETURN(
      std::vector<std::vector<T>> cartesian_product,
      GenerateCartesianProduct(parameter_value_space, cartesian_product_size));

  return absl::WrapUnique(new CartesianProductGenerator(cartesian_product));
}

template <typename T>
const std::vector<std::vector<T>>&
CartesianProductGenerator<T>::GetCartesianProduct() const {
  return cartesian_product_;
}

template <typename T>
CartesianProductGenerator<T>::CartesianProductGenerator(
    const std::vector<std::vector<T>>& cartesian_product)
    : cartesian_product_(cartesian_product) {}

template <typename T>
absl::StatusOr<std::vector<std::vector<T>>>
CartesianProductGenerator<T>::GenerateCartesianProduct(
    const std::vector<std::vector<T>>& parameter_value_space,
    const int cartesian_product_size) {
  std::vector<std::vector<T>> cartesian_product(cartesian_product_size);
  const int value_space_dimension = parameter_value_space.size();
  // Indexes for values in each dimensions for current_combination.
  std::vector<int> current_combination_indexes(value_space_dimension, 0);

  // Generates the Cartesian product as traversing all values of a number with
  // variable weights for each of its digits. The weight for a digit is the
  // number of values in according dimension in parameter_value_space.

  // For example, if parameter_value_space = {{"10", "20"}, {"a", "b", "c"}},
  // then it generates the Cartesian product when traversing all values of a
  // number with weights 2 and 3 for its digits (two values in {"10", "20"},
  // three values in {"a", "b", "c"}).

  // The following loop traverses all the values for the two digits number and
  // adds corresponding combination to the cartesian_product. For example, if
  // parameter_value_space = {{"10", "20"}, {"a", "b", "c"}}, then:

  // Initial number: (0, 0), corresponding combination: ("10", "a").
  // 2nd number: (0, 1), corresponding combination: ("10", "b").
  // 3rd number: (0, 2), corresponding combination: ("10", "c").
  // 4th number: (1, 0), corresponding combination: ("20", "a").
  // 5th number: (1, 1), corresponding combination: ("20", "b").
  // 6th number: (1, 2), corresponding combination: ("20", "c").

  for (int combination_idx = 0; combination_idx < cartesian_product_size;
       combination_idx++) {
    // Generates current combination from indexes.
    std::vector<T> current_combination(value_space_dimension);

    for (int dimension_idx = 0; dimension_idx < value_space_dimension;
         dimension_idx++) {
      const int value_idx = current_combination_indexes[dimension_idx];
      if (value_idx > parameter_value_space[dimension_idx].size()) {
        return absl::InternalError(absl::StrCat(
            "value_idx is beyond the number of values in dimension ",
            dimension_idx, " in value space."));
      }
      current_combination[dimension_idx] =
          parameter_value_space[dimension_idx][value_idx];
    }

    // Stores current combination.
    cartesian_product[combination_idx] = current_combination;

    // Calculates indexes for next combination.
    int carry = 1;
    for (int dimension_idx = value_space_dimension - 1; dimension_idx >= 0;
         dimension_idx--) {
      const int current_index =
          current_combination_indexes[dimension_idx] + carry;
      const int modulus = parameter_value_space[dimension_idx].size();
      if (current_index < modulus) {
        current_combination_indexes[dimension_idx] = current_index;
        break;
      } else {
        current_combination_indexes[dimension_idx] = current_index % modulus;
        carry = current_index / modulus;
      }
    }
  }

  return cartesian_product;
}

// Deletes and creates directory.
absl::Status RecreateDir(absl::string_view file_path);

// Constructs isekai::SimulationConfig from the parameter name to value map.
absl::StatusOr<isekai::SimulationConfig> ConstructSimulationConfig(
    const absl::flat_hash_map<std::string, SweepParameterValue>&
        parameter_name_to_value);

// Generates randomly shuffled indexes from 0 to n - 1.
std::vector<int> GenerateShuffledIndexes(int n);

// Generates the difference of the parameters in the simulations in the sweep.
std::string GenerateSimulationParameterDifference(
    const std::vector<std::string>& parameter_names,
    const std::vector<std::vector<SweepParameterValue>>& parameter_values,
    const std::vector<std::vector<SweepParameterValue>>& cartesian_product);

// Writes isekai::SimulationConfig to files. These files are consumed by Isekai
// Network Generator. It invokes WriteSimulationConfigWorker().
absl::Status WriteSimulationConfig(
    absl::string_view output_path,
    const std::vector<std::string>& parameter_names,
    const std::vector<std::vector<SweepParameterValue>>&
        parameter_value_cartesian_product,
    std::optional<std::vector<std::string>*> sim_path = std::nullopt,
    const std::optional<std::string> network_config_path = std::nullopt);

absl::Status RecreateDir(absl::string_view file_path);

// Randomly shuffles vector.
template <typename T>
void RandomShuffleVector(std::vector<T>* vector) {
  const unsigned seed =
      std::chrono::system_clock::now().time_since_epoch().count();
  std::shuffle(vector->begin(), vector->end(),
               std::default_random_engine(seed));
}

// Extracts simulation sweep parameters.
absl::Status ExtractSimulationSweepParameters(
    const SimulationSweepConfig& simulation_sweep_config,
    std::vector<std::string>* parameter_names,
    std::vector<std::vector<SweepParameterValue>>* parameter_values);

// Splits vector to chunks with chunk_length.
template <typename T>
absl::StatusOr<std::vector<std::vector<T>>> SplitVectorToChunks(
    const std::vector<T>& vector, size_t chunk_length) {
  if (chunk_length < 1)
    return absl::InvalidArgumentError(
        "chunk_length should not be less than 1.");
  std::vector<std::vector<T>> split_vector;
  const size_t vector_length = vector.size();
  auto end_iter = vector.begin();

  for (size_t increment = 0, end_pos = 0; end_pos < vector_length;
       end_pos += increment) {
    const auto start_iter = end_iter;
    increment = std::min(chunk_length, vector_length - end_pos);
    std::advance(end_iter, increment);
    split_vector.push_back(std::vector<T>(start_iter, end_iter));
  }

  return split_vector;
}

}  // namespace isekai

#endif  // ISEKAI_TOOLS_SIMULATION_KIT_COMMON_CONFIG_GENERATION_UTIL_H_
