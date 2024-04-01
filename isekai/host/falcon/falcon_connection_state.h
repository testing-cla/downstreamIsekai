#ifndef ISEKAI_HOST_FALCON_FALCON_CONNECTION_STATE_H_
#define ISEKAI_HOST_FALCON_FALCON_CONNECTION_STATE_H_

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "isekai/common/constants.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_bitmap.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/gating_variable.h"
#include "isekai/host/falcon/rue/bits.h"

namespace isekai {
// Stores metadata for a Falcon packet withing a pending transaction.
struct PacketMetadata {
  uint32_t psn;
  falcon::PacketType type;
  // Represents the direction of the packet.
  PacketDirection direction;
  // Represents if the transaction packet holds resources currently or not.
  absl::Status resources_reserved = absl::OkStatus();
  // Represents the number of transmission attempts of this packet, including
  // both early and timeout based retransmissions.
  uint32_t transmit_attempts = 0;
  // Represents the number of timeout based retransmissions.
  uint32_t timeout_retransmission_attempts = 0;
  // Represents the number of times this packet is early-retx'ed.
  uint32_t early_retx_attempts = 0;
  // Represents the reason of retransmission of the packet.
  RetransmitReason retransmission_reason = RetransmitReason::kTimeout;
  // Represents the total length of CRTBTH and other RDMA headers and any inline
  // data if present (does not include SGL length).
  uint32_t data_length = 0;
  // Represents the SGL length in the TX PMD.
  uint32_t sgl_length = 0;
  // Represents the Falcon payload length (if an incoming packet).
  uint32_t payload_length = 0;
  // Represents the time when the packet was transmitted.
  absl::Duration transmission_time = absl::ZeroDuration();
  // Maintains a packet copy to trigger retransmissions.
  Packet retransmission_packet_copy;
  std::unique_ptr<Packet> active_packet;
  // Represents if the solicitation layer has attempted to schedule the
  // transaction and hand it over to the sliding window layer.
  absl::Status schedule_status = absl::UnknownError("Not scheduled.");
  // Counters related to queuing in the connection Scheduler.
  struct PacketQueuingDelayMetadata {
    // Represents the time when the packet is queued at the scheduler.
    absl::Duration enqueue_time = absl::ZeroDuration();
    // Represents the time when the packet becomes TX-eligible.
    // Push solicited data becomes TX-eligible when receiving the push grant.
    // Unsolicited data becomes TX-eligible when the phantom request is
    // scheduled.
    absl::Duration tx_eligible_time = absl::ZeroDuration();
    // Snapshot of connection scheduler packet type-wise TX counters when this
    // packet is enqueued. These are compared against the counters later when
    // this packet is scheduled to calculate how many packets *per type* are
    // scheduled during the queuing of this packet.
    absl::flat_hash_map<falcon::PacketType, uint64_t>
        enqueue_packet_type_wise_tx_count_snapshot;
    // Snapshot of connection shceduler queue-wise TX counters when this
    // packet is enqueued. These are compared against the counters later when
    // this packet is scheduled to calculate how many packets *per queue* are
    // scheduled during the queuing of this packet.
    absl::flat_hash_map<PacketTypeQueue, uint64_t>
        enqueue_packet_queue_wise_pkt_count_snapshot;
  } queuing_delay_metadata;
  // Represents if the packet is eligible for transmission or not.
  // In ordered connections, Solicited Push Data packets are not eligible for
  // scheduling until the corresponding grant is received (as they are enqueued
  // along with Push Request into the connection scheduler to maintain ordering
  // between solicited and unsolicited data which is required to prevent
  // deadlocks).
  bool is_transmission_eligible = true;
  // If this packet is received or not.
  bool received = false;
  // Represents if the packet has been NACKed by the ULP.
  falcon::NackCode nack_code = falcon::NackCode::kNotANack;
  UlpNackMetadata ulp_nack_metadata;
  // This flag is required for EACK-OWN related changes. The flag is set to true
  // when a packet is created and when a packet is timeout-based retransmitted.
  // The bypass is disallowed, i.e., flag is set to false, once a packet had
  // been early retransmitted.
  bool bypass_recency_check = true;
  // This flag is required for EACK-OWN related changes. The flag is set to true
  // when a packet is early retransmistted as a consequence of serving an EACK.
  // The bypass is disallowed, i.e., flag set to false when -
  // (a) Packet is early retransmitted while servicing EACK-OWN, or
  // (b) Packet bypasses the scanning exit criteria, or
  // (c) Packet is timeout-based retransmitted.
  bool bypass_scanning_exit_criteria = false;
  PacketMetadata() {}
  // By default all packets are eligible to be considered for transmission by
  // the connection scheduler, leaving ordered push solicited data.
  PacketMetadata(falcon::PacketType _type, PacketDirection _direction,
                 std::unique_ptr<Packet> _packet,
                 bool _is_transmission_eligible)
      : type(_type),
        direction(_direction),
        active_packet(std::move(_packet)),
        is_transmission_eligible(_is_transmission_eligible) {}
};

// Represents generic transaction metadata.
struct TransactionMetadata {
  uint32_t rsn;
  uint32_t request_length;
  TransactionType type;
  TransactionLocation location;
  TransactionState state = TransactionState::kInvalid;
  // Represents if this transaction should decrement ORRC on receiving Pull
  // Response. Along with this flag, the transaction type as well as the global
  // config flag which decides if ORRC/ORC decrement happens on Pull Response or
  // Pull Request also determines how ORRC/ORC mutation happens. This additional
  // flag is required to handle the case where a retransmitted Pull Request is
  // in the retransmission scheduler and we get a Pull Response, in that case we
  // should not decrement ORRC since its decremented already when the Pull
  // Request is put in the retx scheduler.

