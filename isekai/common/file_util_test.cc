#include "isekai/common/file_util.h"

#include <fstream>
#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "internal/testing.h"
#include "isekai/common/config.pb.h"

namespace {

TEST(FileUtilTest, ReadTextProto) {
  isekai::SimulationConfig simulation_config;
  EXPECT_OK(ReadTextProtoFromFile("isekai/test_data/config.pb.txt",
                                  &simulation_config));

  EXPECT_EQ(simulation_config.network().hosts_size(), 2);
  EXPECT_EQ(simulation_config.network().hosts(0).id(), "host1");
  EXPECT_EQ(simulation_config.network().hosts(1).id(), "host2");
}

TEST(FileUtilTest, JoinTwoPaths) {
  const std::string joined_path = "abc/def";
  EXPECT_EQ(joined_path, isekai::FileJoinPath("abc", "def"));
  EXPECT_EQ(joined_path, isekai::FileJoinPath("abc/", "def"));
  EXPECT_EQ(joined_path, isekai::FileJoinPath("abc", "/def"));
  EXPECT_EQ(joined_path, isekai::FileJoinPath("abc/", "/def"));
}

TEST(FileUtilTest, WriteFile) {
  std::string file_content = "write file test string";
  std::string file_path =
      isekai::FileJoinPath(::testing::TempDir(), "test/write_test.txt");
  EXPECT_OK(isekai::WriteStringToFile(file_path, file_content));

  std::ifstream ifs(file_path);
  std::ostringstream sstr;
  sstr << ifs.rdbuf();
  EXPECT_EQ(file_content, sstr.str());
}

}  // namespace
