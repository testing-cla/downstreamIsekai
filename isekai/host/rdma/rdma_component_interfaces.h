#ifndef ISEKAI_HOST_RDMA_RDMA_COMPONENT_INTERFACES_H_
#define ISEKAI_HOST_RDMA_RDMA_COMPONENT_INTERFACES_H_

#include <cstdint>
#include <deque>
#include <memory>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/functional/any_invocable.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/net_address.h"
#include "isekai/common/packet.h"

namespace isekai {

inline constexpr QpId kInvalidQpId = 0;

// Internal representation of an RDMA operation, such as Read, Write, Recv,
// Send or Atomic.
struct RdmaOp {
  const RdmaOpcode opcode;
  // Exactly one of sgl or inline_payload_length will be set.
  std::vector<uint32_t> sgl;
  const uint32_t inline_payload_length = 0;
  // dest_qp_id is needed in UD mode.
  QpId dest_qp_id = kInvalidQpId;
  // Completion callback function that is called when the operation finishes.
  const CompletionCallback completion_callback;

  // Each OP is divided into multiple packets (or transactions). psn is the
  // sequence number of the first packet on this OP, and end_psn is the sequence
  // number of the last packet of this OP. (Same as RSN in RDMA/FALCON)
  uint32_t psn = 0, end_psn = 0;
  // Length of the op.
  uint32_t length = 0;
  // Number of packets of this op.
  uint32_t n_pkts = 0;
  // Number of packets finished.
  uint32_t n_pkt_finished = 0;

  // For collecting latency statistics.
  struct {
    bool is_scheduled = false;
    absl::Duration post_timestamp;        // RdmaOp was given to RDMA.
    absl::Duration start_timestamp;       // First packet was sent downstream.
    absl::Duration finish_timestamp;      // Last packet was sent downstream.
    absl::Duration completion_timestamp;  // Last completion/data was received.
  } stats;

  RdmaOp(RdmaOpcode opcode, std::vector<uint32_t> sgl,
         CompletionCallback completion_callback, QpId dest_qp_id)
      : opcode(opcode),
        sgl(std::move(sgl)),
        dest_qp_id(dest_qp_id),
        completion_callback(std::move(completion_callback)) {
    length = 0;
    for (auto sg_len : this->sgl) {
      length += sg_len;
    }
  }

  RdmaOp(RdmaOpcode opcode, uint32_t inline_payload_length,
         CompletionCallback completion_callback, QpId dest_qp_id)
      : opcode(opcode),
        inline_payload_length(inline_payload_length),
        dest_qp_id(dest_qp_id),
        completion_callback(std::move(completion_callback)),
        length(inline_payload_length) {}
};

// An incoming read request arriving from the network from a remote RDMA node.
struct InboundReadRequest {
  // Source rsn.
  const uint32_t rsn = 0;
  // Request scatter-gather list.
  const std::vector<uint32_t> sgl;

  // For RoCE.
  uint32_t psn = 0, end_psn = 0;
  uint32_t length = 0;

  InboundReadRequest(uint32_t rsn, std::vector<uint32_t> sgl)
      : rsn(rsn), sgl(std::move(sgl)) {
    length = 0;
    for (auto sg_len : sgl) length += sg_len;
  }

  InboundReadRequest(bool roce, uint32_t psn, uint32_t end_psn, uint32_t length)
      : psn(psn), end_psn(end_psn), length(length) {}
};

// QueuePair context state that is stored for each QP on the NIC.
struct BaseQpContext {
  // Local RDMA QP id.
  QpId qp_id = kInvalidQpId;
  // Destination RDMA QP id, if QP is connected. Otherwise kInvalidQpId.
  QpId dest_qp_id = kInvalidQpId;
  // Connected QP ordering mode.
  RdmaConnectedMode rc_mode = RdmaConnectedMode::kUnorderedRc;
  // Destination IP address.
  Ipv6Address dst_ip;

