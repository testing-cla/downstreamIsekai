#include "isekai/host/falcon/falcon_counters.h"

#include <sstream>
#include <string>

namespace isekai {

std::string FalconConnectionCounters::DebugString() const {
  std::ostringstream stream;
  // clang-format off
  stream
      << "\n  Global FALCON counters "
      << "\n ========================================================"
      << "\n  pull_request_from_ulp  : " << pull_request_from_ulp
      << "\n  push_request_from_ulp  : " << push_request_from_ulp
      << "\n  pull_response_to_ulp   : " << pull_response_to_ulp
      << "\n  completions_to_ulp     : " << completions_to_ulp
      << "\n --------------------------------------------------------"
      << "\n  pull_request_to_ulp    : " << pull_request_to_ulp
      << "\n  push_request_to_ulp    : " << push_data_to_ulp
      << "\n  pull_response_from_ulp : " << pull_response_from_ulp
      << "\n  acks_from_ulp          : " << acks_from_ulp
      << "\n  nacks_from_ulp         : " << nacks_from_ulp
      << "\n --------------------------------------------------------"
      << "\n  initiator_tx_push_solicited_request : " <<
              initiator_tx_push_solicited_request
      << "\n  initiator_tx_push_unsolicited_data  : " <<
              initiator_tx_push_unsolicited_data
      << "\n  initiator_tx_pull_request           : " <<
              initiator_tx_pull_request
      << "\n  initiator_tx_push_solicited_data    : " <<
              initiator_tx_push_solicited_data
      << "\n  initiator_rx_push_grant             : " <<
              initiator_rx_push_grant
      << "\n  initiator_rx_pull_data              : " <<
              initiator_rx_pull_data
      << "\n --------------------------------------------------------"
      << "\n  target_tx_pull_data                 : " <<
              target_tx_pull_data
      << "\n  target_tx_push_grant                : " <<
              target_tx_push_grant
      << "\n  target_rx_push_solicited_request    : " <<
              target_rx_push_solicited_request
      << "\n  target_rx_push_solicited_data       : " <<
              target_rx_push_solicited_data
      << "\n  target_rx_push_unsolicited_data     : " <<
              target_rx_push_unsolicited_data
      << "\n  target_rx_pull_request              : " <<
              target_rx_pull_request
      << "\n --------------------------------------------------------"
      << "\n  rue_ack_events        : " << rue_ack_events
      << "\n  rue_nack_events       : " << rue_nack_events
      << "\n  rue_retransmit_events : " << rue_retransmit_events
      << "\n  rue_responses         : " << rue_responses
      << "\n --------------------------------------------------------"
      << "\n  rx_packets                              : " <<
              rx_packets
      << "\n  rx_acks                                 : " <<
              rx_acks
      << "\n  rx_nacks                                : " <<
              rx_nacks
      << "\n  total_rx_dropped_pkts                   : " <<
              total_rx_dropped_pkts
      << "\n  rx_resource_dropped_transaction_pkts    : " <<
              rx_resource_dropped_transaction_pkts
      << "\n  rx_window_dropped_transaction_pkts      : " <<
              rx_window_dropped_transaction_pkts
      << "\n  rx_duplicate_dropped_transaction_pkts   : " <<
              rx_duplicate_dropped_transaction_pkts
      << "\n  rx_duplicate_dropped_ack_pkts           : " <<
              rx_duplicate_dropped_ack_pkts
      << "\n  rx_duplicate_dropped_nack_pkts          : " <<
              rx_duplicate_dropped_nack_pkts
      << "\n  tx_packets                              : " <<
              tx_packets
      << "\n  tx_retransmit                           : " <<
              tx_timeout_retransmitted
      << "\n  tx_acks                                 : " <<
              tx_acks
      << "\n  tx_nacks                                : " <<
              tx_nacks
      << "\n ========================================================"
      << "\n";
  // clang-format on
  return stream.str();
}

}  // namespace isekai
