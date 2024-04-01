#ifndef ISEKAI_HOST_FALCON_GEN3_REORDER_ENGINE_H_
#define ISEKAI_HOST_FALCON_GEN3_REORDER_ENGINE_H_

#include "isekai/host/falcon/gen2/reorder_engine.h"

namespace isekai {

class Gen3ReorderEngine : public Gen2ReorderEngine {
 public:
  explicit Gen3ReorderEngine(FalconModelInterface* falcon);
  // Inserts a given packet type into the reorder buffer with given cid, rsn and
  // ssn. The ssn is only relevant for PushRequest and PushGrant packet types.
  // It is ignored for all other packet types.
  absl::Status InsertPacket(falcon::PacketType packet_type, uint32_t cid,
                            uint32_t rsn, uint32_t ssn) override;
  // Retries a RNR-NACKed packet upon a duplicate packet. Returns
  // FailedPreconditionError if the conection is ordered and in RNR state and
  // this packet is not HoL.
  absl::Status RetryRnrNackedPacket(uint32_t cid, uint32_t rsn) override;
  // Determines if the given RSN reflects a HoL request or not.
  bool IsHeadOfLineNetworkRequest(uint32_t cid, uint32_t rsn) override;
  // Handles RNR NACK.
  void HandleRnrNackFromUlp(uint32_t cid, uint32_t rsn,
                            absl::Duration rnr_timeout) override;
  // Ack a transaction from ULP at Target.
  void HandleAckFromUlp(uint32_t cid, uint32_t rsn) override;
  // Returns the RNR timeout of a connection.
  absl::Duration GetRnrTimeout(uint32_t cid) override;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN3_REORDER_ENGINE_H_
