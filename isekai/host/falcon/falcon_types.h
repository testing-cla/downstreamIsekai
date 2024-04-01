#ifndef ISEKAI_HOST_FALCON_FALCON_TYPES_H_
#define ISEKAI_HOST_FALCON_FALCON_TYPES_H_

#include <cstdint>
#include <memory>
#include <utility>

#include "absl/time/time.h"
#include "isekai/common/constants.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon.h"

// This file contains definitions of internal Falcon constructs and types.

namespace isekai {

// List of Falcon transaction types.
enum class TransactionType {
  kPull,
  kPushSolicited,
  kPushUnsolicited,
};

// Indicates the location of a transaction. The individual packets are handled
// according to the transaction location, i.e., the state machine is updated
// based on location.
enum class TransactionLocation {
  kInitiator,
  kTarget,
};

// Represents the stage of a transaction. Used to update state of a transaction.
// States of other stages such as UlpRx and NtwkRx is updated implicitly when
// the transaction metadata is created.
enum class TransactionStage {
  UlpRx,            // Represents receiving a transaction from ULP.
  UlpTx,            // Represents sending a transaction/completion to ULP.
  UlpAckRx,         // Represents receiving an ACK from ULP.
  UlpRnrRx,         // Represents receiving an RNR NACK from ULP.
  UlpCompletionTx,  // Represents completion to ULP, for Push transactions only.
  NtwkRx,     // Represents Falcon receiving a transaction from the network.
  NtwkTx,     // Represents Falcon sending a transaction to network.
  NtwkAckRx,  // Represents receiving an ACK from network.
};

// Represents the possible states of a transaction (both at the initiator as
// well as target).
enum class TransactionState {
  kInvalid,
  // Falcon received transactions from ULP.
  kPullReqUlpRx,
  kPullDataUlpRx,
  kPushSolicitedReqUlpRx,
  kPushUnsolicitedReqUlpRx,
  // Falcon transmitted transactions and completion to ULP.
  kPullReqUlpTx,
  kPullDataUlpTx,
  kPushSolicitedDataUlpTx,
  kPushUnsolicitedDataUlpTx,
  kPushCompletionUlpTx,
  // Falcon received ACK from ULP.
  kPullReqAckUlpRx,
  kPushSolicitedDataAckUlpRx,
  kPushUnsolicitedDataAckUlpRx,
  // Falcon received RNR NACK from ULP.
  kPullReqRnrUlpRx,
  kPushSolicitedDataRnrUlpRx,
  kPushUnsolicitedDataRnrUlpRx,
  // Falcon received transactions and resync from network.
  kPullReqNtwkRx,
  kPullDataNtwkRx,
  kPushReqNtwkRx,
  kPushGrantNtwkRx,
  kPushSolicitedDataNtwkRx,
  kPushUnsolicitedDataNtwkRx,
  kResyncNtwkRx,
  // Falcon transmitted transactions to network.
  kPullReqNtwkTx,
  kPullDataNtwkTx,
  kPushReqNtwkTx,
  kPushGrantNtwkTx,
  kPushSolicitedDataNtwkTx,
  kPushUnsolicitedDataNtwkTx,
  // Falcon received ACK from network.
  kPullDataAckNtwkRx,
  kPushSolicitedDataAckNtwkRx,
  kPushUnsolicitedDataAckNtwkRx,
};

// Indicates the direction of a packet within a transaction.
enum class PacketDirection {
  kOutgoing,
  kIncoming,
};

// Describes the reason a packet is being retransmitted.
enum class RetransmitReason : uint8_t {
  kTimeout = 0,
  // For OOO-distance triggered retransmission.
  kEarlyOooDis = 1,
  // For RACK triggered retransmission.
  kEarlyRack = 2,
  // For TLP triggered retransmission.
  kEarlyTlp = 3,
  // For early retransmission triggered by out-of-sequence NACK.
  kEarlyNack = 4,
  // For retransmission of received packet -- this indicates some thing from
  // target ULP is missing, so this is for recovery of ULP ACK/NACK, hence
  // called ULP-RTO.
  kUlpRto = 5,
};

// Structure to hold metadata from the ULP about the NACK.
struct UlpNackMetadata {
  Packet::Syndrome ulp_nack_code;
  absl::Duration rnr_timeout = absl::ZeroDuration();
};

// The structure of an ack coalescing key, which contains the scid of the
// connection.
struct AckCoalescingKey {
  uint32_t scid = 0;

  explicit AckCoalescingKey(uint32_t scid) : scid(scid) {}
  AckCoalescingKey() {}
  virtual ~AckCoalescingKey() {}
  template <typename H>
  friend H AbslHashValue(H state, const AckCoalescingKey& value) {
    state = H::combine(std::move(state), value.scid);
    return state;
  }
  // Two AckCoalescingKey structs are equal if they share the same scid.
  inline bool operator==(const AckCoalescingKey& other) const {
    return other.scid == scid;
  }
};

// Indicates the location of the packet buffers to be in either on-NIC SRAM or
// DRAM.
enum class PacketBufferLocation {
  kSram,
  kDram,
};

// The structure of a transaction key, which contains the rsn and location of
// the transaction to uniquely identify and access a transaction in Falcon.
struct TransactionKey {
  uint32_t rsn;
  TransactionLocation location;

  template <typename H>
  friend H AbslHashValue(H h, const TransactionKey& s) {
    return H::combine(std::move(h), s.rsn, s.location);
  }

  inline bool operator==(const TransactionKey& s) const {
    return (rsn == s.rsn && location == s.location);
  }

  TransactionKey(uint32_t rsn, TransactionLocation location)
      : rsn(rsn), location(location) {}
  TransactionKey() {}
};

// The structure of a UlpTransaction, containing the rsn and the type of the
// transaction packet received from ULP.
struct UlpTransactionInfo {
  uint32_t rsn;
  falcon::PacketType type;

  template <typename H>
  friend H AbslHashValue(H h, const UlpTransactionInfo& s) {
    return H::combine(std::move(h), s.rsn, s.type);
  }

  inline bool operator==(const UlpTransactionInfo& s) const {
    return (rsn == s.rsn && type == s.type);
  }

  UlpTransactionInfo(uint32_t rsn, falcon::PacketType type)
      : rsn(rsn), type(type) {}
  UlpTransactionInfo() {}
};

// The structure contains the expected and actual length of the transaction
// received from the network.
struct NetworkTransactionInfo {
  uint16_t expected_length;
  uint16_t actual_length;

  template <typename H>
  friend H AbslHashValue(H h, const NetworkTransactionInfo& s) {
    return H::combine(std::move(h), s.expected_length, s.actual_length);
  }

  inline bool operator==(const NetworkTransactionInfo& s) const {
    return (expected_length == s.expected_length &&
            actual_length == s.actual_length);
  }

  NetworkTransactionInfo(uint16_t expected_length, uint16_t actual_length)
      : expected_length(expected_length), actual_length(actual_length) {}
  NetworkTransactionInfo() {}
};

typedef FalconBitmap<kRxBitmapWidth> FalconRxBitmap;
typedef FalconBitmap<kTxBitmapWidth> FalconTxBitmap;
typedef FalconBitmap<kGen2RxBitmapWidth> Gen2FalconRxBitmap;
typedef FalconBitmap<kGen2TxBitmapWidth> Gen2FalconTxBitmap;
typedef AckCoalescingKey RueKey;

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_TYPES_H_
