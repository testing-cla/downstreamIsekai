#ifndef ISEKAI_HOST_RDMA_RDMA_ROCE_CC_DCQCN_H_
#define ISEKAI_HOST_RDMA_RDMA_ROCE_CC_DCQCN_H_

#include <cstdint>

#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/host/rdma/rdma_component_interfaces.h"
#include "isekai/host/rdma/rdma_configuration.h"

namespace isekai {

struct RdmaCcDcqcnOptions : public RdmaCcOptions {
  // Max possible rate (line rate).
  uint64_t max_rate = 100000;
  // DCQCN parameters: https://community.mellanox.com/s/article/dcqcn-parameters
  uint32_t dce_alpha_g = 1019;
  uint32_t dce_alpha_update_period = 1;
  uint32_t initial_alpha_value = 1023;
  uint32_t rate_to_set_on_first_cnp = 85000;
  uint32_t min_dec_fac = 50;
  uint32_t min_rate = 1;
  uint32_t alpha_to_rate_shift = 11;
  uint32_t rate_reduce_monitor_period = 4;
  bool clamp_tgt_rate = false;
  bool clamp_tgt_rate_after_time_inc = true;
  uint32_t time_reset = 300;
  uint32_t byte_reset = 32767;
  uint32_t stage_threshold = 1;
  uint32_t ai_rate = 5;
  uint32_t hai_rate = 50;
};

class RdmaRoceCcDcqcn : public RdmaRoceCcInterface {
 public:
  RdmaRoceCcDcqcn(Environment* env, RdmaQpManagerInterface* qp_manager,
                  RdmaCcDcqcnOptions* options)
      : env_(env), qp_manager_(qp_manager), options_(*options) {}
  void HandleRxPacket(RoceQpContext* context, Packet* packet) override;
  void HandleTxPacket(RoceQpContext* context, Packet* packet) override;
  void SentBytes(RoceQpContext* context, uint32_t bytes);
  void InitContext(RoceQpContext* context) override;

 private:
  Environment* env_ = nullptr;
  RdmaQpManagerInterface* qp_manager_ = nullptr;
  RdmaCcDcqcnOptions options_;

  void HandleAlphaUpdateTimer(QpId qp_id);
  void HandleRateDecreaseTimer(QpId qp_id);
  void HandleRateIncreaseTimer(QpId qp_id);
  void IncreaseRate(RoceQpContext* context);
};

}  // namespace isekai

#endif  // ISEKAI_HOST_RDMA_RDMA_ROCE_CC_DCQCN_H_
