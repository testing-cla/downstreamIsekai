#include "isekai/tools/simulation_kit/common/config_generation_util.h"

#include <string>

#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "internal/testing.h"
#include "isekai/common/status_util.h"

namespace isekai {

namespace {
// Text proto for SimulationSweepConfig for test.
constexpr char kTestSimulationSweepParameterDifference[] =
    R"(simulation index 0 : {
  traffic_pattern.uniform_random.traffic_characteristics.offered_load : 1
}

simulation index 1 : {
  traffic_pattern.uniform_random.traffic_characteristics.offered_load : 2
})";

// Test host config.
constexpr char kTestHostConfig[] =
    R"pb(traffic_pattern {
           uniform_random {
             traffic_characteristics {
               conn_type: ORDERED_RC
               conn_config {
                 op_code: READ
                 msg_size_distribution: UNIFORM_SIZE_DIST
                 size_distribution_params {
                   mean: 1000
                   stddev: 96
                   min: 512
                   max: 1536
                 }
                 arrival_time_distribution: UNIFORM_TIME_DIST
                 offered_load_ratio: 1
               }
               offered_load: 1
               sharing_cid: false
               inline_messasge: false
             }
           }
         }
         network_generator_config {
           nib_snapshot: "path_to_nib_snapshot"
           select_tor_port_evenly: true
           random_seed: 1
           num_hosts: 20
           sim_time_limit_ms: 1
         })pb";

// Text proto for SimulationSweepConfig for test.
constexpr char kTestSimulationSweepConfigTextProto[] =
    R"(simulation_sweep_name: "test_simulations"
processed_result_output_path: "path_to_processed_result_output_directory"
config_output_path: "path_to_config_output_directory"
simulation_result_output_path: "path_to_simulation_result_output_directory"
borg_user: "test_borg_user_name"
borg_cell: "test_borg_cell"
link_speed_gbps: 100
simulation_config_parameters {
  name: "traffic_pattern.uniform_random.traffic_characteristics.conn_type"
  value: "ORDERED_RC"
}
simulation_config_parameters {
  name:
  "traffic_pattern.uniform_random.traffic_characteristics.conn_config.op_code"
  value: "READ"
}
simulation_config_parameters {
  name:
  "traffic_pattern.uniform_random.traffic_characteristics.conn_config.msg_size_distribution"
  value: "UNIFORM_SIZE_DIST"
}
simulation_config_parameters {
  name:
  "traffic_pattern.uniform_random.traffic_characteristics.conn_config.size_distribution_params.mean"
  value: "1000"
}
simulation_config_parameters {
  name:
  "traffic_pattern.uniform_random.traffic_characteristics.conn_config.size_distribution_params.stddev"
  value: "96"
}
simulation_config_parameters {
  name:
  "traffic_pattern.uniform_random.traffic_characteristics.conn_config.size_distribution_params.min"
  value: "512"
}
simulation_config_parameters {
  name:
  "traffic_pattern.uniform_random.traffic_characteristics.conn_config.size_distribution_params.max"
  value: "1536"
}
simulation_config_parameters {
  name:
  "traffic_pattern.uniform_random.traffic_characteristics.conn_config.arrival_time_distribution"
  value: "UNIFORM_TIME_DIST"
}
simulation_config_parameters {
  name:
  "traffic_pattern.uniform_random.traffic_characteristics.conn_config.offered_load_ratio"
  value: "1"
}
simulation_config_parameters {
  name: "traffic_pattern.uniform_random.traffic_characteristics.offered_load"
  value: "1"
}
simulation_config_parameters {
  name: "traffic_pattern.uniform_random.traffic_characteristics.offered_load"
  value: "2"
}
simulation_config_parameters {
  name: "traffic_pattern.uniform_random.traffic_characteristics.sharing_cid"
  value: "false"
}
simulation_config_parameters {
  name:
  "traffic_pattern.uniform_random.traffic_characteristics.inline_messasge"
  value: "false"
}
simulation_config_parameters {
  name: "network_generator_config.nib_snapshot"
  value: "\"path_to_nib_snapshot\""
}
simulation_config_parameters {
  name: "network_generator_config.select_tor_port_evenly"
  value: "true"
}
simulation_config_parameters {
  name: "network_generator_config.random_seed"
  value: "1"
}
simulation_config_parameters {
  name: "network_generator_config.num_hosts"
  value: "20"
}
simulation_config_parameters {
  name: "network_generator_config.sim_time_limit_ms"
  value: "1"
}
)";

