#ifndef ISEKAI_HOST_FALCON_FALCON_H_
#define ISEKAI_HOST_FALCON_FALCON_H_

#include <cstdint>
namespace falcon {

enum class ProtocolType : uint8_t {
  kInvalid = 0,
  kFalcon = 1,
  kRdma = 2,
  kGnvme = 3,
};

enum class PacketType : uint8_t {
  kInvalid = 15,
  kPullRequest = 0,
  kPushRequest = 1,
  kPushGrant = 2,
  kPullData = 3,
  kPushSolicitedData = 4,
  kPushUnsolicitedData = 5,
  kResync = 6,
  kAck = 7,
  kNack = 8,
  // Types below added in C0.
  kBack = 9,
  kEack = 10,
};

enum class ResyncCode : uint8_t {
  kTargetUlpCompletionInError = 1,
  kLocalXlrFlow = 2,
  kTransactionTimedOut = 4,
  kRemoteXlrFlow = 5,
  kTargetUlpNonRecoverableError = 6,
  kTargetUlpInvalidCidError = 7,
};

enum class NackCode : uint8_t {
  kNotANack = 0,  // reserved
  kRxResourceExhaustion = 1,
  kUlpReceiverNotReady = 2,
  kXlrDrop = 4,
  kRxWindowError = 5,
  kUlpCompletionInError = 6,
  kUlpNonRecoverableError = 7,
  kInvalidCidError = 8,
};

// Informs the rate update engine (RUE) of the event type being generated.
// See //isekai/host/falcon/rue/format.h
enum class RueEventType : uint8_t {
  kAck = 0,
  kNack = 1,
  kRetransmit = 2,
};

// Informs the RUE of the type of delay measurement to perform.
// See //isekai/host/falcon/rue/format.h
enum class DelaySelect : uint8_t {
  kFull = 0,
  kFabric = 1,
  kForward = 2,
  kReverse = 3,
};

// Describes the direction the congestion window last took.
enum class WindowDirection : uint8_t {
  kDecrease = 0,
  kIncrease = 1,
};

// Describes the reason a packet is being retransmitted.
enum class RetransmitReason : uint8_t {
  kTimeout = 0,
  kEarly = 1,
};

}  // namespace falcon

#endif  // ISEKAI_HOST_FALCON_FALCON_H_
