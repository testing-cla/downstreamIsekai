#ifndef ISEKAI_HOST_FALCON_FALCON_COMPONENT_FACTORIES_H_
#define ISEKAI_HOST_FALCON_FALCON_COMPONENT_FACTORIES_H_

#include <cstdint>
#include <memory>
#include <utility>

#include "absl/log/log.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_inter_host_rx_scheduler.h"
#include "isekai/host/falcon/falcon_protocol_ack_coalescing_engine.h"
#include "isekai/host/falcon/falcon_protocol_ack_nack_scheduler.h"
#include "isekai/host/falcon/falcon_protocol_admission_control_manager.h"
#include "isekai/host/falcon/falcon_protocol_buffer_reorder_engine.h"
#include "isekai/host/falcon/falcon_protocol_connection_state_manager.h"
#include "isekai/host/falcon/falcon_protocol_fast_connection_scheduler.h"
#include "isekai/host/falcon/falcon_protocol_fast_retransmission_scheduler.h"
#include "isekai/host/falcon/falcon_protocol_packet_reliability_manager.h"
#include "isekai/host/falcon/falcon_protocol_packet_type_based_connection_scheduler.h"
#include "isekai/host/falcon/falcon_protocol_rate_update_engine.h"
#include "isekai/host/falcon/falcon_protocol_resource_manager.h"
#include "isekai/host/falcon/falcon_protocol_retransmission_scheduler.h"
#include "isekai/host/falcon/falcon_protocol_round_robin_arbiter.h"
#include "isekai/host/falcon/gen2/ack_coalescing_engine.h"
#include "isekai/host/falcon/gen2/ack_nack_scheduler.h"
#include "isekai/host/falcon/gen2/admission_control_manager.h"
#include "isekai/host/falcon/gen2/arbiter.h"
#include "isekai/host/falcon/gen2/connection_scheduler.h"
#include "isekai/host/falcon/gen2/connection_state_manager.h"
#include "isekai/host/falcon/gen2/falcon_component_interfaces.h"
#include "isekai/host/falcon/gen2/inter_host_rx_scheduler.h"
#include "isekai/host/falcon/gen2/packet_metadata_transformer.h"
#include "isekai/host/falcon/gen2/rate_update_engine.h"
#include "isekai/host/falcon/gen2/reliability_manager.h"
#include "isekai/host/falcon/gen2/reorder_engine.h"
#include "isekai/host/falcon/gen2/resource_manager.h"
#include "isekai/host/falcon/gen2/retransmission_scheduler.h"
#include "isekai/host/falcon/gen2/rx_packet_buffer_fetch_arbiter.h"
#include "isekai/host/falcon/gen2/sram_dram_reorder_engine.h"
#include "isekai/host/falcon/gen2/stats_manager.h"
#include "isekai/host/falcon/gen3/ack_coalescing_engine.h"
#include "isekai/host/falcon/gen3/ack_nack_scheduler.h"
#include "isekai/host/falcon/gen3/admission_control_manager.h"
#include "isekai/host/falcon/gen3/arbiter.h"
#include "isekai/host/falcon/gen3/connection_scheduler.h"
#include "isekai/host/falcon/gen3/connection_state_manager.h"
#include "isekai/host/falcon/gen3/inter_host_rx_scheduler.h"
#include "isekai/host/falcon/gen3/packet_metadata_transformer.h"
#include "isekai/host/falcon/gen3/rate_update_engine.h"
#include "isekai/host/falcon/gen3/reliability_manager.h"
#include "isekai/host/falcon/gen3/reorder_engine.h"
#include "isekai/host/falcon/gen3/resource_manager.h"
#include "isekai/host/falcon/gen3/retransmission_scheduler.h"
#include "isekai/host/falcon/gen3/stats_manager.h"
#include "isekai/host/falcon/packet_metadata_transformer.h"
#include "isekai/host/falcon/stats_manager.h"
#include "isekai/host/rnic/memory_interface.h"

