#ifndef ISEKAI_HOST_FALCON_GEN3_FALCON_MODEL_H_
#define ISEKAI_HOST_FALCON_GEN3_FALCON_MODEL_H_

#include "isekai/host/falcon/gen2/falcon_model.h"

namespace isekai {

class Gen3FalconModel : public Gen2FalconModel {
 public:
  explicit Gen3FalconModel(const FalconConfig& configuration, Environment* env,
                           StatisticCollectionInterface* stats_collector,
                           ConnectionManagerInterface* connection_manager,
                           std::string_view host_id, uint8_t number_of_hosts);

 private:
  void AckTransaction(uint32_t scid, uint32_t rsn, Packet::Syndrome ack_code,
                      absl::Duration rnr_timeout,
                      std::unique_ptr<OpaqueCookie> cookie) override;
  // Creates the cookie that flows from Falcon to the ULP and then back to
  // Falcon in a ULP ACK.
  std::unique_ptr<OpaqueCookie> CreateCookie(const Packet& packet) override;
  // Sets the right connection type for a connection to be established based on
  // FalconConfig flags and Falcon version.
  void SetConnectionType(
      ConnectionState::ConnectionMetadata& metadata) override;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_FALCON_MODEL_H_
