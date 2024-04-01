#ifndef ISEKAI_HOST_RDMA_RDMA_PER_HOST_RX_BUFFERS_H_
#define ISEKAI_HOST_RDMA_RDMA_PER_HOST_RX_BUFFERS_H_

#include <queue>
#include <vector>

#include "isekai/common/config.pb.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/host/rnic/memory_interface.h"

namespace isekai {

// RDMA Buffer item definition. It essentially consists of the callback and
// pointer to the packet for getting the packet size for the processing.
struct HostBufferItem {
  // Callback when PCIe transaction completes.
  absl::AnyInvocable<void()> pcie_callback_;
  // This callback is called when the packet is handed over to PCIe device.
  absl::AnyInvocable<void()> rdma_pcie_transfer_callback_;
  uint64_t payload_length_;
  HostBufferItem(absl::AnyInvocable<void()> pcie_callback,
                 absl::AnyInvocable<void()> rdma_pcie_transfer_callback,
                 uint64_t payload_length)
      : pcie_callback_(std::move(pcie_callback)),
        rdma_pcie_transfer_callback_(std::move(rdma_pcie_transfer_callback)),
        payload_length_(std::move(payload_length)) {}
};

// Abstraction of host queue.
struct HostBuffer {
  std::queue<HostBufferItem> buffer_;
  uint32_t max_size_bytes_;
  uint32_t current_size_bytes_;

  explicit HostBuffer(uint32_t max_size_bytes)
      : max_size_bytes_(std::move(max_size_bytes)), current_size_bytes_(0) {}
  HostBuffer() = delete;
  HostBuffer(const HostBuffer&) = delete;
  HostBuffer& operator=(const HostBuffer&) = delete;
};

// Abstraction for per host buffers. It essentially puts the RDMA buffer item
// into the corresponding queue. Once the host queue gets full, this abstraction
// xoffs the interhost Rx scheduler for this particular host.
class RdmaPerHostRxBuffers {
 public:
  // Constructor.
  RdmaPerHostRxBuffers(const RdmaRxBufferConfig& config,
                       std::vector<std::unique_ptr<MemoryInterface>>* hif,
                       FalconInterface* falcon);

  // Delete copy constructor and assignment operator.
  RdmaPerHostRxBuffers(const RdmaPerHostRxBuffers&) = delete;
  RdmaPerHostRxBuffers& operator=(const RdmaPerHostRxBuffers&) = delete;
  void Push(uint8_t host_idx, absl::AnyInvocable<void()> pcie_callback,
            std::unique_ptr<Packet> packet,
            absl::AnyInvocable<void()> rdma_pcie_transfer_callback);

 private:
  // Pointer to host interface; there is one-one mapping from falcon inter-host
  // rx schduler to RDMA buffer and host interface.
  std::vector<std::unique_ptr<MemoryInterface>>* const host_interface_;
  std::vector<std::unique_ptr<HostBuffer>> rx_buffer_;
  // Pointer to falcon interface; used for xoffing the the interhost Rx
  // scheduler for this particular host.
  FalconInterface* const falcon_;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_RDMA_RDMA_PER_HOST_RX_BUFFERS_H_