template <typename T>
std::vector<T> CombineSplitVector(
    const std::vector<std::vector<T>>& split_vector) {
  std::vector<T> combined_vector;
  for (const auto& vector : split_vector) {
    for (const auto& item : vector) {
      combined_vector.push_back(item);
    }
  }
  return combined_vector;
}

// Test for function RandomShuffleVector.
TEST(RandomShuffleVectorTest, RandomShuffleVector) {
  const int test_size = 10000;

  // Tests for int vector.
  std::vector<int> vector_int_1;
  std::vector<int> vector_int_2 = vector_int_1;
  RandomShuffleVector(&vector_int_2);
  // Tests for empty int vector.
  EXPECT_EQ(vector_int_2, vector_int_1);
  vector_int_1.reserve(test_size);
  for (int i = 0; i < test_size; i++) {
    vector_int_1.push_back(i);
  }
  vector_int_2 = vector_int_1;
  // Tests for non empty int vector.
  RandomShuffleVector(&vector_int_2);
  EXPECT_NE(vector_int_2, vector_int_1);
  std::sort(vector_int_2.begin(), vector_int_2.end());
  EXPECT_EQ(vector_int_2, vector_int_1);

  // Tests for double vector.
  std::vector<double> vector_double_1;
  std::vector<double> vector_double_2 = vector_double_1;
  RandomShuffleVector(&vector_double_2);
  // Tests for empty double vector.
  EXPECT_EQ(vector_double_2, vector_double_1);
  vector_double_1.reserve(test_size);
  for (int i = 0; i < test_size; i++) {
    vector_double_1.push_back(i);
  }
  vector_double_2 = vector_double_1;
  // Tests for non empty double vector.
  RandomShuffleVector(&vector_double_2);
  EXPECT_NE(vector_double_2, vector_double_1);
  std::sort(vector_double_2.begin(), vector_double_2.end());
  EXPECT_EQ(vector_double_2, vector_double_1);

  // Tests for std::string vector.
  std::vector<std::string> vector_str_1;
  std::vector<std::string> vector_str_2 = vector_str_1;
  RandomShuffleVector(&vector_str_2);
  // Tests for empty std::string vector.
  EXPECT_EQ(vector_str_2, vector_str_1);
  vector_str_1.reserve(test_size);
  for (int i = 0; i < test_size; i++) {
    vector_str_1.push_back(absl::StrCat(i));
  }
  std::sort(vector_str_1.begin(), vector_str_1.end());
  vector_str_2 = vector_str_1;
  // Tests for non empty std::string vector.
  RandomShuffleVector(&vector_str_2);
  EXPECT_NE(vector_str_2, vector_str_1);
  std::sort(vector_str_2.begin(), vector_str_2.end());
  EXPECT_EQ(vector_str_2, vector_str_1);
}

// Test for function GenerateShuffledIndexes.
TEST(GenerateShuffledIndexesTest, GenerateShuffledIndexes) {
  const int test_size = 10000;
  std::vector<int> vector_1;

  // Tests for size zero.
  std::vector<int> vector_2 = GenerateShuffledIndexes(0);
  EXPECT_EQ(vector_2, vector_1);

  // Tests for size test_size.
  vector_1.reserve(test_size);
  for (int i = 0; i < test_size; i++) {
    vector_1.push_back(i);
  }
  vector_2 = GenerateShuffledIndexes(test_size);
  EXPECT_NE(vector_2, vector_1);
  std::sort(vector_2.begin(), vector_2.end());
  EXPECT_EQ(vector_2, vector_1);
}