namespace isekai {

// Factory creates the appropriate connection state manager based on
// the simulation mode.
static std::unique_ptr<ConnectionStateManager> CreateConnectionStateManager(
    FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    return std::make_unique<ProtocolConnectionStateManager>(falcon);
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2ConnectionStateManager>(falcon);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3ConnectionStateManager>(falcon);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

// Factory creates the appropriate ACK coalescing engine based on the
// simulation mode.
inline std::unique_ptr<AckCoalescingEngineInterface> CreateAckCoalescingEngine(
    FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    return std::make_unique<Gen1AckCoalescingEngine>(falcon);
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2AckCoalescingEngine>(falcon);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3AckCoalescingEngine>(falcon);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

// Factory creates the appropriate flow control manager based on the
// simulation mode.
static std::unique_ptr<ResourceManager> CreateResourceManager(
    FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    return std::make_unique<ProtocolResourceManager>(falcon);
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2ResourceManager>(falcon);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3ResourceManager>(falcon);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

// Factory creates the appropriate inter-host scheduler based on the
// simulation mode.
static std::unique_ptr<InterHostRxScheduler> CreateInterHostRxScheduler(
    FalconModelInterface* falcon, const uint8_t number_of_hosts) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    return std::make_unique<ProtocolInterHostRxScheduler>(falcon,
                                                          number_of_hosts);
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2InterHostRxScheduler>(falcon, number_of_hosts);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3InterHostRxScheduler>(falcon, number_of_hosts);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

// Factory creates the appropriate connection scheduler based on the
// simulation mode.
static std::unique_ptr<Scheduler> CreateConnectionScheduler(
    FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  auto scheduler_variant = falcon->get_config()->scheduler_variant();
  // Use ProtocolPacketTypeBasedConnectionScheduler (busy-poll scheduler) by
  // default.

  if (scheduler_variant == FalconConfig::UNSUPPORTED_SCHEDULER ||
      falcon_generation == 2 || falcon_generation == 3) {
    scheduler_variant = FalconConfig::BUSY_POLL_SCHEDULER;
  }
  if (falcon_generation == 1) {
    switch (scheduler_variant) {
      case FalconConfig::BUSY_POLL_SCHEDULER:
        return std::make_unique<ProtocolPacketTypeBasedConnectionScheduler>(
            falcon);
      case FalconConfig::EVENT_BASED_SCHEDULER:
        return std::make_unique<ProtocolFastConnectionScheduler>(falcon);
      default:
        LOG(FATAL) << "Unsupported scheduler variant provided.";
    }
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2ConnectionScheduler>(falcon);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3ConnectionScheduler>(falcon);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

// Factory creates the appropriate retransmission scheduler based on the
// simulation mode.
static std::unique_ptr<Scheduler> CreateRetransmissionScheduler(
    FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  auto scheduler_variant = falcon->get_config()->scheduler_variant();

  if (scheduler_variant == FalconConfig::UNSUPPORTED_SCHEDULER ||
      falcon_generation == 2 || falcon_generation == 3) {
    scheduler_variant = FalconConfig::BUSY_POLL_SCHEDULER;
  }
  if (falcon_generation == 1) {
    switch (scheduler_variant) {
      case FalconConfig::BUSY_POLL_SCHEDULER:
        return std::make_unique<ProtocolRetransmissionScheduler>(falcon);
      case FalconConfig::EVENT_BASED_SCHEDULER:
        return std::make_unique<ProtocolFastRetransmissionScheduler>(falcon);
      default:
        LOG(FATAL) << "Unsupported scheduler variant provided.";
    }
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2RetransmissionScheduler>(falcon);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3RetransmissionScheduler>(falcon);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

// Factory creates the appropriate ack/nack scheduler based on the
// simulation mode.
static std::unique_ptr<Scheduler> CreateAckNackScheduler(
    FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    return std::make_unique<ProtocolAckNackScheduler>(falcon);
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2AckNackScheduler>(falcon);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3AckNackScheduler>(falcon);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

// Factory creates the appropriate arbiter based on the
// simulation mode.
static std::unique_ptr<Arbiter> CreateArbiter(FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    return std::make_unique<ProtocolRoundRobinArbiter>(falcon);
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2Arbiter>(falcon);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3Arbiter>(falcon);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

// Factory creates the appropriate admission control manager based on the
// simulation mode.
static std::unique_ptr<AdmissionControlManager> CreateAdmissionControlManager(
    FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    return std::make_unique<ProtocolAdmissionControlManager>(falcon);
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2AdmissionControlManager>(falcon);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3AdmissionControlManager>(falcon);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

// Factory creates the appropriate packet reliability manager based on the
// simulation mode.
static std::unique_ptr<PacketReliabilityManager> CreatePacketReliabilityManager(
    FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    return std::make_unique<ProtocolPacketReliabilityManager>(falcon);
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2ReliabilityManager>(falcon);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3ReliabilityManager>(falcon);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

// Factory creates the appropriate rate update engine based on the
// simulation mode.
static std::unique_ptr<RateUpdateEngine> CreateRateUpdateEngine(
    FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    return std::make_unique<Gen1RateUpdateEngine>(falcon);
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2RateUpdateEngine>(falcon);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3RateUpdateEngine>(falcon);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

// Factory creates the appropriate buffer reorder engine based on the
// simulation mode.
static std::unique_ptr<BufferReorderEngine> CreateBufferReorderEngine(
    FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    return std::make_unique<ProtocolBufferReorderEngine>(falcon);
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2ReorderEngine>(falcon);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3ReorderEngine>(falcon);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

// Factory creates the appropriate stats manager based on the simulation mode.
static std::unique_ptr<FalconStatsManager> CreateStatsManager(
    FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    return std::make_unique<FalconStatsManager>(falcon);
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2StatsManager>(falcon);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3StatsManager>(falcon);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

inline std::unique_ptr<MemoryInterface> CreateOnNicDramInterface(
    FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    LOG(FATAL) << "No support for On-NIC DRAM in this Falcon generation.";
  } else if (falcon_generation == 2 || falcon_generation == 3) {
    return std::make_unique<MemoryInterface>(falcon->get_config()
                                                 ->gen2_config_options()
                                                 .on_nic_dram_config()
                                                 .memory_interface_config(),
                                             falcon->get_environment());
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

//
inline std::unique_ptr<Gen2SramDramReorderEngine> CreateSramDramReorderEngine(
    FalconModelInterface* falcon, const uint8_t number_of_hosts) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    LOG(FATAL) << "No support for On-NIC DRAM in this Falcon generation.";
  } else if (falcon_generation == 2 || falcon_generation == 3) {
    return std::make_unique<Gen2SramDramReorderEngine>(falcon, number_of_hosts);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

//
inline std::unique_ptr<Gen2RxPacketBufferFetchArbiter>
CreateRxPacketBufferFetchArbiter(FalconModelInterface* falcon,
                                 const uint8_t number_of_hosts) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    LOG(FATAL) << "No support for On-NIC DRAM in this Falcon generation.";
  } else if (falcon_generation == 2 || falcon_generation == 3) {
    return std::make_unique<Gen2RxPacketBufferFetchArbiter>(falcon,
                                                            number_of_hosts);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

inline std::unique_ptr<PacketMetadataTransformer>
CreatePacketMetadataTransformer(FalconModelInterface* falcon) {
  auto falcon_generation = falcon->GetVersion();
  if (falcon_generation == 1) {
    return std::make_unique<Gen1PacketMetadataTransformer>(falcon);
  } else if (falcon_generation == 2) {
    return std::make_unique<Gen2PacketMetadataTransformer>(falcon);
  } else if (falcon_generation == 3) {
    return std::make_unique<Gen3PacketMetadataTransformer>(falcon);
  } else {
    LOG(FATAL) << "Invalid Falcon protocol generation.";
  }
}

}  // namespace isekai
#endif  // ISEKAI_HOST_FALCON_FALCON_COMPONENT_FACTORIES_H_