  // Work queues for different types of RDMA operations.
  std::deque<std::unique_ptr<RdmaOp>> send_queue;
  std::deque<std::unique_ptr<RdmaOp>> receive_queue;
  std::deque<InboundReadRequest> inbound_read_request_queue;
  // Completion queue for requests sent out to the network (or FALCON). Contains
  // the original RDMA Op along with the completion callback that was provided
  // when issuing the op. It is keyed by the RSN (PSN) of the last packet
  // associated with the RdmaOp. The RdmaOp is inserted into this map once the
  // last packet is sent out, but the callback is executed only when all the
  // ACKs are received.
  //
  // through host interface.
  absl::flat_hash_map<uint32_t, std::unique_ptr<RdmaOp>> completion_queue;
  // Map from rsn to the RdmaOp.
  absl::flat_hash_map<uint32_t, RdmaOp*> rsn_to_op;
  uint32_t total_ops_completed = 0;

  inline bool is_connected() { return dest_qp_id != kInvalidQpId; }
  virtual ~BaseQpContext() = default;
};

struct RoceQpContext : BaseQpContext {
  uint32_t cwnd;
  // Number of inflight bytes.
  uint32_t inflight_bytes = 0;
  // The sending rate (Mbps).
  uint64_t rate = 100000;
  // The next time can send packet (allowed by rate).
  absl::Duration next_send_time = absl::ZeroDuration();

  // Initiator side:
  // The next PSN to assign.
  uint32_t next_psn = 0;
  // Oldest unacknowledged PSN.
  uint32_t oldest_unacknowledged_psn = 0;
  // Maximum PSN ever sent + 1.
  uint32_t max_psn = 0;
  // The PSN at the tail of the send queue.
  uint32_t tail_psn = 0;
  // The highest psn that request ACK.
  uint32_t highest_psn_with_ack_req = 0;
  // The index of the op that nxt_psn belongs to.
  uint32_t next_op_index = 0;
  // The interval for waiting before RTO.
  absl::Duration retx_timeout = absl::ZeroDuration();
  // The time when RTO timer started.
  absl::Duration rto_trigger_time = absl::ZeroDuration();
  // Counting number of retries
  uint32_t retry_counter = 0;
  // The maximum number of retries for a request.
  uint32_t max_retry = 8;
  // Whether a READ was retried. IB spec does not have this, but we add it
  // following the same principle for NAK generation (target's signal of retry
  // to initiator) in C9-114 in InfiniBand Architecture Release 1.4 Volume 1,
  // April 7, 2020.
  bool read_retried = false;

  // Target side:
  // The expected PSN.
  uint32_t expected_psn = 0;
  // The psn to send from the op at head of IRRQ.
  uint32_t irrq_psn = 0;

  // ACK coalecsing related:
  // Flag of whether ACK coalescing condition is met.
  bool ack_triggered = false;
  // The timeout for ACK coalescing.
  absl::Duration ack_coalescing_timeout = absl::ZeroDuration();
  // The time when the timer started.
  absl::Duration ack_coalescing_timer_trigger_time = absl::ZeroDuration();
  // The threshold for count of ACKs coalescing.
  uint32_t ack_coalescing_threshold = 1;
  // The number of ACKs coalesced so far.
  uint32_t ack_coalescing_counter = 0;

  // Whether a NAK-sequence-error was sent. This should be cleared when
  // receiving a new request matching the expecte PSN. (C9-114 in InfiniBand
  // Architecture Release 1.4 Volume 1, April 7, 2020).
  bool nak_sequence_error_sent = false;
  // Flag of whether NAK-sequence-error is triggered or not.
  bool nak_sequence_error_triggered = false;

