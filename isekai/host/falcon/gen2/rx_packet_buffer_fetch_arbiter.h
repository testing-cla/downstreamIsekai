#ifndef ISEKAI_HOST_FALCON_GEN2_RX_PACKET_BUFFER_FETCH_ARBITER_H_
#define ISEKAI_HOST_FALCON_GEN2_RX_PACKET_BUFFER_FETCH_ARBITER_H_

#include <cstdint>
#include <memory>
#include <queue>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/gen2/falcon_types.h"
#include "isekai/host/falcon/gen2/sram_dram_reorder_engine.h"
#include "isekai/host/rnic/memory_interface.h"

namespace isekai {

// Arbiter that is responsible for arbitration across hosts to fetch buffers
// from SRAM or DRAM. DRAM fetches can be done only when there is space
// available in the read queue of the on-NIC DRAM memory interface and space
// available to land DRAM fetched data in the per-host prefetch buffers.
class Gen2RxPacketBufferFetchArbiter {
 public:
  explicit Gen2RxPacketBufferFetchArbiter(FalconModelInterface* falcon,
                                          uint8_t num_hosts);
  // Enqueue the packet metadata in the appropriate host queue.
  void EnqueuePacket(uint8_t bifurcation_id, const BufferFetchWorkId& work_id);
  // Removes head packet metadata from the corresponding host queue.
  void DequeuePacket(uint8_t bifurcation_id);
  // Returns true if the arbiter has outstanding work, i.e., packets that need
  // their buffers to be fetched.
  bool HasOutstandingWork();
  // Performs one unit of work, i.e., fetches buffer corresponding to selected
  // packet based on the arbitration policy.
  void DoWork();
  // Sets Xoff/Xon for the given host queue. This happens when the host has run
  // out of its DRAM prefetch buffers.
  void SetXoff(uint8_t bifurcation_id, bool xoff);

 private:
  // Initializes the host queue based on the ID passed.
  absl::Status InitializeHostQueue(uint8_t bifurcation_id);

  FalconModelInterface* const falcon_;
  MemoryInterface* const dram_interface_;
  Gen2SramDramReorderEngine* const sram_dram_reorder_engine_;
  // Represents the arbitration policy adopted by the arbiter.
  std::unique_ptr<InterHostRxSchedulingPolicy> arbitration_policy_;
  // Indicates if arbiter is XoFFed as the memory interface queue is full.
  bool is_arbiter_xoff_;
  // Map that holds the per host queues in which packet metadata is held.
  absl::flat_hash_map<uint8_t, std::queue<BufferFetchWorkId>> host_queues_;
};
}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_GEN2_RX_PACKET_BUFFER_FETCH_ARBITER_H_
