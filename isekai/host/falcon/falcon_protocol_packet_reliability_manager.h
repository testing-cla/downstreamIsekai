#ifndef ISEKAI_HOST_FALCON_FALCON_PROTOCOL_PACKET_RELIABILITY_MANAGER_H_
#define ISEKAI_HOST_FALCON_FALCON_PROTOCOL_PACKET_RELIABILITY_MANAGER_H_

#include <sys/types.h>

#include <array>
#include <cstdint>
#include <limits>
#include <random>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/time/time.h"
#include "gtest/gtest_prod.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/token_bucket.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_bitmap.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_protocol_connection_scheduler_types.h"
#include "isekai/host/falcon/falcon_types.h"

namespace isekai {

inline constexpr unsigned kRandomSeed = 1;

struct AckWindowMetadata {
  const uint32_t base_packet_sequence_number;
  const bool own;
  const FalconAckPacketBitmap& window;
  AckWindowMetadata(const uint32_t base_packet_sequence_number, const bool own,
                    const FalconAckPacketBitmap& window)
      : base_packet_sequence_number(base_packet_sequence_number),
        own(own),
        window(window) {}
};

// Helper function to get the correct TX window based on transaction type.
TransmitterReliabilityWindowMetadata* GetAppropriateTxWindow(
    ConnectionState::TransmitterReliabilityMetadata* metadata,
    falcon::PacketType packet_type);
// Helper function to get the correct TX window. This is used during NACK
// processing as they indicate whether the NACK corresponds to request window or
// data window.
TransmitterReliabilityWindowMetadata* GetAppropriateTxWindow(
    ConnectionState::TransmitterReliabilityMetadata* metadata,
    bool request_window);
// Helper function to get the correct RX window based on packet type.
ReceiverReliabilityWindowMetadata* GetAppropriateRxWindow(
    ConnectionState::ReceiverReliabilityMetadata* metadata,
    falcon::PacketType packet_type);

// Responsibilities include implementing sliding window, loss recovery via
// retransmissions and handling/generating (N)ACKs.
class ProtocolPacketReliabilityManager : public PacketReliabilityManager {
 public:
  explicit ProtocolPacketReliabilityManager(FalconModelInterface* falcon);
  // Initializes the per-connection state stored in the packet reliability
  // manager.
  void InitializeConnection(uint32_t scid) override;
  // Transmits the packet picked by the connection scheduler.
  absl::Status TransmitPacket(uint32_t scid, uint32_t rsn,
                              falcon::PacketType type) override;
  // Handles RTO reduction by explicitly checking if packets need to be
  // retransmitted.
  absl::Status HandleRtoReduction(uint32_t scid) override;
  // Handles ACK received from the local ULP. Corresponds to Pull Request, Push
  // Solicited Data and Push Unsolicted Data.
  absl::Status HandleAckFromUlp(uint32_t scid, uint rsn,
                                const OpaqueCookie& cookie) override;
  // Handles NACK received from the local ULP.
  absl::Status HandleNackFromUlp(uint32_t scid,
                                 const TransactionKey& transaction_key,
                                 UlpNackMetadata* nack_metadata,
                                 const OpaqueCookie& cookie) override;
  // Receives an incoming packet and performing the sliding-window related
  // processing.
  absl::Status ReceivePacket(const Packet* packet) override;
  // Verifies if initial transmission meets Tx gating criteria.
  bool MeetsInitialTransmissionCCTxGatingCriteria(
      uint32_t scid, falcon::PacketType type) override;
  // Verifies if initial transmission meets Tx OOW gating criteria.
  bool MeetsInitialTransmissionOowTxGatingCriteria(
      uint32_t scid, falcon::PacketType type) override;
  // Returns the open fcwnd for the given connection and packet type.
  uint32_t GetOpenFcwnd(uint32_t cid, falcon::PacketType type);
  // Verifies if retransmission meets Tx gating criteria.
  bool MeetsRetransmissionCCTxGatingCriteria(uint32_t scid, uint32_t psn,
                                             falcon::PacketType type) override;
  // Testing method to verify AR bit setting policy.
  bool MeetsAckRequestedBitSetCriteriaForTesting(uint32_t fcwnd);

