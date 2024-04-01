#include "isekai/host/falcon/falcon_rate_update_engine_adapter.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/status/statusor.h"
#include "absl/strings/substitute.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/rue/format.h"

namespace isekai {}  // namespace isekai
