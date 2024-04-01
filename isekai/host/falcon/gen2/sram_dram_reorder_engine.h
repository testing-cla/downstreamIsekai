#ifndef ISEKAI_HOST_FALCON_GEN2_SRAM_DRAM_REORDER_ENGINE_H_
#define ISEKAI_HOST_FALCON_GEN2_SRAM_DRAM_REORDER_ENGINE_H_

#include <stdint.h>

#include <memory>
#include <queue>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/gen2/falcon_types.h"

namespace isekai {

// Holds the metadata corresponding to a packet within the reorder engine.
struct PacketPrefetchBufferMetadata {
  BufferFetchWorkId work_id;
  uint32_t packet_payload_length;
  explicit PacketPrefetchBufferMetadata(BufferFetchWorkId work_id,
                                        uint32_t packet_payload_length)
      : work_id(work_id), packet_payload_length(packet_payload_length) {}
};

// Holds metadata corresponding to a host within the reorder engine.
struct HostPrefetchBufferMetadata {
  // Prefetch buffer size in bytes.
  const uint32_t buffer_size_bytes;
  // Current occupancy of the prefetch buffer.
  uint32_t occupied_size_bytes;
  // Per-connection queues within the host.
  absl::flat_hash_map<uint32_t, std::queue<PacketPrefetchBufferMetadata>>
      connection_queues;
  explicit HostPrefetchBufferMetadata(uint32_t buffer_size_bytes)
      : buffer_size_bytes(buffer_size_bytes), occupied_size_bytes(0) {}
};

// Responsible for reordering data fetched from DRAM and SRAM to ensure RSN
// ordering on a connection-basis is still enforced.
class Gen2SramDramReorderEngine {
 public:
  explicit Gen2SramDramReorderEngine(FalconModelInterface* falcon,
                                     uint8_t num_hosts);
  // Initializes the relevant connection queue.
  absl::Status InitConnectionQueue(uint8_t bifurcation_id, uint32_t scid);
  // Enqueue the packet in the appropriate queue.
  void EnqueuePacket(uint8_t bifurcation_id, const BufferFetchWorkId& work_id,
                     PacketBufferLocation& location,
                     uint32_t& packet_payload_length);
  // Checks if there is enough space in the prefetch buffer to land the packet
  // data.
  bool CanPrefetchData(uint8_t bifurcation_id, uint32_t& size_bytes);
  // Caller return space reserved in prefetch buffer.
  void RefundPrefetchBufferSpace(uint8_t bifurcation_id, uint32_t& size_bytes);
  // Called on DRAM fetch completion to update the packet status, and trigger
  // the next processing step.
  void OnDramFetchCompletion(uint8_t bifurcation_id,
                             const BufferFetchWorkId& work_id);

 private:
  FalconModelInterface* const falcon_;
  // Map that holds the per-host prefetch buffers.
  absl::flat_hash_map<uint8_t, std::unique_ptr<HostPrefetchBufferMetadata>>
      prefetch_buffers_;
};

}  // namespace isekai
#endif  // ISEKAI_HOST_FALCON_GEN2_SRAM_DRAM_REORDER_ENGINE_H_