// Test for function SplitVectorToChunks.
TEST(SplitVectorToChunksTest, SplitVectorToChunks) {
  const int test_size = 10000;

  std::vector<int> vector;
  // Tests for empty vector input.
  ASSERT_OK_THEN_ASSIGN(std::vector<std::vector<int>> split_vector,
                        SplitVectorToChunks(vector, test_size));
  EXPECT_EQ(split_vector.size(), 0);

  // Tests for non-empty vector input.
  vector.reserve(test_size);
  for (int i = 0; i < test_size; i++) {
    vector.push_back(i);
  }
  // Tests for chunk_length 0.
  ASSERT_DEATH(
      { CHECK_OK(SplitVectorToChunks(vector, 0).status()); },
      "chunk_length should not be less than 1");

  // Tests for different chunk_length.
  // chunk_length = 1.
  ASSERT_OK_THEN_ASSIGN(split_vector, SplitVectorToChunks(vector, 1));
  EXPECT_EQ(split_vector.size(),
            static_cast<int>(std::ceil(static_cast<double>(test_size) /
                                       std::min(test_size, 1))));
  std::vector<int> combined_vector = CombineSplitVector(split_vector);
  std::sort(combined_vector.begin(), combined_vector.end());
  EXPECT_EQ(combined_vector, vector);

  // chunk_length = 3.
  ASSERT_OK_THEN_ASSIGN(split_vector, SplitVectorToChunks(vector, 3));
  EXPECT_EQ(split_vector.size(),
            static_cast<int>(std::ceil(static_cast<double>(test_size) /
                                       std::min(test_size, 3))));
  combined_vector = CombineSplitVector(split_vector);
  std::sort(combined_vector.begin(), combined_vector.end());
  EXPECT_EQ(combined_vector, vector);

  // chunk_length = 5.
  ASSERT_OK_THEN_ASSIGN(split_vector, SplitVectorToChunks(vector, 5));
  EXPECT_EQ(split_vector.size(),
            static_cast<int>(std::ceil(static_cast<double>(test_size) /
                                       std::min(test_size, 5))));
  combined_vector = CombineSplitVector(split_vector);
  std::sort(combined_vector.begin(), combined_vector.end());
  EXPECT_EQ(combined_vector, vector);

  // chunk_length = 20000.
  ASSERT_OK_THEN_ASSIGN(split_vector, SplitVectorToChunks(vector, 20000));
  EXPECT_EQ(split_vector.size(),
            static_cast<int>(std::ceil(static_cast<double>(test_size) /
                                       std::min(test_size, 20000))));
  combined_vector = CombineSplitVector(split_vector);
  std::sort(combined_vector.begin(), combined_vector.end());
  EXPECT_EQ(combined_vector, vector);
}

