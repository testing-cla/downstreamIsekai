#include "isekai/host/falcon/falcon_resource_credits.h"

#include "isekai/common/config.pb.h"

namespace isekai {

FalconResourceCredits FalconResourceCredits::Create(
    const FalconConfig::ResourceCredits& config) {
  FalconResourceCredits credits = {
      .tx_packet_credits =
          {
              .ulp_requests = config.tx_packet_credits().ulp_requests(),
              .ulp_data = config.tx_packet_credits().ulp_data(),
              .network_requests = config.tx_packet_credits().network_requests(),
              .max_ulp_requests = config.tx_packet_credits().ulp_requests(),
          },
      .tx_buffer_credits =
          {
              .ulp_requests = config.tx_buffer_credits().ulp_requests(),
              .ulp_data = config.tx_buffer_credits().ulp_data(),
              .network_requests = config.tx_buffer_credits().network_requests(),
              .max_ulp_requests = config.tx_buffer_credits().ulp_requests(),
          },
      .rx_packet_credits =
          {
              .ulp_requests = config.rx_packet_credits().ulp_requests(),
              .network_requests = config.rx_packet_credits().network_requests(),
          },
      .rx_buffer_credits =
          {
              .ulp_requests = config.rx_buffer_credits().ulp_requests(),
              .network_requests = config.rx_buffer_credits().network_requests(),
          },
      .enable_ulp_pool_oversubscription =
          config.has_enable_ulp_pool_oversubscription()
              ? config.enable_ulp_pool_oversubscription()
              : false,
  };

  return credits;
}

bool FalconResourceCredits::IsInitialized() const {
  return tx_packet_credits.ulp_requests != 0 ||
         tx_packet_credits.ulp_data != 0 ||
         tx_packet_credits.network_requests ||
         tx_buffer_credits.ulp_requests != 0 ||
         tx_buffer_credits.ulp_data != 0 ||
         tx_buffer_credits.network_requests != 0 ||
         rx_packet_credits.ulp_requests != 0 ||
         rx_packet_credits.network_requests != 0 ||
         rx_buffer_credits.ulp_requests != 0 ||
         rx_buffer_credits.network_requests != 0;
}

bool FalconResourceCredits::operator<=(const FalconResourceCredits& rhs) const {
  // Check whether all resources except ulp_data are within the limit of rhs.
  bool are_non_oversubscribed_resources_within_limit =
      tx_packet_credits.ulp_requests <= rhs.tx_packet_credits.ulp_requests &&
      tx_packet_credits.network_requests <=
          rhs.tx_packet_credits.network_requests &&
      tx_buffer_credits.ulp_requests <= rhs.tx_buffer_credits.ulp_requests &&
      tx_buffer_credits.network_requests <=
          rhs.tx_buffer_credits.network_requests &&
      rx_packet_credits.ulp_requests <= rhs.rx_packet_credits.ulp_requests &&
      rx_packet_credits.network_requests <=
          rhs.rx_packet_credits.network_requests &&
      rx_buffer_credits.ulp_requests <= rhs.rx_buffer_credits.ulp_requests &&
      rx_buffer_credits.network_requests <=
          rhs.rx_buffer_credits.network_requests;

  bool are_oversubscribed_resources_within_limit;
  if (enable_ulp_pool_oversubscription ||
      rhs.enable_ulp_pool_oversubscription) {
    // If ULP pool oversubscription is enabled, the sum of ulp_data and
    // ulp_request should be less than than the sum of ulp_data and ulp_request
    // of rhs.
    are_oversubscribed_resources_within_limit =
        tx_packet_credits.ulp_data + tx_packet_credits.ulp_requests <=
            rhs.tx_packet_credits.ulp_data +
                rhs.tx_packet_credits.ulp_requests &&
        tx_buffer_credits.ulp_data + tx_buffer_credits.ulp_requests <=
            rhs.tx_buffer_credits.ulp_data + rhs.tx_buffer_credits.ulp_requests;
  } else {
    are_oversubscribed_resources_within_limit =
        tx_packet_credits.ulp_data <= rhs.tx_packet_credits.ulp_data &&
        tx_buffer_credits.ulp_data <= rhs.tx_buffer_credits.ulp_data;
  }

  return are_non_oversubscribed_resources_within_limit &&
         are_oversubscribed_resources_within_limit;
}

bool FalconResourceCredits::operator==(const FalconResourceCredits& rhs) const {
  return tx_packet_credits.ulp_requests == rhs.tx_packet_credits.ulp_requests &&
         tx_packet_credits.ulp_data == rhs.tx_packet_credits.ulp_data &&
         tx_packet_credits.network_requests ==
             rhs.tx_packet_credits.network_requests &&
         tx_buffer_credits.ulp_requests == rhs.tx_buffer_credits.ulp_requests &&
         tx_buffer_credits.ulp_data == rhs.tx_buffer_credits.ulp_data &&
         tx_buffer_credits.network_requests ==
             rhs.tx_buffer_credits.network_requests &&
         rx_packet_credits.ulp_requests == rhs.rx_packet_credits.ulp_requests &&
         rx_packet_credits.network_requests ==
             rhs.rx_packet_credits.network_requests &&
         rx_buffer_credits.ulp_requests == rhs.rx_buffer_credits.ulp_requests &&
         rx_buffer_credits.network_requests ==
             rhs.rx_buffer_credits.network_requests;
}

FalconResourceCredits& FalconResourceCredits::operator+=(
    const FalconResourceCredits& rhs) {
  // Add rhs for all resources except ulp_data.
  tx_packet_credits.ulp_requests += rhs.tx_packet_credits.ulp_requests;
  tx_packet_credits.network_requests += rhs.tx_packet_credits.network_requests;
  tx_buffer_credits.ulp_requests += rhs.tx_buffer_credits.ulp_requests;
  tx_buffer_credits.network_requests += rhs.tx_buffer_credits.network_requests;
  rx_packet_credits.ulp_requests += rhs.rx_packet_credits.ulp_requests;
  rx_packet_credits.network_requests += rhs.rx_packet_credits.network_requests;
  rx_buffer_credits.ulp_requests += rhs.rx_buffer_credits.ulp_requests;
  rx_buffer_credits.network_requests += rhs.rx_buffer_credits.network_requests;

  if (enable_ulp_pool_oversubscription) {
    // If ULP pool oversubscription is enabled and ulp_data is zero, it means
    // some ulp_requests might have been consumed by ulp_data. In that case, we
    // replenish ulp_requests upto max_ulp_requests and add the remaining to
    // ulp_data.
    if (tx_packet_credits.ulp_data == 0) {
      int32_t consumed_from_requests = std::min(
          rhs.tx_packet_credits.ulp_data,
          tx_packet_credits.max_ulp_requests - tx_packet_credits.ulp_requests);
      tx_packet_credits.ulp_requests += consumed_from_requests;
      tx_packet_credits.ulp_data +=
          (rhs.tx_packet_credits.ulp_data - consumed_from_requests);
    } else {
      tx_packet_credits.ulp_data += rhs.tx_packet_credits.ulp_data;
    }

    // Same steps as above, but for TX buffer.
    if (tx_buffer_credits.ulp_data == 0) {
      int32_t consumed_from_requests = std::min(
          rhs.tx_buffer_credits.ulp_data,
          tx_buffer_credits.max_ulp_requests - tx_buffer_credits.ulp_requests);
      tx_buffer_credits.ulp_requests += consumed_from_requests;
      tx_buffer_credits.ulp_data +=
          (rhs.tx_buffer_credits.ulp_data - consumed_from_requests);
    } else {
      tx_buffer_credits.ulp_data += rhs.tx_buffer_credits.ulp_data;
    }
  } else {
    tx_packet_credits.ulp_data += rhs.tx_packet_credits.ulp_data;
    tx_buffer_credits.ulp_data += rhs.tx_buffer_credits.ulp_data;
  }
  return *this;
}

FalconResourceCredits& FalconResourceCredits::operator-=(
    const FalconResourceCredits& rhs) {
  // Subtract rhs for all resources except ulp_data.
  tx_packet_credits.ulp_requests -= rhs.tx_packet_credits.ulp_requests;
  tx_packet_credits.network_requests -= rhs.tx_packet_credits.network_requests;
  tx_buffer_credits.ulp_requests -= rhs.tx_buffer_credits.ulp_requests;
  tx_buffer_credits.network_requests -= rhs.tx_buffer_credits.network_requests;
  rx_packet_credits.ulp_requests -= rhs.rx_packet_credits.ulp_requests;
  rx_packet_credits.network_requests -= rhs.rx_packet_credits.network_requests;
  rx_buffer_credits.ulp_requests -= rhs.rx_buffer_credits.ulp_requests;
  rx_buffer_credits.network_requests -= rhs.rx_buffer_credits.network_requests;

  if (enable_ulp_pool_oversubscription) {
    // If TX ULP pool oversubscription is enabled and this.ulp_data does not
    // have enough credits for rhs.ulp_data, we subtract credits from
    // ulp_requests.
    int32_t consume_from_data =
        std::min(tx_packet_credits.ulp_data, rhs.tx_packet_credits.ulp_data);
    int32_t consume_from_requests =
        rhs.tx_packet_credits.ulp_data - consume_from_data;
    tx_packet_credits.ulp_requests -= consume_from_requests;
    tx_packet_credits.ulp_data -= consume_from_data;

    // Same steps as above, but for TX buffer.
    consume_from_data =
        std::min(tx_buffer_credits.ulp_data, rhs.tx_buffer_credits.ulp_data);
    consume_from_requests = rhs.tx_buffer_credits.ulp_data - consume_from_data;
    tx_buffer_credits.ulp_requests -= consume_from_requests;
    tx_buffer_credits.ulp_data -= consume_from_data;
  } else {
    tx_packet_credits.ulp_data -= rhs.tx_packet_credits.ulp_data;
    tx_buffer_credits.ulp_data -= rhs.tx_buffer_credits.ulp_data;
  }
  return *this;
}

}  // namespace isekai
