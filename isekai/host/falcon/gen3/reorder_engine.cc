#include "isekai/host/falcon/gen3/reorder_engine.h"

#include "isekai/host/falcon/gen3/falcon_utils.h"

namespace isekai {

Gen3ReorderEngine::Gen3ReorderEngine(FalconModelInterface* falcon)
    : Gen2ReorderEngine(falcon) {}

// Inserts a given packet type into the reorder buffer with given cid, rsn and
// ssn. The ssn is only relevant for PushRequest and PushGrant packet types.
// It is ignored for all other packet types.
absl::Status Gen3ReorderEngine::InsertPacket(falcon::PacketType packet_type,
                                             uint32_t cid, uint32_t rsn,
                                             uint32_t ssn) {
  return Gen2ReorderEngine::InsertPacket(packet_type, cid, rsn, ssn);
}

// Retries a RNR-NACKed packet upon a duplicate packet. Returns
// FailedPreconditionError if the connection is ordered and in RNR state and
// this packet is not HoL.
absl::Status Gen3ReorderEngine::RetryRnrNackedPacket(uint32_t cid,
                                                     uint32_t rsn) {
  return Gen2ReorderEngine::RetryRnrNackedPacket(cid, rsn);
}

// Determines if the given RSN reflects a HoL request or not.
bool Gen3ReorderEngine::IsHeadOfLineNetworkRequest(uint32_t cid, uint32_t rsn) {
  return Gen2ReorderEngine::IsHeadOfLineNetworkRequest(cid, rsn);
}

// Handles RNR NACK.
void Gen3ReorderEngine::HandleRnrNackFromUlp(uint32_t cid, uint32_t rsn,
                                             absl::Duration rnr_timeout) {
  return Gen2ReorderEngine::HandleRnrNackFromUlp(cid, rsn, rnr_timeout);
}

// Ack a transaction from ULP at Target.
void Gen3ReorderEngine::HandleAckFromUlp(uint32_t cid, uint32_t rsn) {
  return Gen2ReorderEngine::HandleAckFromUlp(cid, rsn);
}

// Returns the RNR timeout of a connection.
absl::Duration Gen3ReorderEngine::GetRnrTimeout(uint32_t cid) {
  return Gen2ReorderEngine::GetRnrTimeout(cid);
}

}  // namespace isekai