// Test for class CartesianProductGenerator.
TEST(CartesianProductGeneratorTest, CartesianProductGenerator) {
  std::vector<std::vector<std::string>> value_space;

  // Empty value space.
  ASSERT_DEATH(
      {
        CHECK_OK(CartesianProductGenerator<std::string>::Create(value_space)
                     .status());
      },
      "parameter_value_space should be not empty");

  // Value space with one empty parameter.
  value_space = {{"1", "2"}, {}, {"4", "5"}};
  ASSERT_DEATH(
      {
        CHECK_OK(CartesianProductGenerator<std::string>::Create(value_space)
                     .status());
      },
      "The values for each parameter should be not empty");

  // Small value space.
  value_space = {{"1"}};
  ASSERT_OK_THEN_ASSIGN(
      auto cartesian_product_generator1,
      CartesianProductGenerator<std::string>::Create(value_space));
  const std::vector<std::vector<std::string>> expected_cartesian_product1{
      {"1"}};
  EXPECT_EQ(cartesian_product_generator1->GetCartesianProduct(),
            expected_cartesian_product1);

  value_space = {{"1", "2"}, {"3"}, {"4", "5"}};
  ASSERT_OK_THEN_ASSIGN(
      auto cartesian_product_generator2,
      CartesianProductGenerator<std::string>::Create(value_space));
  const std::vector<std::vector<std::string>> expected_cartesian_product2{
      {"1", "3", "4"}, {"1", "3", "5"}, {"2", "3", "4"}, {"2", "3", "5"}};
  EXPECT_EQ(cartesian_product_generator2->GetCartesianProduct(),
            expected_cartesian_product2);

  // Large value space (6 parameters, each of them has 10 values. 1000000 lines
  // in the Cartesian product).
  value_space.clear();
  for (int i = 0; i < 6; i++) {
    std::vector<std::string> parameter_values(10);
    for (int j = 0; j < 10; j++) {
      parameter_values[j] = absl::StrCat(j);
    }
    value_space.push_back(parameter_values);
  }

  ASSERT_OK_THEN_ASSIGN(
      auto cartesian_product_generator3,
      CartesianProductGenerator<std::string>::Create(value_space));
  const std::vector<std::vector<std::string>>& cartesian_product3 =
      cartesian_product_generator3->GetCartesianProduct();
  EXPECT_EQ(cartesian_product3.size(),
            /* 10 ^ 6 = */ 1000000);
  const std::vector<std::string> expected_first_combination{"0", "0", "0",
                                                            "0", "0", "0"};
  EXPECT_EQ(cartesian_product3.front(), expected_first_combination);
  const std::vector<std::string> expected_last_combination{"9", "9", "9",
                                                           "9", "9", "9"};
  EXPECT_EQ(cartesian_product3.back(), expected_last_combination);
}

TEST(ConstructSimulationConfigTest, ConstructSimulationConfig) {
  SimulationSweepConfig simulation_sweep_config;
  google::protobuf::TextFormat::ParseFromString(
      kTestSimulationSweepConfigTextProto, &simulation_sweep_config);
  absl::flat_hash_map<std::string, SweepParameterValue> parameter_name_to_value;
  parameter_name_to_value.reserve(
      simulation_sweep_config.simulation_config_parameters_size());
  for (const auto& parameter :
       simulation_sweep_config.simulation_config_parameters()) {
    parameter_name_to_value.insert(std::make_pair(
        parameter.name(),
        SweepParameterValue(parameter.value(), parameter.is_submessage())));
  }

  ASSERT_OK_THEN_ASSIGN(const auto simulation_config,
                        ConstructSimulationConfig(parameter_name_to_value));
  isekai::SimulationConfig expected_simulation_config;
  google::protobuf::TextFormat::ParseFromString(kTestHostConfig,
                                                &expected_simulation_config);
  EXPECT_EQ(simulation_config.DebugString(),
            expected_simulation_config.DebugString());
}

// Test for function GenerateSimulationParameterDifference.
TEST(GenerateSimulationParameterDifferenceTest,
     GenerateSimulationParameterDifference) {
  SimulationSweepConfig simulation_sweep_config;
  google::protobuf::TextFormat::ParseFromString(
      kTestSimulationSweepConfigTextProto, &simulation_sweep_config);
  std::vector<std::string> parameter_names;
  std::vector<std::vector<SweepParameterValue>> parameter_values;
  EXPECT_OK(ExtractSimulationSweepParameters(
      simulation_sweep_config, &parameter_names, &parameter_values));
  ASSERT_OK_THEN_ASSIGN(
      auto cartesian_product_generator,
      CartesianProductGenerator<SweepParameterValue>::Create(parameter_values));

  EXPECT_EQ(GenerateSimulationParameterDifference(
                parameter_names, parameter_values,
                cartesian_product_generator->GetCartesianProduct()),
            kTestSimulationSweepParameterDifference);
}

}  // namespace

}  // namespace isekai