 protected:
  FRIEND_TEST(ProtocolPacketReliabilityManagerTest, RtoTest);
  virtual void DequeuePacketFromRetxScheduler(uint32_t scid, uint32_t rsn,
                                              falcon::PacketType type,
                                              TransactionMetadata* transaction);
  // Chooses the flow label for packet depending on its type.
  virtual uint32_t ChooseOutgoingPacketFlowLabel(
      falcon::PacketType packet_type, uint32_t rsn,
      const ConnectionState* connection_state);
  // Piggybacks ACK information on an outgoing data/request packet.
  virtual void PiggybackAck(uint32_t scid, Packet* packet);
  // Records the input packet as outstanding and changes the connection and
  // transaction state accordingly.
  virtual void AddToOutstandingPackets(ConnectionState* const connection_state,
                                       TransactionMetadata* const transaction,
                                       PacketMetadata* const packet_metadata);
  // Accumulates the number of packets acked to be used for the congestion
  // control algorithm in a RUE event.
  virtual void AccumulateNumAcked(
      ConnectionState* connection_state,
      const OutstandingPacketContext* packet_context);
  // Handles a stale ACK where RX window BPSN in ACK < TX window BPSN.
  virtual absl::Status HandleStaleAck(const Packet* ack_packet);
  // Sets the PSN/SSN of the outgoing packet based on its packet type.
  absl::Status AssignPsnAndSsn(ConnectionState* const connection_state,
                               PacketMetadata* const packet_metadata,
                               Packet* const packet);
  // Sets up retransmission for an outgoing transaction.
  absl::Status SetupRetransmission(ConnectionState* const connection_state,
                                   TransactionMetadata* const transaction,
                                   PacketMetadata* const packet_metadata);
  // Sets up retransmission timer when an appropriate one is not running.
  absl::Status SetupRetransmitTimer(ConnectionState* const connection_state);
  // Handles when an outstanding retransmission timer is triggered.
  void HandleRetransmitTimeout(uint32_t scid);
  // Get the minimum of all timeout values (RACK, RTO, RNR-TO, etc) for an
  // individual packet.
  absl::Duration GetTimeoutOfPacket(ConnectionState* const connection_state,
                                    PacketMetadata* const packet_metadata);
  // Get the packet metadata from rsn and type.
  PacketMetadata* GetPacketMetadata(ConnectionState* const connection_state,
                                    uint32_t rsn, falcon::PacketType type);
  // Get the packet metadata from psn.
  PacketMetadata* GetPacketMetadataFromPsn(
      ConnectionState* const connection_state,
      TransmitterReliabilityWindowMetadata* window, uint32_t psn);
  // Get the packet metadata from work id.
  PacketMetadata* GetPacketMetadataFromWorkId(
      ConnectionState* const connection_state,
      const RetransmissionWorkId& work_id);
  // Enqueues a packet in the retransmission scheduler for retransmission.
  void InitiateRetransmission(uint32_t scid, uint32_t rsn,
                              falcon::PacketType packet_type);
  // Initiates early retransmission as a response to receiving RxWindow NACK.
  absl::Status InitiateNackBasedEarlyRetransmission(const Packet* packet);
  // Handles an incoming ACK packet.
  absl::Status HandleACK(const Packet* packet);
  // Handle acks for request/grant when the corresponding grant/data packet
  // arrives.
  void HandleImplicitAck(const Packet* packet,
                         ConnectionState* const connection_state);
  // Handles piggybacked ACKs on incoming transactions and NACKs.
  void HandlePiggybackedACK(const Packet* packet);
  // Handles E-ACK.
  absl::Status HandleEACK(const Packet* packet, bool& drop_detected);

 private:
  // Determines whether to skip the retransmission during EACK bitmap scanning.
  virtual bool ShouldSkipRetransmission(
      int bitmap_index, const AckWindowMetadata& ack_metadata,
      const TransmitterReliabilityWindowMetadata* tx_metadata,
      const Packet* eack_packet);