  bool decrement_orrc_on_pull_response = true;
  // Represents if resources have been reserved proactively for the entire
  // transaction.
  absl::Status resources_reserved = absl::OkStatus();
  // Enables capturing solicitation window occupancy latency.
  absl::Duration solicitation_window_reservation_timestamp;
  // Holds the metadata corresponding to the individual packets that make up
  // this transaction, keyed by the packet type.
  absl::flat_hash_map<falcon::PacketType, std::unique_ptr<PacketMetadata>>
      packets;

  // Creates a packet for this transaction.
  void CreatePacket(falcon::PacketType type, PacketDirection direction,
                    std::unique_ptr<Packet> packet,
                    bool is_transmission_eligible = true) {
    packets[type] = std::make_unique<PacketMetadata>(
        type, direction, std::move(packet), is_transmission_eligible);
  }
  // Creates a packet and sets the data/sgl length (typically done for ULP
  // generated packets).
  void CreatePacket(falcon::PacketType type, PacketDirection direction,
                    uint32_t data_length, uint32_t sgl_length,
                    std::unique_ptr<Packet> packet,
                    bool is_transmission_eligible = true) {
    CreatePacket(type, direction, std::move(packet), is_transmission_eligible);
    packets[type]->data_length = data_length;
    packets[type]->sgl_length = sgl_length;
  }
  // Creates a packet and sets the Falcon payload length (typically done for
  // network generated transactions).
  void CreatePacket(falcon::PacketType type, PacketDirection direction,
                    uint32_t payload_length, std::unique_ptr<Packet> packet,
                    bool is_transmission_eligible = true) {
    packets[type] = std::make_unique<PacketMetadata>(
        type, direction, std::move(packet), is_transmission_eligible);
    packets[type]->payload_length = payload_length;
  }
  // Returns the packet metadata based on packet type.
  absl::StatusOr<PacketMetadata*> GetPacketMetadata(falcon::PacketType type) {
    auto it = packets.find(type);
    if (it != packets.end()) {
      return it->second.get();
    }
    return absl::NotFoundError("Packet not found.");
  }
  // Updates the state of the transaction based on the packet type and the
  // current stage of the transaction.
  void UpdateState(falcon::PacketType type, TransactionStage stage,
                   FalconInterface* falcon);
  TransactionMetadata() {}
  TransactionMetadata(uint32_t rsn, TransactionType ctype,
                      TransactionLocation location)
      : rsn(rsn), type(ctype), location(location) {}

