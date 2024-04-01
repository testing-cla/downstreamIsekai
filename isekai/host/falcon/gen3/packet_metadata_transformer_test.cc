#include <cstdint>
#include <memory>
#include <vector>

#include "absl/log/check.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_testing_helpers.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/gen3/falcon_utils.h"

namespace isekai {
namespace {

using ::testing::_;
using ::testing::InSequence;

constexpr uint32_t kFalconVersion3 = 3;

}  // namespace
}  // namespace isekai
