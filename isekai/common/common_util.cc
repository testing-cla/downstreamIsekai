#include "isekai/common/common_util.h"

#include <string>

#include "absl/strings/string_view.h"
#include "glog/logging.h"
#include "isekai/common/config.pb.h"

namespace isekai {

HostConfigProfile GetHostConfigProfile(absl::string_view host_id,
                                       const NetworkConfig& network) {
  for (auto const& host_config : network.hosts()) {
    if (host_id == host_config.id()) {
      switch (host_config.config_case()) {
        case HostConfig::ConfigCase::kHostConfig: {
          return host_config.host_config();
        }
        case HostConfig::ConfigCase::kHostConfigProfileName: {
          // Gets the host config according to the profile name
          for (const auto& profile : network.host_configs()) {
            if (profile.profile_name() ==
                host_config.host_config_profile_name()) {
              return profile;
            }
          }
          LOG(FATAL) << "no profile is found: "
                     << host_config.host_config_profile_name();
        }
        case HostConfig::ConfigCase::CONFIG_NOT_SET: {
          LOG(INFO) << "host: " << host_id << " uses the default config.";
          return {};
        }
      }
    }
  }
  LOG(FATAL) << "host does not exist: " << host_id;
}

}  // namespace isekai