 private:
  absl::Duration transaction_start_timestamp;  // Enables capturing transaction
                                               // end-to-end latency.
  absl::Duration last_state_update_timestamp;  // Enables capturing transaction
                                               // stage latencies.
};

// Stores the context of the received packet within the sliding window layer
// (referred to as rx_cmd_ctx in the Falcon MAS). The lifetime of this context
// is as follows -
// 1. Created (if does not exist) when sending a packet out and when receiving a
// packet.
// 2. Deleted when ULP ACKs (at the target) and when completions are delivered
// to ULP at the initiator.
struct ReceivedPacketContext {
  // Stores the local QP ID as this is required to send a completion (including
  // ULP NACK) to the ULP.
  uint32_t qp_id = 0;
  // Stores the PSN of the received packet as this is required when a ULP NACK
  // needs to be sent out.
  uint32_t psn = 0;
  // Stores the type of the packet received as this is required when a ULP NACK
  // needs to be sent out.
  falcon::PacketType type;
  // Stored for incoming packets with AR-bit set so that we can send back an ACK
  // after ULP acks the packet.
  bool is_ack_requested = false;
  // Location of the packet buffer, by default packets received are in SRAM.

  PacketBufferLocation location = PacketBufferLocation::kSram;
  ReceivedPacketContext() {}
};

// Stores the metadata related to the receiver reliability window (e.g., data
// window or request window)
struct ReceiverReliabilityWindowMetadata {
  uint32_t base_packet_sequence_number = 0;
  uint32_t next_packet_sequence_number = 0;
  // Indicates if this window has experienced sliding window drops. It is reset
  // once an ACK is sent out on this connection conveying this information.
  bool has_oow_drop = false;
  //
  // Bitmap to indicate if a packet has been received by Falcon.
  std::unique_ptr<FalconBitmapInterface> receive_window;
  // Bitmap to indicate if a packet has been acknowledged by Falcon. For packet
  // types of Push Solicited Data and Push Unsolicited Data, the ULP needs to
  // send out an explicit ACK for us to change the corresponding bit to 1. For
  // other packet types, this bit is set to 1 when Falcon receives the packet.
  std::unique_ptr<FalconBitmapInterface> ack_window;
  explicit ReceiverReliabilityWindowMetadata(WindowType type, int version) {
    if (version == 1) {
      receive_window = std::make_unique<FalconRxBitmap>(
          type == WindowType::kRequest ? kRxRequestWindowSize
                                       : kRxDataWindowSize);
      ack_window = std::make_unique<FalconRxBitmap>(
          (type == WindowType::kRequest ? kRxRequestWindowSize
                                        : kRxDataWindowSize));

    } else if (version >= 2) {
      receive_window = std::make_unique<Gen2FalconRxBitmap>(kGen2RxBitmapWidth);
      ack_window = std::make_unique<Gen2FalconRxBitmap>(kGen2RxBitmapWidth);
    }
  }
};

// Stores the context of the outstanding packet within the sliding window layer.
struct OutstandingPacketContext {
  uint32_t rsn = 0;
  falcon::PacketType packet_type;
  OutstandingPacketContext() {}
  OutstandingPacketContext(uint32_t rsn, falcon::PacketType type)
      : rsn(rsn), packet_type(type) {}
  virtual ~OutstandingPacketContext() {}
};

// Stores the metadata related to the transmitter reliability window (e.g., data
// window or request window)
struct TransmitterReliabilityWindowMetadata {
  static const int kTransmitterRequestWindowSize = 1024;
  static const int kTransmitterDataWindowSize = 1024;
  WindowType type;
  GatingVariable<uint32_t> base_packet_sequence_number;
  GatingVariable<uint32_t> next_packet_sequence_number;
  GatingVariable<uint32_t> outstanding_requests_counter;
  // This variable and its behavior differs from the HW implementation, and is
  // not necessary in a HW implementation.
  GatingVariable<uint32_t> outstanding_retransmission_requests_counter;
  GatingVariable<bool> oow_notification_received;
  std::unique_ptr<FalconBitmapInterface> window;
  // Set that prioritizes outstanding packets with lower PSN.
  absl::btree_set<RetransmissionWorkId> outstanding_packets;
  // Mapping between PSN and outstanding packet context (consists of RSN, etc).
  absl::flat_hash_map<uint32_t, std::unique_ptr<OutstandingPacketContext>>
      outstanding_packet_contexts;
  absl::StatusOr<const OutstandingPacketContext* const> GetOutstandingPacketRSN(
      uint32_t psn) const {
    auto it = outstanding_packet_contexts.find(psn);
    if (it != outstanding_packet_contexts.end()) {
      return (it->second).get();
    }
    return absl::NotFoundError("OutstandingPacketContext not found.");
  }
  explicit TransmitterReliabilityWindowMetadata(WindowType type, int version)
      : type(type) {
    if (version == 1) {
      window = std::make_unique<FalconTxBitmap>(kTxBitmapWidth);

    } else if (version >= 2) {
      window = std::make_unique<Gen2FalconTxBitmap>(kGen2TxBitmapWidth);
    }
  }
};

inline constexpr int kIpv6FlowLabelNumBits = 20;
// A connection is limited to 40Mops/s, which translates to a per-connection
// inter op gap of 25ns.
inline constexpr absl::Duration kPerConnectionInterOpGap =
    absl::Nanoseconds(25);
// In hardware, after a packet is retransmitted threshold (say 7) times, it
// notifies software to take the next actions (e.g., teardown connection).
inline constexpr int kDefaultMaxRetransmitAttempts =
    std::numeric_limits<int>::max();
// Set to 64 (smaller of the window sizes), effectively turning off the feature.
inline constexpr int kDefaultEarlyTransmitLimit = 64;
// Suggested by frwang@, used by DV for their performance simulations,
//
static constexpr int kDefaultAckCoalescingThreshold = 10;
static constexpr absl::Duration kDefaultAckCoalescingTimer =
    absl::Microseconds(8);
// ULP-RTO is expected to be >= RTO.
inline constexpr absl::Duration kDefaultUlpRto = absl::Milliseconds(5);

inline constexpr uint8_t kDefaultBifurcationId = 0;

// Stores all the per-connection metadata related to the various aspects
// of the FALCON protocol such as sliding window, reliability etc.
struct ConnectionState {
  struct ConnectionMetadata {
    uint32_t scid;
    uint32_t dcid;
    std::string dst_ip = "0:0:0:0:0:0:0:0";
    enum class RdmaMode {
      kRC,
      kUD,
    } rdma_mode = RdmaMode::kRC;
    OrderingMode ordered_mode = OrderingMode::kOrdered;
    uint32_t max_retransmit_attempts = kDefaultMaxRetransmitAttempts;
    uint32_t early_retransmit_limit = kDefaultEarlyTransmitLimit;
    uint32_t ack_coalescing_threshold = kDefaultAckCoalescingThreshold;
    absl::Duration ack_coalescing_timeout = kDefaultAckCoalescingTimer;
    uint8_t source_bifurcation_id = kDefaultBifurcationId;
    uint8_t destination_bifurcation_id = kDefaultBifurcationId;

