#ifndef ISEKAI_HOST_RDMA_RDMA_ROCE_MODEL_H_
#define ISEKAI_HOST_RDMA_RDMA_ROCE_MODEL_H_

#include <cstdint>
#include <memory>
#include <utility>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/time/time.h"
#include "gtest/gtest_prod.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/host/rdma/rdma_base_model.h"
#include "isekai/host/rdma/rdma_component_interfaces.h"
#include "isekai/host/rdma/rdma_configuration.h"

namespace isekai {

class RdmaRoceModel : public RdmaBaseModel<RoceQpContext>,
                      public RoceInterface {
 public:
  RdmaRoceModel(const RdmaConfig& rdma_config, const RoceConfig& roce_config,
                uint32_t mtu_size, Environment* env,
                StatisticCollectionInterface* stats_collector,
                ConnectionManagerInterface* connection_manager)
      : RdmaBaseModel(RdmaType::kRoceRdma, rdma_config, mtu_size, env,
                      stats_collector, connection_manager),
        outer_header_size_(roce_config.outer_header_size()) {}

  void SetupCc(RdmaCcOptions* options);
  void ConnectPacketBuilder(PacketBuilderInterface* packet_builder) override {
    packet_builder_ = packet_builder;
  }
  void CreateRcQp(QpId local_qp_id, QpId remote_qp_id, QpOptions& options,
                  RdmaConnectedMode rc_mode) override;
  void SetPfcXoff(bool xoff) override;
  void TransferRxPacket(std::unique_ptr<Packet> packet, bool ecn) override;
  uint32_t GetHeaderSize(Packet::Roce::Opcode opcode) override;

  static const uint32_t kValidPsnRange = 8388608;
  static const uint32_t kPsnRange = 16777216;
  static uint32_t PsnAdd(uint32_t a, uint32_t b) {
    return (a + b + kPsnRange) % kPsnRange;
  }
  static uint32_t PsnSubtract(uint32_t a, uint32_t b) {
    return (a - b + kPsnRange) % kPsnRange;
  }
  // Checks if a is between [b,c] in PSN space.
  static bool PsnBetween(uint32_t a, uint32_t b, uint32_t c) {
    return PsnSubtract(a, b) + PsnSubtract(c, a) == PsnSubtract(c, b);
  }
  static const uint32_t kBthHeaderSize = 12;
  static const uint32_t kRethHeaderSize = 16;
  static const uint32_t kAethHeaderSize = 4;

 protected:
  FRIEND_TEST(RdmaRoceCcDcqcnTest, RateUpdate);
  std::unique_ptr<RdmaRoceCcInterface> cc_ = nullptr;
  // Pointer to the packet builder so that traffic shaper could push the
  // ready-to-depart packet to the packet builder.
  PacketBuilderInterface* packet_builder_ = nullptr;
  // The header size outside RoCE.
  uint32_t outer_header_size_ = 0;
  bool pfc_xoff_ = false;

  void InitializeQpContext(BaseQpContext* base_context, QpId local_qp_id,
                           QpOptions& options) override;

  // WorkScheduler state.
  // Stores the status of each WorkQueue whether it has pending work or not.
  absl::flat_hash_map<WorkQueue, bool> is_work_queue_active_;
  // Ordered set of WorkQueues that have been scheduled by the work scheduler,
  // in order of next_send_time.
  absl::btree_set<std::pair<absl::Duration, WorkQueue>> scheduled_work_;
  // Last time when the work scheduler ran and transmitted a packet.
  absl::Duration last_scheduler_run_time_ = absl::ZeroDuration();

  void AddWorkQueueToActiveSet(WorkQueue work_queue, RoceQpContext* context);
  void RunWorkScheduler();

  void PostOp(RoceQpContext* qp_context, RdmaOp op) override;
  void TransmitPacket(WorkQueue work_queue_id, RoceQpContext* context);
  void ProcessRxPacket(RoceQpContext* context, Packet p, bool ecn);
  void GenerateCongestionNotification(RoceQpContext* context);
  void ProcessRxAck(RoceQpContext* context, const Packet& p, bool& reset_rto);
  // Complete ops up to psn.
  bool CompleteOpTillPsn(RoceQpContext* context, uint32_t psn);
  // Complete the op at the head of send queue.
  void CompleteOpAtHead(RoceQpContext* context);
  std::unique_ptr<Packet> CreateRequestPacket(RoceQpContext* context);
  std::unique_ptr<Packet> CreateResponsePacket(RoceQpContext* context);
  // Checks if `psn` is valid (including duplicate) at Target.
  bool PsnIsValidAtTarget(RoceQpContext* context, uint32_t psn);
  // Checks if `psn` is duplicate at Target.
  bool PsnIsDuplicateAtTarget(RoceQpContext* context, uint32_t psn);
  // Checks `psn1` is logically < `psn2` at Target.
  bool PsnSmallerAtTarget(RoceQpContext* context, uint32_t psn1, uint32_t psn2);
  // Checks `psn1` is logically >= `psn2` at Initiator.
  bool PsnGeAtInitiator(RoceQpContext* context, uint32_t psn1, uint32_t psn2);
  // Checks if `psn` is valid: between oldest_unacknowledged_psn and max_psn.
  bool PsnIsValidAtInitiator(RoceQpContext* context, uint32_t psn);
  // Generate and transmit an ACK to the lower layer.
  void TransmitAck(RoceQpContext* context);
  // Handle the ACK coalescing timeout.
  void HandleAckCoalescingTimeout(QpId qp_id);
  // Trigger immediate ACK if count threshold met, otherwise start timer.
  void GenerateAckOrUpdateCoalescingState(RoceQpContext* context,
                                          bool ack_requested);
  // Set up timer for ACK coalescing.
  void SetupAckCoalescingTimer(RoceQpContext* context);
  // Generates and transmit a NACK-sequence-error.
  void TransmitNakSequenceError(RoceQpContext* context);
  // Set up timer for RTO.
  void SetupRto(RoceQpContext* context);
  // Cancel timer of RTO.
  void CancelRto(RoceQpContext* context);
  // Handle RTO.
  void HandleRto(QpId qp_id);
};

};  // namespace isekai

#endif  // ISEKAI_HOST_RDMA_RDMA_ROCE_MODEL_H_