 protected:
  // Sets the appropriate headers for an outgoing packet.
  void SetOutgoingPacketHeader(ConnectionState* const connection_state,
                               uint32_t rsn, falcon::PacketType type,
                               Packet* const packet);
  // Performs the sliding window related processing for incoming packet or
  // resync packets.
  virtual absl::Status HandleIncomingPacketOrResync(const Packet* packet);
  // Generates and sends out a resync packet as a response to receiving the
  // corresponding NACK.
  absl::Status GenerateResyncPacket(uint32_t scid, uint32_t original_rsn,
                                    uint32_t original_psn,
                                    falcon::PacketType original_type,
                                    falcon::ResyncCode code);
  // Handles an incoming resync packet.
  absl::Status HandleResyncPacket(const Packet* packet);
  // Handles an incoming duplicate NACK.
  absl::Status HandleDuplicateNackedPacket(
      ConnectionState* const connection_state, const Packet* packet);
  // Handles an incoming NACK.
  absl::Status HandleNack(const Packet* packet);
  // Handles an incoming RNR NACK.
  absl::Status HandleRnrNack(const Packet* rnr_nack);
  // Cancels an outstanding retransmission of the NACKed packet,and trigger
  // sending out a resync packet.
  absl::Status CancelRetransmissionAndTriggerResync(
      const Packet* nack_packet, falcon::ResyncCode resync_code,
      bool generate_completion);
  // Sends an ACK to the ULP via the reorder engine.
  void EnqueueAckToUlp(TransmitterReliabilityWindowMetadata* window,
                       uint32_t scid, uint32_t psn);
  // Creates or updates the receiver packet context.
  void CreateOrUpdateReceiverPacketContext(
      ConnectionState* const connection_state, const Packet* packet,
      PacketDirection direction);
  // Updates the receive ACK bitmap on receiving ACKs from either the network or
  // ULP (if necessary, i.e., in case of push data). The function returns the
  // number of implicitly acked packets after updating the ACK bitmap.
  void UpdateReceiveAckBitmap(
      ConnectionState::ReceiverReliabilityMetadata* const
          rx_reliability_metadata,
      ReceiverReliabilityWindowMetadata* const rx_window_metadata,
      uint32_t psn);
  // Increments outstanding request count when sending out a request.
  void IncrementOutstandingRequestCount(ConnectionState* const connection_state,
                                        falcon::PacketType type);
  // Decrements outstanding request count when an ACK is received.
  virtual void DecrementOutstandingRequestCount(uint32_t scid,
                                                falcon::PacketType type);
  // Increments outstanding retransmission request count when sending out a
  // request.
  void IncrementOutstandingRetransmittedRequestCount(
      ConnectionState* const connection_state, falcon::PacketType type);
  // Decrements outstanding retransmission request count when an ACK is
  // received.
  virtual void DecrementOutstandingRetransmittedRequestCount(
      uint32_t scid, falcon::PacketType type, bool is_acked);

  // Determines if the AR bit set criteria is met or not.
  bool MeetsAckRequestedBitSetCriteria(uint32_t fcwnd);

  absl::Duration GetJitter();
  // Count the number of packets in the outstanding packet list of a connection.
  uint32_t CountOutstandingPackets(ConnectionState* const connection_state);
  void UpdateOutstandingCounterAfterErase(
      ConnectionState* const connection_state);