    enum class ConnectionStateType {
      Gen1,
      Gen2,
      Gen3,
      Gen3ExploratoryMultipath,
    } connection_type = ConnectionStateType::Gen1;
    uint32_t degree_of_multipathing = 1;
    // This optional vector (static_routing_port_lists) includes the static
    // routing port lists for every path, indexed by path ID. Each element
    // specifies the router ports that packets on this path should traverse
    // sequentially, either in the forward or reverse direction. The number of
    // elements in the static_routing_port_lists must be equal to the degree of
    // multipathing. Non-multipathing cases are also supported, as the degree of
    // multipathing is 1 by default.
    // This variable is used only for specific simulation tests that aim to
    // enable static routing.
    std::optional<std::vector<std::vector<uint32_t>>> static_routing_port_lists;
  } connection_metadata;

  // These values are used by the backpressure manager.
  struct ConnectionRdmaXoffMetadata {
    // If the connection is RC, this metadata stores the corresponding qp_id of
    // this connection.
    uint64_t qp_id = 0;
    // Whether this connection's xoff has been asserted by the backpressure
    // manager.
    bool qp_xoff = false;
    uint8_t alpha_request;
    uint8_t alpha_response;
  } connection_xoff_metadata;

  // see //isekai/host/falcon/rue/format.h
  struct CongestionControlMetadata {
    // These values are used by various FALCON components
    uint32_t flow_label : kIpv6FlowLabelNumBits;
    GatingVariable<uint32_t> fabric_congestion_window;
    absl::Duration inter_packet_gap;
    GatingVariable<uint32_t> nic_congestion_window;
    absl::Duration retransmit_timeout;
    // These values are only used in the RUE
    uint32_t cc_metadata : falcon_rue::kCcMetadataBits;
    uint32_t fabric_window_time_marker : falcon_rue::kTimeBits;
    uint32_t nic_window_time_marker : falcon_rue::kTimeBits;
    falcon::WindowDirection nic_window_direction
        : falcon_rue::kWindowDirectionBits;
    falcon::DelaySelect delay_select : falcon_rue::kDelaySelectBits;
    uint32_t base_delay : falcon_rue::kTimeBits;
    uint32_t delay_state : falcon_rue::kTimeBits;
    uint32_t rtt_state : falcon_rue::kTimeBits;
    uint8_t cc_opaque : falcon_rue::kDnaCcOpaqueBits;
    uint8_t rx_buffer_level : falcon_rue::kRxBufferLevelBits;
    // Number of packets ACKed since last successful RUE event.
    uint32_t num_acked;
    // Set when posting RUE event, reset when the RUE response is processed.
    bool outstanding_rue_event;
    // Record of the time of the last successful RUE event.
    absl::Duration last_rue_event_time;
    // These fields are Gen2-specific.