  // CC states: derived from the descriptions in
  // https://community.mellanox.com/s/article/dcqcn-parameters.
  struct {
    uint64_t target_rate;
    uint32_t alpha = 1023;
    uint32_t byte_count = 0;
    uint32_t increase_stage_by_byte = 0;
    uint32_t increase_stage_by_timer = 0;
    bool alpha_cnp_arrived = false;
    bool first_cnp = true;
    bool decrease_cnp_arrived = false;
    absl::Duration rate_increase_timer_start_time = absl::ZeroDuration();
  } dcqcn;
};

struct FalconQpContext : BaseQpContext {
  // FALCON channel source connection id.
  uint32_t scid;
  // The maximum credit usage of this QP.
  FalconCredit credit_limit;
  // Current credit usage for FALCON resource pools.
  FalconCredit credit_usage;
  // Next RSN for packets of this QP.
  uint32_t next_rsn = 0;
  // Number of outstanding outbound read request.
  uint32_t outbound_read_requests = 0;
  // If this QP is in RNR state or not.
  bool in_rnr = false;
  // Next Rx RSN to process.
  uint32_t next_rx_rsn = 0;
  // RNR timeout.
  absl::Duration rnr_timeout = absl::ZeroDuration();
  // Next schedule time (required to enforce per-qo op rate limits).
  absl::Duration next_schedule_time = absl::ZeroDuration();
  // Variables to track the total time this QP was xoff'ed by Falcon.
  absl::Duration last_xoff_assert_time;
  uint32_t total_xoff_time_ns;
};

class RdmaQpManagerInterface {
 public:
  virtual ~RdmaQpManagerInterface() {}
  // Creates a QueuePair with the given local qp id and QpOptions.
  virtual void CreateQp(std::unique_ptr<BaseQpContext> context) = 0;
  // Connects the given local QueuePair with remote QueuePair.
  virtual void ConnectQp(QpId local_qp_id, QpId remote_qp_id,
                         RdmaConnectedMode rc_mode) = 0;
  // Looks up a QueuePair using qp_id and executes the callback after accounting
  // for caching behavior and DRAM latency/bandwidth.
  virtual void InitiateQpLookup(
      QpId qp_id,
      absl::AnyInvocable<void(absl::StatusOr<BaseQpContext*>)> callback) = 0;
  // Performs a direct lookup using qp_id and return context immediately.
  virtual BaseQpContext* DirectQpLookup(QpId qp_id) = 0;
};

class RdmaRoceCcInterface {
 public:
  virtual ~RdmaRoceCcInterface() {}
  // Initialize the QP context for CC.
  virtual void InitContext(RoceQpContext* context) = 0;
  // Handle incoming packets to update CC states.
  virtual void HandleRxPacket(RoceQpContext* qp_context, Packet* packet) = 0;
  // Handle transmitted packets to update CC states.
  virtual void HandleTxPacket(RoceQpContext* qp_context, Packet* packet) = 0;
};

// Types of work a QP has to process. Request (as the initiator) corresponds to
// Send Queue and response (as the target) corresponds to Inbound Read Request
// Queue.
enum class WorkType {
  kRequest = 0,
  kResponse = 1,
};

// Identifies and input queue to the RDMA Work Scheduler.
struct WorkQueue {
  QpId qp_id;
  WorkType work_type;

  template <typename H>
  friend H AbslHashValue(H h, const WorkQueue& s) {
    return H::combine(std::move(h), s.qp_id, s.work_type);
  }

  inline bool operator==(const WorkQueue& s) const {
    return (qp_id == s.qp_id && work_type == s.work_type);
  }

  inline bool operator<(const WorkQueue& s) const {
    if (qp_id < s.qp_id) {
      return true;
    } else if (qp_id == s.qp_id) {
      if (work_type < s.work_type) {
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  WorkQueue(QpId qp_id, WorkType work_type)
      : qp_id(qp_id), work_type(work_type) {}
};

class RdmaWorkSchedulerInterface {
 public:
  virtual ~RdmaWorkSchedulerInterface() {}
  // Adds a WorkQueue to the set of active WQs being scheduled by the scheduler.
  // The WorkQueu is identified by a QpId and WorkType, and must have pending
  // ops and sufficient credits when added for scheduling.
  virtual void AddQpToScheduleSet(QpId qp_id, WorkType work_type) = 0;
  // Asserts Xoff so that the work scheduler stops sending any new requests
  // (request_xoff) or both request and responses (global_xoff) downstream.
  virtual void SetXoff(bool request_xoff, bool global_xoff) = 0;
  // Connects the work scheduler to a FALCON interface which it uses for sending
  // request and response transactions.
  virtual void ConnectFalconInterface(FalconInterface* falcon) = 0;
};

class RdmaFreeListManagerInterface {
 public:
  virtual ~RdmaFreeListManagerInterface() {}
  virtual absl::Status AllocateResource(QpId qp_id) = 0;
  virtual absl::Status FreeResource(QpId qp_id) = 0;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_RDMA_RDMA_COMPONENT_INTERFACES_H_
