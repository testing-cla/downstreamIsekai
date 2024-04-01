#include "isekai/host/falcon/gen3/falcon_model.h"

#include "absl/log/check.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/gen3/falcon_utils.h"

namespace isekai {

Gen3FalconModel::Gen3FalconModel(const FalconConfig& configuration,
                                 Environment* env,
                                 StatisticCollectionInterface* stats_collector,
                                 ConnectionManagerInterface* connection_manager,
                                 std::string_view host_id,
                                 uint8_t number_of_hosts)
    : Gen2FalconModel(configuration, env, stats_collector, connection_manager,
                      host_id, number_of_hosts) {}

void Gen3FalconModel::AckTransaction(uint32_t scid, uint32_t rsn,
                                     Packet::Syndrome ack_code,
                                     absl::Duration rnr_timeout,
                                     std::unique_ptr<OpaqueCookie> cookie) {
  Gen2FalconModel::AckTransaction(scid, rsn, ack_code, rnr_timeout,
                                  std::move(cookie));
}

std::unique_ptr<OpaqueCookie> Gen3FalconModel::CreateCookie(
    const Packet& packet) {
  return std::make_unique<Gen3OpaqueCookie>();
}

// Sets the right connection type for a connection to be established based on
// FalconConfig flags and Falcon version.
void Gen3FalconModel::SetConnectionType(
    ConnectionState::ConnectionMetadata& metadata) {
  metadata.connection_type =
      ConnectionState::ConnectionMetadata::ConnectionStateType::Gen3;
}

}  // namespace isekai
