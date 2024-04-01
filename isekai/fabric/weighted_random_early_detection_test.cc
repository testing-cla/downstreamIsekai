#include "isekai/fabric/weighted_random_early_detection.h"

#include <fstream>
#include <random>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"
#include "gtest/gtest.h"
#include "isekai/fabric/memory_management_config.pb.h"

namespace isekai {
namespace {

constexpr char kWredConfigDataDir[] = "isekai/test_data/";

template <typename T>
void LoadTextProto(absl::string_view proto_file, T* proto) {
  std::ifstream ifs(absl::StrCat(kWredConfigDataDir, proto_file));
  google::protobuf::io::IstreamInputStream iis(&ifs);
  CHECK(google::protobuf::TextFormat::Parse(&iis, proto))
      << "fail to parse proto text.";
}

TEST(PacketFilterTest, WredTest) {
  MemoryManagementUnitConfig mmu_config;
  LoadTextProto("memory_management_config.pb.txt", &mmu_config);
  auto wred_0 = WeightedRandomEarlyDetection(
      /* configuration = */ mmu_config.buffer_carving_config()
          .port_configs(0)
          .qos_config()
          .queue_configs(0)
          .cos_config()
          .wred_config(),
      /* seed = */ 0);

  // The min_avg_queue_size and max_avg_queue_size are both 10000, and the
  // weight is 0.002.
  EXPECT_EQ(wred_0.PerformWred(1000), WredResult::kEnqueue);
  EXPECT_EQ(wred_0.PerformWred(10000 / 0.002), WredResult::kEcnMark);
}

}  // namespace
}  // namespace isekai
