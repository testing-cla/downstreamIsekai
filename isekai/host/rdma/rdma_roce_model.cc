#include "isekai/host/rdma/rdma_roce_model.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <memory>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/meta/type_traits.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "glog/logging.h"
#include "isekai/common/common_util.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/status_util.h"
#include "isekai/host/rdma/rdma_base_model.h"
#include "isekai/host/rdma/rdma_component_interfaces.h"
#include "isekai/host/rdma/rdma_configuration.h"
#include "isekai/host/rdma/rdma_qp_manager.h"
#include "isekai/host/rdma/rdma_roce_cc_dcqcn.h"

namespace isekai {

void RdmaRoceModel::SetupCc(RdmaCcOptions* options) {
  switch (options->type) {
    case RdmaCcOptions::Type::kDcqcn:
      cc_ = std::make_unique<RdmaRoceCcDcqcn>(
          env_, &qp_manager_, down_cast<RdmaCcDcqcnOptions*>(options));
      break;
    case RdmaCcOptions::Type::kNone:
      break;
  }
}

void RdmaRoceModel::CreateRcQp(QpId local_qp_id, QpId remote_qp_id,
                               QpOptions& options, RdmaConnectedMode rc_mode) {
  // Create the qp context, and initialize it.
  auto context = std::make_unique<RoceQpContext>();
  InitializeQpContext(context.get(), local_qp_id, options);
  // Add the context to the qp manager.
  qp_manager_.CreateQp(std::move(context));
  qp_manager_.ConnectQp(local_qp_id, remote_qp_id, rc_mode);
  is_work_queue_active_[WorkQueue(local_qp_id, WorkType::kRequest)] = false;
  is_work_queue_active_[WorkQueue(local_qp_id, WorkType::kResponse)] = false;
}

void RdmaRoceModel::InitializeQpContext(BaseQpContext* base_context,
                                        QpId local_qp_id, QpOptions& options) {
  RdmaBaseModel::InitializeQpContext(base_context, local_qp_id, options);
  auto context = down_cast<RoceQpContext*>(base_context);
  context->ack_coalescing_threshold = options.ack_coalescing_threshold;
  context->ack_coalescing_timeout = options.ack_coalesing_timeout;
  context->retx_timeout = options.retx_timeout;
  cc_->InitContext(context);
}

void RdmaRoceModel::SetPfcXoff(bool xoff) {
  pfc_xoff_ = xoff;
  // If PFC was de-asserted (i.e., we are now unpaused), and if there is pending
  // work, we run the work scheduler.
  if (!pfc_xoff_ && !scheduled_work_.empty()) {
    RunWorkScheduler();
  }
}

void RdmaRoceModel::TransferRxPacket(std::unique_ptr<Packet> packet, bool ecn) {
  Packet p = *packet;
  qp_manager_.InitiateQpLookup(
      packet->roce.dest_qp_id,
      [this, p, ecn](absl::StatusOr<BaseQpContext*> qp_context) {
        CHECK_OK(qp_context.status())
            << "Received packet destined for unknown qp id: "
            << qp_context.status() << "\n"
            << p.DebugString();
        ProcessRxPacket(down_cast<RoceQpContext*>(qp_context.value()), p, ecn);
      });
}

uint32_t RdmaRoceModel::GetHeaderSize(Packet::Roce::Opcode opcode) {
  switch (opcode) {
    case Packet::Roce::Opcode::kSendFirst:
    case Packet::Roce::Opcode::kSendMiddle:
    case Packet::Roce::Opcode::kSendLast:
    case Packet::Roce::Opcode::kSendOnly:
      return kBthHeaderSize;
    case Packet::Roce::Opcode::kWriteFirst:
    case Packet::Roce::Opcode::kWriteOnly:
      return kBthHeaderSize + kRethHeaderSize;
    case Packet::Roce::Opcode::kWriteMiddle:
    case Packet::Roce::Opcode::kWriteLast:
      return kBthHeaderSize;
    case Packet::Roce::Opcode::kReadRequest:
      return kBthHeaderSize + kRethHeaderSize;
    case Packet::Roce::Opcode::kReadResponseFirst:
    case Packet::Roce::Opcode::kReadResponseLast:
    case Packet::Roce::Opcode::kReadResponseOnly:
      return kBthHeaderSize + kAethHeaderSize;
    case Packet::Roce::Opcode::kReadResponseMiddle:
      return kBthHeaderSize;
    case Packet::Roce::Opcode::kAck:
      return kBthHeaderSize + kAethHeaderSize;
    case Packet::Roce::Opcode::kCongestionNotification:
      return kBthHeaderSize;
    default:
      LOG(FATAL) << "Packet type not implemented.";
  }
}

void RdmaRoceModel::ProcessRxPacket(RoceQpContext* context, Packet p,
                                    bool ecn) {
  if (ecn) {
    GenerateCongestionNotification(context);
  }

  bool reset_rto = false;
  if (cc_) cc_->HandleRxPacket(context, &p);
  switch (p.roce.opcode) {
    case Packet::Roce::Opcode::kSendFirst:
      break;
    case Packet::Roce::Opcode::kSendMiddle:
      break;
    case Packet::Roce::Opcode::kSendLast:
      break;
    case Packet::Roce::Opcode::kSendOnly:
      break;
    case Packet::Roce::Opcode::kWriteFirst:
    case Packet::Roce::Opcode::kWriteMiddle:
    case Packet::Roce::Opcode::kWriteLast:
    case Packet::Roce::Opcode::kWriteOnly:
      if (PsnIsValidAtTarget(context, p.roce.psn)) {
        if (p.roce.psn == context->expected_psn) {
          context->expected_psn = PsnAdd(context->expected_psn, 1);
          GenerateAckOrUpdateCoalescingState(context, p.roce.ack_req);
          // Permit future NAK-sequence-error (C9-114 in InfiniBand Spec).
          context->nak_sequence_error_sent = false;
        } else {
          // Transmit an ACK for duplicate SEND or WRITE (C9-105 in InfiniBand
          // spec).
          TransmitAck(context);
        }
      } else {
        // If NAK-sequence-error was not sent, generate and send one.
        if (!context->nak_sequence_error_sent) {
          TransmitNakSequenceError(context);
        }
      }
      break;
    case Packet::Roce::Opcode::kReadRequest: {
      auto& irrq = context->inbound_read_request_queue;
      const uint32_t n_pkt =
          p.roce.request_length == 0
              ? 1
              : (1 + (p.roce.request_length - 1) / rdma_mtu_);
      if (PsnIsValidAtTarget(context, p.roce.psn)) {
        if (p.roce.psn == context->expected_psn) {
          // If there is no outstanding READ and ACK coalescing is ongoing, stop
          // ACK coalescing and send an ACK immediately.
          if (irrq.empty() && context->ack_coalescing_counter > 0) {
            TransmitAck(context);
          }
          context->expected_psn = PsnAdd(context->expected_psn, n_pkt);
          // Permit future NAK-sequence-error (C9-114 in InfiniBand Spec).
          context->nak_sequence_error_sent = false;
        } else {
          while (!irrq.empty() &&
                 PsnSmallerAtTarget(context, p.roce.psn, irrq.back().psn))
            irrq.pop_back();
        }
        irrq.emplace_back(true, p.roce.psn, PsnAdd(p.roce.psn, n_pkt - 1),
                          p.roce.request_length);
        if (irrq.size() == 1) context->irrq_psn = p.roce.psn;
        // An op was enqueued to IRRQ. Add WorkQueue in active set.
        AddWorkQueueToActiveSet(WorkQueue(context->qp_id, WorkType::kResponse),
                                context);
      } else {
        // If NAK-sequence-error was not sent, generate and send one.
        if (!context->nak_sequence_error_sent) {
          TransmitNakSequenceError(context);
        }
      }
      break;
    }
    case Packet::Roce::Opcode::kReadResponseFirst:
    case Packet::Roce::Opcode::kReadResponseOnly:
      // Implicitly acknowledge ops before the READ.
      reset_rto |= CompleteOpTillPsn(context, PsnAdd(p.roce.psn, -1));
      // Fall through to response processing below.
      ABSL_FALLTHROUGH_INTENDED;
    case Packet::Roce::Opcode::kReadResponseMiddle:
    case Packet::Roce::Opcode::kReadResponseLast:
      if (p.roce.psn == context->oldest_unacknowledged_psn) {
        CHECK(context->send_queue.front()->opcode == RdmaOpcode::kRead);
        context->oldest_unacknowledged_psn =
            PsnAdd(context->oldest_unacknowledged_psn, 1);
        reset_rto = true;
        if (p.roce.psn == context->send_queue.front()->end_psn)
          CompleteOpAtHead(context);
        // Permit future read retry.
        context->read_retried = false;
      } else if (PsnIsValidAtInitiator(context, p.roce.psn)) {
        // If READ was not retried, retry.
        if (!context->read_retried) {
          context->read_retried = true;
          // Out-of-order READ responses. Go-back-N retry.
          context->next_psn = context->oldest_unacknowledged_psn;
          context->next_op_index = 0;
          // next_op_index has changed, add WorkQueue to active set.
          AddWorkQueueToActiveSet(WorkQueue(context->qp_id, WorkType::kRequest),
                                  context);
          context->highest_psn_with_ack_req =
              PsnAdd(context->oldest_unacknowledged_psn, -1);
          CancelRto(context);
        }
      } else {
        // Ignore invalid READ responses.
        LOG(INFO) << "Invalid READ response, psn=" << p.roce.psn;
      }
      break;
    case Packet::Roce::Opcode::kAck:
      ProcessRxAck(context, p, reset_rto);
      break;
    default:
      break;
  }
  if (reset_rto) {
    context->retry_counter = 0;
    CancelRto(context);
    // If there are still unacknowledged packets with ack_req, reset RTO.
    if (PsnIsValidAtInitiator(context, context->highest_psn_with_ack_req)) {
      SetupRto(context);
    }
  }
}

void RdmaRoceModel::GenerateCongestionNotification(RoceQpContext* context) {
  auto packet = std::make_unique<Packet>();
  packet->metadata.destination_ip_address = context->dst_ip;
  packet->roce.is_roce = true;
  packet->roce.opcode = Packet::Roce::Opcode::kCongestionNotification;
  packet->roce.psn = 0;
  packet->roce.dest_qp_id = context->dest_qp_id;
  // For stats collection.
  packet->roce.qp_id = context->qp_id;
  // Send packet out.
  packet_builder_->EnqueuePacket(std::move(packet));
}

void RdmaRoceModel::ProcessRxAck(RoceQpContext* context, const Packet& p,
                                 bool& reset_rto) {
  // If psn is invalid, skip.
  if (!PsnIsValidAtInitiator(context, p.roce.psn)) return;
  bool nak_seq =
      (p.roce.ack.type == Packet::Roce::Ack::Type::kNak &&
       p.roce.ack.nak_type == Packet::Roce::Ack::NakType::kPsnSequenceError);
  // ACK or NAK-sequence
  if (p.roce.ack.type == Packet::Roce::Ack::Type::kAck || nak_seq) {
    reset_rto = CompleteOpTillPsn(context, p.roce.psn);
    // After completing ops and updating oldest_unacknowledged_psn, if psn
    // is still valid, acknowledge up to psn.
    if (PsnIsValidAtInitiator(context, p.roce.psn)) {
      CHECK(!context->send_queue.empty())
          << "A valid PSN indicates outstanding requests, so send_queue "
             "cannot be empty.";
      auto& op = *(context->send_queue.front());
      if (op.opcode == RdmaOpcode::kRead) {
        CHECK(!PsnBetween(p.roce.psn, op.psn, op.end_psn) ||
              p.roce.psn == op.end_psn)
            << "ACK cannot be in the middle of READ.";
        // If psn is not READ's end_psn, it must be larger (possibility of
        // smaller is ruled out), so NAK.
        nak_seq |= (p.roce.psn != op.end_psn);
      } else {
        context->oldest_unacknowledged_psn = PsnAdd(p.roce.psn, 1);
        reset_rto = true;
      }
    }
    if (nak_seq) {
      context->next_psn = context->oldest_unacknowledged_psn;
      context->next_op_index = 0;
      // next_op_index has changed, add WorkQueue to active set.
      AddWorkQueueToActiveSet(WorkQueue(context->qp_id, WorkType::kRequest),
                              context);
      context->highest_psn_with_ack_req =
          PsnAdd(context->oldest_unacknowledged_psn, -1);
      CancelRto(context);
    }
  } else {
    //
  }
}

bool RdmaRoceModel::CompleteOpTillPsn(RoceQpContext* context, uint32_t psn) {
  bool op_completed = false;
  auto& send_queue = context->send_queue;
  // Complete all ops up to psn.
  while (!send_queue.empty() &&
         send_queue.front()->opcode != RdmaOpcode::kRead &&
         PsnIsValidAtInitiator(context, send_queue.front()->end_psn) &&
         PsnGeAtInitiator(context, psn, send_queue.front()->end_psn)) {
    context->oldest_unacknowledged_psn = PsnAdd(send_queue.front()->end_psn, 1);
    CompleteOpAtHead(context);
    op_completed = true;
  }
  return op_completed;
}

void RdmaRoceModel::CompleteOpAtHead(RoceQpContext* context) {
  RdmaOp& op = *(context->send_queue.front());
  op.stats.completion_timestamp = env_->ElapsedTime();
  CollectVectorStats("rdma.op_post_time",
                     absl::ToDoubleSeconds(op.stats.post_timestamp));
  CollectVectorStats("rdma.op_latency_app_level",
                     absl::ToDoubleSeconds(op.stats.completion_timestamp -
                                           op.stats.post_timestamp));
  CollectVectorStats("rdma.op_latency_transport_level",
                     absl::ToDoubleSeconds(op.stats.completion_timestamp -
                                           op.stats.start_timestamp));
  CollectVectorStats("rdma.op_qp_id", context->qp_id);
  CollectVectorStats("rdma.op_code", static_cast<double>(op.opcode));
  CollectVectorStats("rdma.op_length", op.length);
  CollectVectorStats("rdma.op_psn", op.psn);
  CollectVectorStats("rdma.op_end_psn", op.end_psn);

  // Complete the op.
  op.completion_callback(Packet::Syndrome::kAck);
  context->send_queue.pop_front();
  // The op index should -1 as the queue shifts.
  context->next_op_index--;
}

void RdmaRoceModel::PostOp(RoceQpContext* context, RdmaOp op) {
  if (context->is_connected()) {
    op.dest_qp_id = context->dest_qp_id;
  } else {
    CHECK(op.opcode != RdmaOpcode::kSend && op.opcode != RdmaOpcode::kRecv)
        << "Unsupported verb on UD QP.";
  }

  op.stats.post_timestamp = env_->ElapsedTime();

  if (op.opcode == RdmaOpcode::kRecv) {
    context->receive_queue.push_back(std::make_unique<RdmaOp>(op));
  } else {
    op.psn = context->tail_psn;
    op.end_psn =
        PsnAdd(op.psn, (op.length == 0 ? 0 : op.length - 1) / rdma_mtu_);
    context->tail_psn = PsnAdd(op.end_psn, 1);
    context->send_queue.push_back(std::make_unique<RdmaOp>(op));

    // An op was added to send_queue. Add this WorkQueue to active set.
    AddWorkQueueToActiveSet(WorkQueue(context->qp_id, WorkType::kRequest),
                            context);
  }
}

void RdmaRoceModel::AddWorkQueueToActiveSet(WorkQueue work_queue,
                                            RoceQpContext* context) {
  // Confirm this WorkQueue has work available, else return.
  if (context->next_op_index >= context->send_queue.size() &&
      context->inbound_read_request_queue.empty()) {
    return;
  }
  bool was_empty = scheduled_work_.empty();
  // Only schedule the WorkQueue if it wasn't active previously.
  auto it = is_work_queue_active_.find(work_queue);
  if (it == is_work_queue_active_.end() || it->second == false) {
    absl::Duration next_send_time =
        std::max(env_->ElapsedTime(), context->next_send_time);
    scheduled_work_.insert(std::make_pair(next_send_time, work_queue));
    it->second = true;
  }
  // If the scheduler was sitting idle, restart it.
  if (was_empty) {
    absl::Duration next_run_time =
        std::max(last_scheduler_run_time_ +
                     absl::Nanoseconds(config_.chip_cycle_time_ns()),
                 env_->ElapsedTime());
    CHECK_OK(env_->ScheduleEvent(next_run_time - env_->ElapsedTime(),
                                 [this]() { RunWorkScheduler(); }));
  }
}

void RdmaRoceModel::RunWorkScheduler() {
  if (pfc_xoff_ || scheduled_work_.empty()) {
    // Cannot schedule anything because PFC is asserted or no work is available.
    return;
  }

  // Process the WorkQueue at the head of the active list.
  absl::Duration now = env_->ElapsedTime();
  absl::Duration next_send_time = scheduled_work_.begin()->first;
  WorkQueue work_queue = scheduled_work_.begin()->second;

  // Check if we can send out a packet from this WorkQueue right now. If not,
  // reschedule to run at the next earliest send_time.
  if (next_send_time > now) {
    CHECK_OK(env_->ScheduleEvent(next_send_time - now,
                                 [this]() { RunWorkScheduler(); }));
    return;
  }

  // We can send out a packet, so transmit it.
  last_scheduler_run_time_ = now;
  RoceQpContext* context =
      down_cast<RoceQpContext*>(qp_manager_.DirectQpLookup(work_queue.qp_id));
  TransmitPacket(work_queue, context);

  // Check if this WorkQueue has any more work remaining.
  bool has_more_work = true;
  switch (work_queue.work_type) {
    case WorkType::kRequest:
      if (context->next_op_index >= context->send_queue.size()) {
        has_more_work = false;
      }
      break;
    case WorkType::kResponse:
      if (context->inbound_read_request_queue.empty()) {
        has_more_work = false;
      }
      break;
  }

  // Remove the WorkQueue from the head of active WorkQueues. But if it has more
  // work, re-insert into the active WorkQueues (with a new next_send_time).
  // Else mark it as inactive and do not insert it back.
  scheduled_work_.erase(scheduled_work_.begin());
  if (has_more_work) {
    // TransmitPacket() above updates context->next_send_time.
    scheduled_work_.insert(std::make_pair(context->next_send_time, work_queue));
  } else {
    is_work_queue_active_[work_queue] = false;
  }

  // If no more work to do, return.
  if (scheduled_work_.empty()) {
    return;
  }

  // Calculate when to run the work scheduler again.
  absl::Duration next_run_scheduler_delta;
  next_send_time = scheduled_work_.begin()->first;
  if (next_send_time >= now) {
    // Next scheduled WorkQueue send is in the future, so schedule it at that
    // time (with at least chip_cycle_time in between).
    next_run_scheduler_delta =
        std::max(next_send_time - last_scheduler_run_time_,
                 absl::Nanoseconds(config_.chip_cycle_time_ns()));
  } else {
    // Next send time is in the past, which means we are overdue. This is
    // possible due to overload or PFC pauses. Schedule after chip_cycle_time.
    next_run_scheduler_delta = absl::Nanoseconds(config_.chip_cycle_time_ns());
  }

  CHECK_OK(env_->ScheduleEvent(next_run_scheduler_delta,
                               [this]() { RunWorkScheduler(); }));
}

void RdmaRoceModel::TransmitPacket(WorkQueue work_queue_id,
                                   RoceQpContext* context) {
  std::unique_ptr<Packet> packet;

  switch (work_queue_id.work_type) {
    case WorkType::kRequest:
      packet = CreateRequestPacket(context);
      break;
    case WorkType::kResponse:
      packet = CreateResponsePacket(context);
      break;
  }

  uint32_t packet_size = packet->roce.payload_length +
                         GetHeaderSize(packet->roce.opcode) +
                         outer_header_size_;
  context->next_send_time =
      env_->ElapsedTime() +
      absl::Nanoseconds(packet_size * 8 * 1000 / context->rate);
  if (cc_) cc_->HandleTxPacket(context, packet.get());

  // For stats collection.
  packet->roce.qp_id = context->qp_id;

  // Send packet out.
  packet_builder_->EnqueuePacket(std::move(packet));

  if (work_queue_id.work_type == WorkType::kRequest &&
      context->rto_trigger_time == absl::ZeroDuration() &&
      PsnIsValidAtInitiator(context, context->highest_psn_with_ack_req)) {
    SetupRto(context);
  }

  // If IRRQ becomes empty, also check if ACK/NAK should be sent.
  if (work_queue_id.work_type == WorkType::kResponse &&
      context->inbound_read_request_queue.empty()) {
    // If NAK-sequence-error is triggered, send it.
    if (context->nak_sequence_error_triggered) {
      TransmitNakSequenceError(context);
      // Since NAK-sequence-error delivers a superset of info than ACK, no need
      // to send ACK.
      context->ack_triggered = false;
    }
    // If ACK is triggered, send it.
    if (context->ack_triggered) {
      TransmitAck(context);
    }
  }
}

std::unique_ptr<Packet> RdmaRoceModel::CreateRequestPacket(
    RoceQpContext* context) {
  auto packet = std::make_unique<Packet>();
  packet->metadata.destination_ip_address = context->dst_ip;
  packet->roce.is_roce = true;
  RdmaOp& op = *(context->send_queue[context->next_op_index]);
  // Set opcode.
  switch (op.opcode) {
    case RdmaOpcode::kRecv:
      LOG(FATAL) << "Cannot schedule recv op to be sent";
      break;
    case RdmaOpcode::kRead:
      packet->roce.opcode = Packet::Roce::Opcode::kReadRequest;
      // If next PSN is not op.psn, this needs a retry with shorter length.
      packet->roce.request_length =
          op.length - rdma_mtu_ * PsnSubtract(context->next_psn, op.psn);
      break;
    case RdmaOpcode::kSend:
      if (op.psn == op.end_psn)
        packet->roce.opcode = Packet::Roce::Opcode::kSendOnly;
      else if (context->next_psn == op.psn)
        packet->roce.opcode = Packet::Roce::Opcode::kSendFirst;
      else if (context->next_psn == op.end_psn)
        packet->roce.opcode = Packet::Roce::Opcode::kSendLast;
      else
        packet->roce.opcode = Packet::Roce::Opcode::kSendMiddle;
      break;
    case RdmaOpcode::kWrite:
      if (op.psn == op.end_psn)
        packet->roce.opcode = Packet::Roce::Opcode::kWriteOnly;
      else if (context->next_psn == op.psn)
        packet->roce.opcode = Packet::Roce::Opcode::kWriteFirst;
      else if (context->next_psn == op.end_psn)
        packet->roce.opcode = Packet::Roce::Opcode::kWriteLast;
      else
        packet->roce.opcode = Packet::Roce::Opcode::kWriteMiddle;
      break;
  }

  // Set payload length.
  if (op.opcode == RdmaOpcode::kSend || op.opcode == RdmaOpcode::kWrite) {
    if (op.psn == op.end_psn)
      packet->roce.payload_length = op.length;
    else if (context->next_psn == op.end_psn)
      packet->roce.payload_length = op.length % rdma_mtu_;
    else
      packet->roce.payload_length = rdma_mtu_;
  }

  // Set packet headers.
  packet->roce.psn = context->next_psn;
  packet->roce.dest_qp_id = context->dest_qp_id;

  // IRN paper (sec 5.2) suggests per-packet ack_req can run in RoCE. Decide
  // later whether we want to try other policies.
  if (true) {
    packet->roce.ack_req = 1;
    context->highest_psn_with_ack_req = context->next_psn;
  }

  // Update reliabiltiy states.
  if (op.opcode == RdmaOpcode::kRead) {
    if (context->max_psn == context->next_psn)
      context->max_psn = PsnAdd(op.end_psn, 1);
    context->next_op_index++;
    context->next_psn = PsnAdd(op.end_psn, 1);
  } else {
    if (context->max_psn == context->next_psn) {
      context->max_psn = PsnAdd(context->max_psn, 1);
    }
    if (context->next_psn == op.end_psn) {
      context->next_op_index++;
    }
    context->next_psn = PsnAdd(context->next_psn, 1);
  }

  // Record the start time of op.
  if (op.stats.start_timestamp == absl::ZeroDuration())
    op.stats.start_timestamp = env_->ElapsedTime();

  return packet;
}

std::unique_ptr<Packet> RdmaRoceModel::CreateResponsePacket(
    RoceQpContext* context) {
  CHECK(context->is_connected()) << "Read response works in RC mode only.";
  auto packet = std::make_unique<Packet>();
  packet->metadata.destination_ip_address = context->dst_ip;
  packet->roce.is_roce = true;

  const InboundReadRequest& request =
      context->inbound_read_request_queue.front();

  // Set opcode.
  if (request.psn == request.end_psn)
    packet->roce.opcode = Packet::Roce::Opcode::kReadResponseOnly;
  else if (context->irrq_psn == request.psn)
    packet->roce.opcode = Packet::Roce::Opcode::kReadResponseFirst;
  else if (context->irrq_psn == request.end_psn)
    packet->roce.opcode = Packet::Roce::Opcode::kReadResponseLast;
  else
    packet->roce.opcode = Packet::Roce::Opcode::kReadResponseMiddle;

  // Set payload length.
  if (request.psn == request.end_psn)
    packet->roce.payload_length = request.length;
  else if (context->irrq_psn == request.end_psn)
    packet->roce.payload_length = request.length % rdma_mtu_;
  else
    packet->roce.payload_length = rdma_mtu_;

  // Set packet psn.
  packet->roce.psn = context->irrq_psn;

  // Update states.
  if (context->irrq_psn == request.end_psn) {
    context->inbound_read_request_queue.pop_front();
    if (!context->inbound_read_request_queue.empty())
      context->irrq_psn = context->inbound_read_request_queue.front().psn;
  } else {
    context->irrq_psn = PsnAdd(context->irrq_psn, 1);
  }

  packet->roce.dest_qp_id = context->dest_qp_id;
  return packet;
}

bool RdmaRoceModel::PsnIsValidAtTarget(RoceQpContext* context, uint32_t psn) {
  return (context->expected_psn - psn + kPsnRange) % kPsnRange < kValidPsnRange;
}

bool RdmaRoceModel::PsnIsDuplicateAtTarget(RoceQpContext* context,
                                           uint32_t psn) {
  return PsnIsValidAtTarget(context, psn) && psn != context->expected_psn;
}

bool RdmaRoceModel::PsnSmallerAtTarget(RoceQpContext* context, uint32_t psn1,
                                       uint32_t psn2) {
  // Compare the distance between the psn and the expected_psn. Larger distance
  // means smaller psn.
  return (context->expected_psn - psn1 + kPsnRange) % kPsnRange >
         (context->expected_psn - psn2 + kPsnRange) % kPsnRange;
}

bool RdmaRoceModel::PsnGeAtInitiator(RoceQpContext* context, uint32_t psn1,
                                     uint32_t psn2) {
  return PsnBetween(psn1, psn2, PsnAdd(context->max_psn, -1));
}

bool RdmaRoceModel::PsnIsValidAtInitiator(RoceQpContext* context,
                                          uint32_t psn) {
  return context->oldest_unacknowledged_psn != context->max_psn &&
         PsnBetween(psn, context->oldest_unacknowledged_psn,
                    PsnAdd(context->max_psn, -1));
}

void RdmaRoceModel::TransmitAck(RoceQpContext* context) {
  context->ack_coalescing_timer_trigger_time = absl::ZeroDuration();
  context->ack_coalescing_counter = 0;
  if (context->inbound_read_request_queue.empty()) {
    context->ack_triggered = false;
    // Generate ACK and send to lower layer.
    auto ack = std::make_unique<Packet>();
    ack->metadata.destination_ip_address = context->dst_ip;
    ack->roce.is_roce = true;
    ack->roce.opcode = Packet::Roce::Opcode::kAck;
    ack->roce.psn = PsnAdd(context->expected_psn, -1);
    ack->roce.ack.type = Packet::Roce::Ack::Type::kAck;
    ack->roce.dest_qp_id = context->dest_qp_id;
    // For stats collection.
    ack->roce.qp_id = context->qp_id;
    // Send packet out.
    packet_builder_->EnqueuePacket(std::move(ack));
  } else {
    // Mark ACK triggered, which will be checked after IRRQ becomes empty.
    context->ack_triggered = true;
  }
}

void RdmaRoceModel::HandleAckCoalescingTimeout(QpId qp_id) {
  auto timeout_time = env_->ElapsedTime();
  qp_manager_.InitiateQpLookup(
      qp_id, [this, timeout_time](absl::StatusOr<BaseQpContext*> qp_context) {
        CHECK_OK(qp_context.status())
            << "Timeout for unknown qp id: " << qp_context.status() << "\n";
        auto context = down_cast<RoceQpContext*>(*qp_context);
        // check if timeout is valid.
        const absl::Duration elapsed_time =
            timeout_time - context->ack_coalescing_timer_trigger_time;
        if (elapsed_time == context->ack_coalescing_timeout &&
            context->ack_coalescing_counter > 0)
          TransmitAck(context);
      });
}

void RdmaRoceModel::GenerateAckOrUpdateCoalescingState(RoceQpContext* context,
                                                       bool ack_requested) {
  // If ACK is requested, or and ACK event already scheduled.
  if (ack_requested || context->ack_coalescing_counter > 0) {
    context->ack_coalescing_counter++;
    if (context->ack_coalescing_counter >= context->ack_coalescing_threshold) {
      TransmitAck(context);
    } else if (context->ack_coalescing_timer_trigger_time ==
               absl::ZeroDuration()) {
      SetupAckCoalescingTimer(context);
    }
  }
}

void RdmaRoceModel::SetupAckCoalescingTimer(RoceQpContext* context) {
  context->ack_coalescing_timer_trigger_time = env_->ElapsedTime();
  auto ret = env_->ScheduleEvent(
      context->ack_coalescing_timeout,
      [this, context]() { HandleAckCoalescingTimeout(context->qp_id); });
}

void RdmaRoceModel::TransmitNakSequenceError(RoceQpContext* context) {
  context->nak_sequence_error_sent = true;
  // NAK-sequence-error delivers a superset of info of ACK, so no need for
  // another ACK.
  context->ack_coalescing_timer_trigger_time = absl::ZeroDuration();
  context->ack_coalescing_counter = 0;
  if (context->inbound_read_request_queue.empty()) {
    context->nak_sequence_error_triggered = false;
    // Generate NAK and send to lower layer.
    auto nak = std::make_unique<Packet>();
    nak->metadata.destination_ip_address = context->dst_ip;
    nak->roce.is_roce = true;
    nak->roce.opcode = Packet::Roce::Opcode::kAck;
    nak->roce.ack.type = Packet::Roce::Ack::Type::kNak;
    nak->roce.ack.nak_type = Packet::Roce::Ack::NakType::kPsnSequenceError;
    nak->roce.psn = PsnAdd(context->expected_psn, -1);
    nak->roce.dest_qp_id = context->dest_qp_id;
    // For stats collection.
    nak->roce.qp_id = context->qp_id;
    // Send packet out.
    packet_builder_->EnqueuePacket(std::move(nak));
  } else {
    // Mark ACK triggered, which will be checked after IRRQ becomes empty.
    context->nak_sequence_error_triggered = true;
  }
}

void RdmaRoceModel::SetupRto(RoceQpContext* context) {
  if (context->retx_timeout != absl::ZeroDuration()) {
    context->rto_trigger_time = env_->ElapsedTime();
    CHECK_OK(env_->ScheduleEvent(
        context->retx_timeout,
        [this, qp_id = context->qp_id]() { HandleRto(qp_id); }));
  }
}

void RdmaRoceModel::CancelRto(RoceQpContext* context) {
  context->rto_trigger_time = absl::ZeroDuration();
}

void RdmaRoceModel::HandleRto(QpId qp_id) {
  auto timeout_time = env_->ElapsedTime();
  qp_manager_.InitiateQpLookup(
      qp_id, [this, timeout_time](absl::StatusOr<BaseQpContext*> qp_context) {
        CHECK_OK(qp_context.status())
            << "Timeout for unknown qp id: " << qp_context.status() << "\n";
        auto context = down_cast<RoceQpContext*>(*qp_context);
        // check if timeout is valid.
        const absl::Duration elapsed_time =
            timeout_time - context->rto_trigger_time;
        if (elapsed_time == context->retx_timeout) {
          context->rto_trigger_time = absl::ZeroDuration();
          if (context->retry_counter < context->max_retry) {
            context->next_psn = context->oldest_unacknowledged_psn;
            context->next_op_index = 0;
            // next_op_index has changed, add WorkQueue to active set.
            AddWorkQueueToActiveSet(
                WorkQueue(context->qp_id, WorkType::kRequest), context);
            context->retry_counter++;
          } else {
            //
            LOG(FATAL) << "Exceeds maximum retry. Not implemented";
          }
        }
      });
}

}  // namespace isekai
