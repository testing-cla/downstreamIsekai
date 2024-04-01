#include <cstdint>
#include <memory>
#include <utility>

#include "absl/log/check.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "isekai/common/common_util.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/simple_environment.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_testing_helpers.h"
#include "isekai/host/falcon/gen3/falcon_model.h"
#include "isekai/host/falcon/gen3/falcon_utils.h"
#include "isekai/host/falcon/gen3/rate_update_engine.h"
#include "isekai/host/falcon/rue/format.h"
#include "isekai/host/falcon/rue/format_bna.h"

namespace isekai {

// This is a peer class that is defined to allow access to private functions
// and variables in Gen3RateUpdateAdapter for testing purposes.
class Gen3RateUpdateAdapterTestPeer {
 public:
  void Set(Gen3RateUpdateEngine* rue) {
    rue_ = rue;
    auto rue_adapter = std::make_unique<
        MockRueAdapter<falcon_rue::Event_BNA, falcon_rue::Response_BNA>>();
    rue_adapter_ = rue_adapter.get();
    rue_->rue_adapter_ = std::move(rue_adapter);
  }
  MockRueAdapter<falcon_rue::Event_BNA, falcon_rue::Response_BNA>&
  mock_rue_adapter() {
    return *rue_adapter_;
  }

 private:
  Gen3RateUpdateEngine* rue_;
  MockRueAdapter<falcon_rue::Event_BNA, falcon_rue::Response_BNA>* rue_adapter_;
};

namespace {

constexpr uint64_t kEventQueueLimit = 100;

}  // namespace
}  // namespace isekai