  // OOO-count algorithm returns the highest index in the bitmap that can be
  // retransmitted, for request and data.
  std::vector<int> EarlyRetransmissionOooCount(
      const std::array<AckWindowMetadata, 2>& ack_received_metadata);
  // OOO-distance algorithm returns the highest index in the bitmap that can be
  // retransmitted, for request and data.
  std::vector<int> EarlyRetransmissionOooDistance(
      const std::array<AckWindowMetadata, 2>& ack_received_metadata);
  // Merge the retx_limit returned by different algorithm.
  void EarlyRetransmissionMergeRetxLimit(std::vector<int>& retx_limit,
                                         const std::vector<int>& new_limit);
  bool EackBasedEarlyRetransmissionIsEnabled() const;
  // Update RACK.
  void RackUpdate(ConnectionState* const connection_state,
                  absl::Duration tx_time);
  // Update RACK based on NACK.
  void RackUpdateByNack(ConnectionState* const connection_state,
                        const Packet* packet);
  // Calculate RACK's reorder window.
  absl::Duration CalculateRackWindow(ConnectionState* const connection_state);
  // Reset TLP timer.
  void SetupTlpTimer(ConnectionState* const connection_state);
  // Calculate TLP's timeout.
  absl::Duration CalculateTlpTimeout(ConnectionState* const connection_state);
  // Handle TLP timeout.
  void HandleTlpTimeout(ConnectionState* const connection_state);
  // TlpFirst: retransmit the first un-received packet as probe. Returns the
  // index of the sliding window (0 or 1) and RetransmissionWorkId to retx. If
  // no packet to send, returns <0,nullptr>.
  std::pair<int, const RetransmissionWorkId*> TlpRetransmitFirst(
      ConnectionState* const connection_state,
      const std::array<TransmitterReliabilityWindowMetadata*, 2>& tx_windows);
  // TlpLast: retransmit the last un-received packet as probe. Returns the
  // index of the sliding window (0 or 1) and RetransmissionWorkId to retx. If
  // no packet to send, returns <0,nullptr>.
  std::pair<int, const RetransmissionWorkId*> TlpRetransmitLast(
      ConnectionState* const connection_state,
      const std::array<TransmitterReliabilityWindowMetadata*, 2>& tx_windows);
  // If RACK is enabled, or TLP not retransmit first unacked packet, received
  // states are required to maintain at the sender.
  bool NeedToMaintainReceivedStateAtSender() const;
  bool IsReceived(PacketMetadata* packet_metadata) const;

  FalconModelInterface* const falcon_;
  TokenBucket ack_token_bucket_;
  std::mt19937 random_number_generator_;
  std::uniform_int_distribution<int32_t> ar_bit_set_generator_;
  uint8_t next_ar_ = 0;
  uint8_t passed_packets_for_ar_ = 0;

  // Variables for calculating jitter of retransmission scan event. These are
  // for modeling jitter observed in actual DNA with fidelity. Jitter is
  // calculated as: random(0,retx_jitter_base_range_ns_) +
  // retx_jitter_conn_factor_ns_ * outstanding_conn_counter_ +
  // retx_jitter_pkt_factor_ns_ * outstanding_pkt_counter_
  const uint32_t retx_jitter_base_range_ns_ = 0;
  const uint32_t retx_jitter_conn_factor_ns_ = 0;
  const uint32_t retx_jitter_pkt_factor_ns_ = 0;
  uint32_t outstanding_conn_counter_ = 0;
  uint32_t outstanding_pkt_counter_ = 0;

  // Early retransmission parameters.
  // Early-retx enablers.
  bool enable_ooo_count_ = false;
  bool enable_ooo_distance_ = true;
  bool enable_eack_own_ = false;
  bool enable_recency_check_bypass_ = false;
  bool enable_scanning_exit_criteria_bypass_ = false;
  bool enable_smaller_psn_recency_check_bypass_ = false;
  bool enable_pause_initial_transmission_on_oow_drops_ = false;
  uint32_t request_window_slack_ = 0;
  uint32_t data_window_slack_ = 0;

  bool enable_rack_ = false;
  bool enable_tlp_ = false;
  // Limit of per-packet early retx times.
  uint32_t early_retx_threshold_ = std::numeric_limits<uint32_t>::max();
  uint32_t ooo_distance_threshold_ = 3;
  uint32_t ooo_count_threshold_ = 3;
  // RACK reorder window is calculated using rtt * this factor.
  double rack_time_window_rtt_factor_ = 0.25;
  // RACK's minimum reorder window.
  absl::Duration min_rack_time_window_ = absl::ZeroDuration();
  // RACK uses T1 as the most recent received packet's Tx time or not.
  bool rack_use_t1_ = true;
  // TLP's minimum PTO.
  absl::Duration min_tlp_timeout_ = absl::Microseconds(50);
  // TLP's PTO is calculated using rtt * this factor.
  double tlp_timeout_rtt_factor_ = 2.0;
  // TLP retransmits which packet.
  FalconConfig::EarlyRetx::TlpType tlp_type_ =
      FalconConfig::EarlyRetx::FIRST_UNACKED;
  // TLP packet bypass cwnds limit or not.
  bool tlp_bypass_cc_ = false;
};

};  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_PROTOCOL_PACKET_RELIABILITY_MANAGER_H_
