#include "isekai/fabric/port_selection.h"

#include <climits>
#include <cstdint>
#include <memory>

#include "glog/logging.h"
#include "isekai/fabric/packet_util.h"

namespace isekai {
namespace {

constexpr int kS1 = 1;
constexpr int kS2 = 2;
constexpr int kS3 = 3;

uint32_t RotateLeftBits(uint32_t value, unsigned int bit_count) {
  const unsigned int mask = CHAR_BIT * sizeof(value) - 1;
  bit_count &= mask;
  return (value << bit_count) | (value >> (-bit_count & mask));
}

uint32_t HashValueRotation(uint32_t hash_value, int stage, bool uplink) {
  switch (stage) {
    case kS1: {
      return hash_value;
    }
    case kS2: {
      if (uplink) {
        return RotateLeftBits(hash_value, 4);
      } else {
        return RotateLeftBits(hash_value, 16);
      }
    }
    case kS3: {
      if (uplink) {
        return RotateLeftBits(hash_value, 8);
      } else {
        return RotateLeftBits(hash_value, 12);
      }
    }
    default:
      LOG(FATAL) << "Unknown stage: " << stage;
  }
}

}  // namespace

std::unique_ptr<OutputPortSelection>
OutputPortSelectionFactory::GetOutputPortSelectionScheme(
    RouterConfigProfile::PortSelectionPolicy port_selection_scheme) {
  switch (port_selection_scheme) {
    case RouterConfigProfile::WCMP:
      return std::make_unique<WcmpOutputPortSelection>();
    default:
      // // Currently, only WCMP is implemented.
      LOG(FATAL) << "unknown port selection scheme.";
  }
}

uint32_t WcmpOutputPortSelection::SelectOutputPort(
    const TableOutputOptions& output_options, omnetpp::cMessage* packet,
    int stage) {
  uint32_t hash_value = GenerateHashValueForPacket(packet);
  size_t number_of_output_choices = output_options.size();
  uint32_t weight_sum = 0;
  for (size_t i = 0; i < number_of_output_choices; i++) {
    weight_sum += output_options[i].weight;
  }
  if (GetPacketSourceMac(packet) == kFlowSrcMacVrfUp) {
    hash_value = HashValueRotation(hash_value, stage, true);
  } else {
    hash_value = HashValueRotation(hash_value, stage, false);
  }
  uint32_t mod_hash_value = hash_value % weight_sum;

  // Simple example of selecting output port via WCMP:
  //   mod_hash_value = 4.
  //   output_options = {{/* weight */ 2, /* port_index */ 1}, {1, 2}, {4, 3},
  //   {3, 4}}.
  //   Then, selected_port_index = 3.

  // Details:

  // Assumptions:
  //   1. weight > 0.
  //   2. The sum of weights can be hold in uint32.

  // Scheme 1 - original selection steps:
  // std::vector<double> discrete_cdf(/* number of weights + 1 = */
  // output_options.size() + 1, /* value */ 0);
  // std::vector<double> prefix_sum(/* number of weights + 1 = */
  // output_options.size() + 1, /* value */ 0);
  // for (size_t i = 0; i < discrete_cdf.size() - 1; i++) {
  //   prefix_sum[i + 1] = output_options[i].weight + prefix_sum[i];
  // }
  // for (size_t i = 0; i < discrete_cdf.size(); i++) {
  //   discrete_cdf[i] = prefix_sum[i] / static_cast<double>(weight_sum);
  // }
  // The selection for output port is performed via following logic:
  // double position = mod_hash_value / static_cast<double>(weight_sum);
  // Then select port k (k belongs to 0, 1, ..., discrete_cdf.size()-1), if and
  // only if (discrete_cdf[k] <=  position  && discrete_cdf[k+1] > position).

  // Scheme 2 - simplified selection steps (currently used):
  // After reducing the denominators (static_cast<double>(weight_sum)) for
  // discrete_cdf[k], discrete_cdf[k+1], and position, above condition equals to
  // (prefix_sum[k] <=  mod_hash_value  && prefix_sum[k+1] > mod_hash_value).

  // In the following computation,it selects i that makes (prefix_sum[i] <=
  // mod_hash_value  && prefix_sum[i+1] > mod_hash_value) true.

  uint32_t selected_port_index = 0;
  for (size_t i = 0; i < number_of_output_choices; i++) {
    if (mod_hash_value < output_options[i].weight) {
      selected_port_index = output_options[i].port_index;
      break;
    }
    mod_hash_value -= output_options[i].weight;
  }

  return selected_port_index;
}

}  // namespace isekai
