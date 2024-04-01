#ifndef ISEKAI_HOST_FALCON_GEN2_FALCON_UTILS_H_
#define ISEKAI_HOST_FALCON_GEN2_FALCON_UTILS_H_

#include <cstdint>

#include "absl/log/check.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/falcon_utils.h"
#include "isekai/host/falcon/gen2/falcon_types.h"

namespace isekai {

inline absl::StatusOr<const ReceivedPacketContext* const>
GetReceivedPacketContextFromBufferFetchWorkId(
    const FalconModelInterface* const falcon,
    const BufferFetchWorkId& work_id) {
  CHECK_OK_THEN_ASSIGN(
      auto connection_state,
      falcon->get_state_manager()->PerformDirectLookup(work_id.cid));
  const auto& transaction_location =
      GetTransactionLocation(work_id.type, /*is_incoming_packet=*/true);
  CHECK_OK_THEN_ASSIGN(
      auto packet_context,
      connection_state->rx_reliability_metadata.GetReceivedPacketContext(
          {work_id.rsn, transaction_location}));
  return packet_context;
}

inline absl::StatusOr<const PacketMetadata* const>
GetPacketMetadataFromBufferFetchWorkId(const FalconModelInterface* const falcon,
                                       const BufferFetchWorkId& work_id) {
  CHECK_OK_THEN_ASSIGN(
      auto connection_state,
      falcon->get_state_manager()->PerformDirectLookup(work_id.cid));
  CHECK_OK_THEN_ASSIGN(auto transaction,
                       connection_state->GetTransaction(TransactionKey(
                           work_id.rsn, GetTransactionLocation(
                                            /*type=*/work_id.type,
                                            /*is_incoming_packet=*/true))));
  CHECK_OK_THEN_ASSIGN(auto packet_metadata,
                       transaction->GetPacketMetadata(work_id.type));
  return packet_metadata;
}

// Gets the flow's ID from the flow label and the configured number of flows per
// connection. The Flow ID is represented by the LSBs in the flow label, where
// the number of LSBs depend on the configured num_flows_per_connection.
inline uint8_t GetFlowIdFromFlowLabel(uint32_t flow_label,
                                      uint8_t num_flows_per_connection) {
  return static_cast<uint8_t>(flow_label % num_flows_per_connection);
}

// The Gen2MultipathingNumFlows value is the number of flows (paths) per
// connection for Gen2 multipathing.  A value of 1  means that only one
// path/flow is used at a time by a connection, but still goes through all
// Gen2-multipathing-related code changes.

// moved to Gen3.
inline uint32_t GetGen2MultipathingNumFlows(
    const ConnectionState::ConnectionMetadata& connection_metadata) {
  switch (connection_metadata.connection_type) {
    case ConnectionState::ConnectionMetadata::ConnectionStateType::Gen1:
      // As a safety check, make sure ConnectionStateType is not Gen1 when we
      // call this function.
      LOG(FATAL) << "Gen1 connection trying to get number of flows for Gen2 "
                    "multipathing.";
    case ConnectionState::ConnectionMetadata::ConnectionStateType::Gen2:
      // If the ConnectionStateType is Gen2, then Gen2MultipathingNumFlows is
      // the degree_of_multipathing value in ConnectionMetadata and should be >
      // 0.
      CHECK_GT(connection_metadata.degree_of_multipathing, 0);
      return connection_metadata.degree_of_multipathing;
    case ConnectionState::ConnectionMetadata::ConnectionStateType::
        Gen3ExploratoryMultipath:
      // If the ConnectionStateType is Gen3ExploratoryMultipath,
      // Gen2MultipathingNumFlows is 1 to allow the case where we want to enable
      // exploratory multipathing in Gen2.
      return 1;
    case ConnectionState::ConnectionMetadata::ConnectionStateType::Gen3:
      // If the ConnectionStateType is Gen3,
      // Gen2MultipathingNumFlows is 1 to allow the case where we want to enable
      // Gen3 multipathing with other Gen2 features.
      return 1;
  }
}

// Gets the flow's ID from the flow label and the FalconInterface as well as the
// scid. We use scid to get ConnectionState from the ConnectionStateManager in
// FalconInterface. From ConnectionState, we fetch the configured number of
// flows for that connection.
inline uint8_t GetFlowIdFromFlowLabel(uint32_t flow_label,
                                      const FalconModelInterface* falcon,
                                      uint32_t scid) {
  CHECK_OK_THEN_ASSIGN(auto connection_state,
                       falcon->get_state_manager()->PerformDirectLookup(scid));
  uint32_t multipathing_num_flows =
      GetGen2MultipathingNumFlows(connection_state->connection_metadata);
  return GetFlowIdFromFlowLabel(flow_label, multipathing_num_flows);
}

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_FALCON_UTILS_H_
