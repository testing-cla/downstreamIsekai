#ifndef ISEKAI_HOST_RDMA_RDMA_BASE_MODEL_H_
#define ISEKAI_HOST_RDMA_RDMA_BASE_MODEL_H_

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/host/rdma/rdma_component_interfaces.h"
#include "isekai/host/rdma/rdma_free_list_manager.h"
#include "isekai/host/rdma/rdma_latency_histograms.pb.h"
#include "isekai/host/rdma/rdma_qp_manager.h"

namespace isekai {

// The header lengths in bytes
constexpr uint32_t kEthernetHeader = 14;
constexpr uint32_t kIpv6Header = 40;
constexpr uint32_t kUdpHeader = 8;
constexpr uint32_t kPspHeader = 24;
constexpr uint32_t kFalconHeader = 24;
constexpr uint32_t kFalconOpHeader = 12;
constexpr uint32_t kRdmaHeader = 28;
constexpr uint32_t kPspTrailer = 16;
constexpr uint32_t kEthernetPreambleFCS = 12;
constexpr uint32_t kRoceBthHeaderSize = 12;
constexpr uint32_t kRoceRethHeaderSize = 16;

enum class RdmaType {
  kFalconRdma,
  kRoceRdma,
};

template <typename QpContext>
class RdmaBaseModel : public RdmaBaseInterface {
 public:
  RdmaBaseModel(RdmaType rdma_type, const RdmaConfig& config,
                uint32_t network_mtu_size, Environment* env,
                StatisticCollectionInterface* stats_collector,
                ConnectionManagerInterface* connection_manager);

  const RdmaConfig& GetConfig() override { return config_; }

  ConnectionManagerInterface* get_connection_manager() const {
    return connection_manager_;
  }

  // Schedules an RDMA op (from a traffic generator) with a scatter-gather list,
  // specified as a list of fragment lengths. The scatter-gather list may be
  // empty. Fails if there is no QP with the given source qp_id. dest_qp_id is
  // needed if QP is in UD mode. Note that sgl is pass-by-value; consider
  // std::move()ing it.
  void PerformOp(QpId qp_id, RdmaOpcode opcode, std::vector<uint32_t> sgl,
                 bool is_inline, CompletionCallback completion_callback,
                 QpId dest_qp_id) override;

  // Associates (src_qp_id, dst_qp_id) with scid.
  void Associate(QpId src_qp_id, QpId dst_qp_id, uint32_t scid) override;

  void DumpLatencyHistogramsToProto(
      RdmaLatencyHistograms* histograms) override {};

  // Collection methods.
  void CollectVectorStats(std::string_view stat_name, double value);
  void CollectScalarStats(std::string_view stat_name, double value);

 protected:
  virtual void InitializeQpContext(BaseQpContext* base_context,
                                   QpId local_qp_id, QpOptions& options);

  // Schedules an RDMA op on the given QP context.
  virtual void PostOp(QpContext* qp_context, RdmaOp op);

  const RdmaConfig config_;
  Environment* const env_;

  RdmaFreeListManager free_list_manager_;
  RdmaQpManagerInfiniteResources qp_manager_;
  ConnectionManagerInterface* connection_manager_ = nullptr;

  // Mapping from (src_qp_id, dst_qp_id) to scid.
  absl::flat_hash_map<std::pair<QpId, QpId>, uint32_t> qp_to_scid_;

  // Stats collector and flags.
  StatisticCollectionInterface* const stats_collector_;
  static StatisticsCollectionConfig::RdmaFlags stats_collection_flags_;

  // Maximum size of a data segment that the RDMA model can send to FALCON. It
  // excludes various other headers (such as Ethernet, IP, UDP, FALCON, PSP,
  // etc), all of which sum upto network MTU.
  uint32_t rdma_mtu_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_RDMA_RDMA_BASE_MODEL_H_
