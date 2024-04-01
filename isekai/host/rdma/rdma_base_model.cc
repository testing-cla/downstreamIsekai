#include "isekai/host/rdma/rdma_base_model.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "glog/logging.h"
#include "isekai/common/common_util.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/default_config_generator.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/net_address.h"
#include "isekai/common/status_util.h"
#include "isekai/host/rdma/rdma_component_interfaces.h"
#include "isekai/host/rdma/rdma_qp_manager.h"

namespace isekai {

template <typename QpContext>
StatisticsCollectionConfig::RdmaFlags
    RdmaBaseModel<QpContext>::stats_collection_flags_;

template <typename QpContext>
RdmaBaseModel<QpContext>::RdmaBaseModel(
    RdmaType rdma_type, const RdmaConfig& config, uint32_t network_mtu_size,
    Environment* env, StatisticCollectionInterface* stats_collector,
    ConnectionManagerInterface* connection_manager)
    : config_(config),
      env_(env),
      free_list_manager_(config_),
      qp_manager_(env_),
      connection_manager_(connection_manager),
      stats_collector_(stats_collector),
      rdma_mtu_(network_mtu_size) {
  if (stats_collector_ != nullptr &&
      stats_collector_->GetConfig().has_rdma_flags()) {
    stats_collection_flags_ = stats_collector_->GetConfig().rdma_flags();
  } else {
    stats_collection_flags_ = DefaultConfigGenerator::DefaultRdmaStatsFlags();
  }
  if (rdma_type == RdmaType::kFalconRdma) {
    // This FALCON RDMA MTU value is determined by subtracting all outer header
    // lengths from a Netwok MTU.
    // -   14 : Ethernet header
    // -   40 : IPv6 header
    // -    8 : UDP header
    // -   24 : PSP header
    // -   24 : FALCON base header
    // -   12 : FALCON op header (max)
    // -   28 : RDMA header (max)
    // -   16 : PSP trailer bytes
    // -   12 : Ethernet Preamble, SFD and FCS
    rdma_mtu_ -= kEthernetHeader + kIpv6Header + kUdpHeader + kPspHeader +
                 kFalconHeader + kFalconOpHeader + kRdmaHeader + kPspTrailer +
                 kEthernetPreambleFCS;
  } else if (rdma_type == RdmaType::kRoceRdma) {
    rdma_mtu_ -= kEthernetHeader + kIpv6Header + kUdpHeader +
                 kRoceBthHeaderSize + kRoceRethHeaderSize +
                 kEthernetPreambleFCS;
  }
  CHECK_LT(rdma_mtu_, network_mtu_size)
      << "rdma MTU is not smaller than network MTU size";
}

template <typename QpContext>
void RdmaBaseModel<QpContext>::InitializeQpContext(BaseQpContext* base_context,
                                                   QpId local_qp_id,
                                                   QpOptions& options) {
  base_context->qp_id = local_qp_id;
  CHECK_OK_THEN_ASSIGN(base_context->dst_ip,
                       Ipv6Address::OfString(options.dst_ip));
}

template <typename QpContext>
void RdmaBaseModel<QpContext>::PerformOp(QpId qp_id, RdmaOpcode opcode,
                                         std::vector<uint32_t> sgl,
                                         bool is_inline,
                                         CompletionCallback completion_callback,
                                         QpId dest_qp_id) {
  uint32_t total_payload_length = 0;
  for (uint32_t segment_length : sgl) {
    CHECK(segment_length <= config_.max_segment_length() && segment_length > 0)
        << "Segment of " << segment_length << " bytes is too big or 0";
    total_payload_length += segment_length;
  }

  std::unique_ptr<RdmaOp> rdma_op;
  if (!is_inline || opcode == RdmaOpcode::kRead ||
      opcode == RdmaOpcode::kRecv) {
    CHECK(!is_inline) << "RDMA read and recv op cannot be inline.";
    rdma_op = std::make_unique<RdmaOp>(
        opcode, std::move(sgl), std::move(completion_callback), dest_qp_id);
  } else {
    CHECK(total_payload_length <= config_.max_inline_payload_length())
        << "Inline payload of " << total_payload_length << " bytes is too big";
    rdma_op =
        std::make_unique<RdmaOp>(opcode, total_payload_length,
                                 std::move(completion_callback), dest_qp_id);
  }

  // Initiate qp lookup, and when it finishes, perform further qp processing.
  qp_manager_.InitiateQpLookup(
      qp_id, [this, op = *rdma_op](absl::StatusOr<BaseQpContext*> qp_context) {
        CHECK_OK(qp_context.status())
            << "Unknown QP id: " << qp_context.status();
        PostOp(down_cast<QpContext*>(qp_context.value()), std::move(op));
      });
}

template <typename QpContext>
void RdmaBaseModel<QpContext>::Associate(QpId src_qp_id, QpId dst_qp_id,
                                         uint32_t scid) {
  qp_to_scid_[std::make_pair(src_qp_id, dst_qp_id)] = scid;
}

template <typename QpContext>
void RdmaBaseModel<QpContext>::PostOp(QpContext* context, RdmaOp op) {
  if (context->is_connected()) {
    op.dest_qp_id = context->dest_qp_id;
  } else {
    CHECK(op.opcode == RdmaOpcode::kSend || op.opcode == RdmaOpcode::kRecv)
        << "Unsupported verb on UD QP.";
  }

  if (op.opcode == RdmaOpcode::kRecv) {
    context->receive_queue.push_back(std::make_unique<RdmaOp>(op));
  } else {
    context->send_queue.push_back(std::make_unique<RdmaOp>(op));
  }
}

template <typename QpContext>
void RdmaBaseModel<QpContext>::CollectVectorStats(std::string_view stat_name,
                                                  double value) {
  if (stats_collector_) {
    CHECK_OK(stats_collector_->UpdateStatistic(
        stat_name, value, StatisticsCollectionConfig::TIME_SERIES_STAT));
  }
}

template <typename QpContext>
void RdmaBaseModel<QpContext>::CollectScalarStats(std::string_view stat_name,
                                                  double value) {
  if (stats_collector_) {
    CHECK_OK(stats_collector_->UpdateStatistic(
        stat_name, value, StatisticsCollectionConfig::SCALAR_MAX_STAT));
  }
}

template class RdmaBaseModel<FalconQpContext>;
template class RdmaBaseModel<RoceQpContext>;

}  // namespace isekai
