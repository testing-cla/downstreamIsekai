#include "isekai/host/falcon/gen2/sram_dram_reorder_engine.h"

#include <cstdint>
#include <memory>
#include <queue>
#include <utility>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/gen2/falcon_types.h"
#include "isekai/host/falcon/gen2/falcon_utils.h"

namespace isekai {

Gen2SramDramReorderEngine::Gen2SramDramReorderEngine(
    FalconModelInterface* falcon, uint8_t num_hosts)
    : falcon_(falcon) {
  for (auto host_id = 0; host_id < num_hosts; ++host_id) {
    auto prefetch_buffer = std::make_unique<HostPrefetchBufferMetadata>(
        falcon_->get_config()
            ->gen2_config_options()
            .on_nic_dram_config()
            .per_host_prefetch_buffer_size_bytes());
    prefetch_buffers_[host_id] = std::move(prefetch_buffer);
  }
}

// Initializes the relevant connection queue.
absl::Status Gen2SramDramReorderEngine::InitConnectionQueue(
    uint8_t bifurcation_id, uint32_t scid) {
  if (!prefetch_buffers_.contains(bifurcation_id)) {
    LOG(FATAL) << "Host prefetch buffer not initialized.";
  }
  bool was_inserted =
      prefetch_buffers_[bifurcation_id]
          ->connection_queues
          .insert({scid, std::queue<PacketPrefetchBufferMetadata>()})
          .second;
  if (!was_inserted) {
    return absl::AlreadyExistsError(
        "Duplicate source connection ID. Queue already exist.");
  }
  return absl::OkStatus();
}

// Enqueues the work item in the appropriate queue, if required, or pass it on
// to the next processing step. Specifically -
// Case 1: Packet in DRAM, need to wait until packet is fetched from DRAM.
// Case 2: Packet in SRAM & belongs to unordered connection or is HoL packet,
// can move onto next processing step rightaway.
// Case 3: non-HOL packet in SRAM & belongs to ordered connection, needs HoL
// packet to be cleared out first.
void Gen2SramDramReorderEngine::EnqueuePacket(uint8_t bifurcation_id,
                                              const BufferFetchWorkId& work_id,
                                              PacketBufferLocation& location,
                                              uint32_t& packet_payload_length) {
  auto& prefetch_buffer = prefetch_buffers_[bifurcation_id];
  auto& connection_queue = prefetch_buffer->connection_queues[work_id.cid];
  // 1. Packet needs to be fetched from DRAM.
  if (location == PacketBufferLocation::kDram) {
    // Packet is being fetched from DRAM. Decrement prefetch buffer and put the
    // work item in the appropriate queue.
    prefetch_buffer->occupied_size_bytes += packet_payload_length;
    CHECK_LE(prefetch_buffer->occupied_size_bytes,
             prefetch_buffer->buffer_size_bytes);
    connection_queue.push(
        PacketPrefetchBufferMetadata(work_id, packet_payload_length));
  } else {
    // Packet in SRAM.
    CHECK_OK_THEN_ASSIGN(
        auto connection_state,
        falcon_->get_state_manager()->PerformDirectLookup(work_id.cid));
    // 2. Packet in SRAM belongs to unordered connection or is HoL packet
    // corresponding to an ordered connection.
    if ((connection_state->connection_metadata.ordered_mode ==
         OrderingMode::kUnordered) ||
        connection_queue.empty()) {
      // Packet can be passed to the next processing step rightaway.
      falcon_->ReorderCallback(work_id.cid, work_id.rsn, work_id.type);
    } else {
      // 3. Packet in SRAM belongs to ordered connection and is non-HoL. Packet
      // is blocked by HoL packet that needs to be fetched from DRAM. Decrement
      // prefetch buffer and put the work item in the appropriate queue.
      prefetch_buffer->occupied_size_bytes += packet_payload_length;
      CHECK_LE(prefetch_buffer->occupied_size_bytes,
               prefetch_buffer->buffer_size_bytes);
      connection_queue.push(
          PacketPrefetchBufferMetadata(work_id, packet_payload_length));
    }
  }
}

// Checks if there is space in the prefetch buffer to land the packet data.
bool Gen2SramDramReorderEngine::CanPrefetchData(uint8_t bifurcation_id,
                                                uint32_t& size_bytes) {
  auto& prefetch_buffer = prefetch_buffers_[bifurcation_id];
  return ((prefetch_buffer->occupied_size_bytes + size_bytes) <=
          prefetch_buffer->buffer_size_bytes);
}

// Caller return space reserved in prefetch buffer.
void Gen2SramDramReorderEngine::RefundPrefetchBufferSpace(
    uint8_t bifurcation_id, uint32_t& size_bytes) {
  auto& prefetch_buffer = prefetch_buffers_[bifurcation_id];
  prefetch_buffer->occupied_size_bytes -= size_bytes;
}

// Called on DRAM fetch completion to trigger the next processing step. The
// packet is sent to the next processing step via the reorder callback. Further
// we continue sending subsequent packets corresponding to this connection in
// SRAM to the next processing step.
void Gen2SramDramReorderEngine::OnDramFetchCompletion(
    uint8_t bifurcation_id, const BufferFetchWorkId& work_id) {
  auto& prefetch_buffer = prefetch_buffers_[bifurcation_id];
  auto& connection_queue = prefetch_buffer->connection_queues[work_id.cid];
  auto& front_work_item = connection_queue.front();
  falcon_->ReorderCallback(front_work_item.work_id.cid,
                           front_work_item.work_id.rsn,
                           front_work_item.work_id.type);
  connection_queue.pop();
  while (!connection_queue.empty()) {
    auto& subsequent_work_item = connection_queue.front();
    CHECK_OK_THEN_ASSIGN(auto packet_context,
                         GetReceivedPacketContextFromBufferFetchWorkId(
                             falcon_, subsequent_work_item.work_id));
    auto location = packet_context->location;
    if (location == PacketBufferLocation::kSram) {
      falcon_->ReorderCallback(subsequent_work_item.work_id.cid,
                               subsequent_work_item.work_id.rsn,
                               subsequent_work_item.work_id.type);

      connection_queue.pop();
    } else {
      break;
    }
  }
}

}  // namespace isekai
