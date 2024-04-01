#include "isekai/host/falcon/gen3/connection_scheduler.h"

#include <cstdint>
#include <memory>

#include "absl/container/flat_hash_map.h"
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/simple_environment.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_model.h"
#include "isekai/host/falcon/falcon_testing_helpers.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/gen3/falcon_model.h"
#include "isekai/host/falcon/gen3/falcon_utils.h"
#include "isekai/host/falcon/rue/bits.h"
#include "isekai/host/falcon/rue/fixed.h"
#include "isekai/host/rnic/connection_manager.h"

namespace isekai {
namespace {
constexpr int kFalconVersion = 3;
using ::testing::InSequence;
using ::testing::NiceMock;
}  // namespace
}  // namespace isekai