    uint32_t gen2_plb_state
        : falcon_rue::kPlbStateBits;  // PLB state in Gen2 has a separate field
                                      // in the datapath

    // bit sizing as commented.
    std::vector<uint8_t> gen2_flow_weights;  // Each weights is 4 bits.
    std::vector<uint32_t> gen2_flow_labels;  // Each label is 20 bits.
    // Number of packets ACKed since last successful RUE event for every flow.
    std::vector<uint32_t> gen2_num_acked;
    // Set when posting RUE event for a flow, reset when the corresponding
    // RUE response is processed.
    std::vector<bool> gen2_outstanding_rue_event;
    // Record of the time of the last successful RUE event for every flow.
    std::vector<absl::Duration> gen2_last_rue_event_time;
  } congestion_control_metadata;

  // This is the per-connection resource profile.
  // The default values enforce no limits per connection.
  struct ResourceProfile {
    struct Thresholds {
      uint32_t shared_total = 0xffffffff;
      uint32_t shared_hol = 0xffffffff;
      uint32_t guarantee_ulp = 0xffffffff;
      uint32_t guarantee_network = 0xffffffff;
    };
    Thresholds tx_packet;
    Thresholds tx_buffer;
    Thresholds rx_packet;
    Thresholds rx_buffer;
  } resource_profile;

  struct TransmitterTransactionMetadata {
    uint32_t base_request_sequence_number = 0;
    uint32_t next_request_sequence_number = 0;
    uint32_t current_solicitation_sequence_number = 0;
  } tx_transaction_metadata;

  struct ReceiverTransactionMetadata {
    uint32_t base_request_sequence_number = 0;
    uint32_t next_request_sequence_number = 0;
  } rx_transaction_metadata;

  struct TransmitterReliabilityMetadata {
    TransmitterReliabilityWindowMetadata request_window_metadata;
    TransmitterReliabilityWindowMetadata data_window_metadata;

    // The actual RTO expiration is at rto_expiration_base_time +
    // rto_expiration_jitter. rto_expiration_base_time is the time of the
    // schedule RTO event without jitter. rto_expiration_jitter is the jitter.
    absl::Duration rto_expiration_base_time = absl::ZeroDuration();
    absl::Duration rto_expiration_jitter = absl::ZeroDuration();
    absl::Duration GetRtoExpirationTime() {
      return rto_expiration_base_time + rto_expiration_jitter;
    }
    void ClearRtoExpirationTime() {
      rto_expiration_base_time = rto_expiration_jitter = absl::ZeroDuration();
    }

