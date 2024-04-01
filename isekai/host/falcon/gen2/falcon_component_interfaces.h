#ifndef ISEKAI_HOST_FALCON_GEN2_FALCON_COMPONENT_INTERFACES_H_
#define ISEKAI_HOST_FALCON_GEN2_FALCON_COMPONENT_INTERFACES_H_

#include <cstdint>

#include "absl/status/status.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/host/falcon/gen2/sram_dram_reorder_engine.h"
#include "isekai/host/rnic/memory_interface.h"

namespace isekai {

class Gen2FalconModelExtensionInterface {
 public:
  virtual ~Gen2FalconModelExtensionInterface() = default;
  virtual MemoryInterface* get_on_nic_dram_interface() const = 0;
  virtual Gen2SramDramReorderEngine* get_sram_dram_reorder_engine() const = 0;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_FALCON_COMPONENT_INTERFACES_H_