    explicit TransmitterReliabilityMetadata(int version)
        : request_window_metadata(TransmitterReliabilityWindowMetadata(
              WindowType::kRequest, version)),
          data_window_metadata(TransmitterReliabilityWindowMetadata(
              WindowType::kData, version)) {}
  } tx_reliability_metadata;

  struct ReceiverReliabilityMetadata {
    ReceiverReliabilityWindowMetadata request_window_metadata;
    ReceiverReliabilityWindowMetadata data_window_metadata;
    // Reflects the number of packets that are implicitly ACKed. This occurs
    // when the BPSN of the receiver windows are moved ahead prior to sending
    // out an ACK.
    uint32_t implicitly_acked_counter = 0;
    // Mapping between TransactionKey and context of received packets.
    absl::flat_hash_map<TransactionKey, ReceivedPacketContext>
        received_packet_contexts;
    absl::StatusOr<const ReceivedPacketContext* const> GetReceivedPacketContext(
        const TransactionKey& transaction_key) {
      if (received_packet_contexts.contains(transaction_key)) {
        return &received_packet_contexts[transaction_key];
      } else {
        return absl::NotFoundError("ReceivedPacketContext not found.");
      }
    }
    explicit ReceiverReliabilityMetadata(int version)
        : request_window_metadata(
              ReceiverReliabilityWindowMetadata(WindowType::kRequest, version)),
          data_window_metadata(
              ReceiverReliabilityWindowMetadata(WindowType::kData, version)) {}
  } rx_reliability_metadata;

  // Last time this connection sent out a packet. Whenever this variable is
  // updated in the scheduler(s), the following variable (is_op_rate_limited)
  // must also be updated to ensure scheduler eligibility is up-to-date and
  // maintained correctly. If is_op_rate_limited is set to true (1), there must
  // be an event scheduled to mark it false (0) at the appropriate time.
  absl::Duration last_packet_send_time = -absl::InfiniteDuration();
  GatingVariable<int> is_op_rate_limited;

  // RACK states.
  struct Rack {
    // The most recent ACK's T1.
    absl::Duration xmit_ts = absl::ZeroDuration();
    // RACK RTO.
    absl::Duration rto = absl::InfiniteDuration();
  } rack;

  // TLP states.
  struct Tlp {
    // Probe timeout.
    absl::Duration pto;
  } tlp;

  // ULP-RTO is for received packets to be retransmitted.
  absl::Duration ulp_rto = kDefaultUlpRto;

  // This metadata is used to capture the start time of a cwnd pause.
  absl::Duration cwnd_pause_start_time = absl::ZeroDuration();

  explicit ConnectionState(const ConnectionMetadata& connection_metadata,
                           int version = 1)
      : connection_metadata(connection_metadata),
        tx_reliability_metadata(version),
        rx_reliability_metadata(version) {}

  // Holds the metadata related to transactions corresponding to a
  // connection, keyed by the RSN + role of the host (initiator or target).
  absl::flat_hash_map<TransactionKey, std::unique_ptr<TransactionMetadata>>
      transactions;

  absl::StatusOr<TransactionMetadata*> GetTransaction(
      const TransactionKey& transaction_key) const {
    auto it = transactions.find(transaction_key);
    if (it != transactions.end()) {
      return it->second.get();
    }
    return absl::NotFoundError("Transaction not found.");
  }

  // Creates or updates a transaction as Falcon receives ULP ops. It returns the
  // (RSN, PacketType) corresponding to the packet.
  UlpTransactionInfo CreateOrUpdateTransactionFromUlp(
      std::unique_ptr<Packet> packet, int solicited_write_threshold,
      OrderingMode ordering_mode, FalconInterface* falcon);
  // Creates or updates a transaction as Falcon receives a packet from network.
  // It returns the expected request length (as per the original request) and
  // the request/response length of the received Falcon transaction.
  NetworkTransactionInfo CreateOrUpdateTransactionFromNetwork(
      std::unique_ptr<Packet> packet, FalconInterface* falcon);
};

};  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_CONNECTION_STATE_H_
