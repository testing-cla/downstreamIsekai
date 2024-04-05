#include "isekai/host/rdma/rdma_per_host_rx_buffers.h"

#include <functional>
#include <memory>

#include "absl/strings/substitute.h"
#include "isekai/host/rdma/rdma_base_model.h"

namespace isekai {

static constexpr std::string_view kRdmaHostRxBufferBytes =
    "host.rdma.bifurcation$0.rx_buffer_bytes";

// Constructor for Per host buffers maintained at RDMA.
RdmaPerHostRxBuffers::RdmaPerHostRxBuffers(
    const RdmaRxBufferConfig& config,
    std::vector<std::unique_ptr<MemoryInterface>>* hif, FalconInterface* falcon)
    : host_interface_(hif), falcon_(falcon) {
  CHECK(host_interface_) << "Host interface cannot be nullptr";
  CHECK(falcon_) << "Falcon interface cannot be nullptr";
  CHECK(config.buffer_size_bytes_size() == host_interface_->size())
      << "Number of RDMA Rx buffer does not match with number of hosts";

  for (auto buffer_size : config.buffer_size_bytes()) {
    rx_buffer_.push_back(std::make_unique<HostBuffer>(buffer_size));
  }

  // The wakeup callback is mainly used because we did not want Host interface
  // to have any dependency on RDMA PE. The RDMA PE uses this device and is
  // responsible for sending more data (push model as done in NetDevices) if
  // any.
  auto wakeup_callback = [this](uint8_t host_id) {
    MemoryInterface* interface = (*host_interface_)[host_id].get();
    HostBuffer* rx_buf = rx_buffer_[host_id].get();
    while (!rx_buf->buffer_.empty()) {
      auto& rx_buffer_item = rx_buf->buffer_.front();
      auto payload_length = rx_buffer_item.payload_length_;
      auto pcie_payload = 0;
      // Remove ULP header length when transferring the packet for PCIe
      // transaction.
      if ((payload_length - kRdmaHeader) >= 0) {
        pcie_payload = payload_length - kRdmaHeader;
      }
      auto status = interface->CanWriteToMemory();
      if (absl::IsResourceExhausted(status)) {
        break;
      }
      // PCIe device has accepted the packet, remove from RDMA Rx buffer and
      // call transfer success which may trigger the ACK to Falcon.
      interface->WriteToMemory(pcie_payload,
                               std::move(rx_buffer_item.pcie_callback_));
      if (rx_buffer_item.rdma_pcie_transfer_callback_) {
        rx_buffer_item.rdma_pcie_transfer_callback_();
      }
      rx_buf->current_size_bytes_ -= rx_buffer_item.payload_length_;
      rx_buf->buffer_.pop();
      // Record new outstanding RDMA bytes in Rx buffer.
      StatisticCollectionInterface* stats_collector =
          falcon_->get_stats_collector();
      if (stats_collector) {
        // Records TX Packet Credits available.
        CHECK_OK(stats_collector->UpdateStatistic(
            absl::Substitute(kRdmaHostRxBufferBytes,
                             static_cast<uint32_t>(host_id)),
            rx_buf->current_size_bytes_,
            StatisticsCollectionConfig::TIME_SERIES_STAT));
      }
    }
    if (rx_buf->current_size_bytes_ < rx_buf->max_size_bytes_) {
      // There is space available in the RDMA Rx buffer for this host, send
      // xon to the falcon.
      falcon_->SetXoffByRdma(host_id, false);
    }
  };
  for (uint32_t host_idx = 0; host_idx < (*host_interface_).size();
       host_idx++) {
    MemoryInterface* interface = (*host_interface_)[host_idx].get();
    interface->SetWriteCallback(std::bind(wakeup_callback, host_idx));
  }
}

// Push packet and completion into the provided host queue. This function
// xoffs the Falcon when the configured queue limit is reached or exceeded for
// the first time.
void RdmaPerHostRxBuffers::Push(
    uint8_t host_idx, absl::AnyInvocable<void()> pcie_callback,
    std::unique_ptr<Packet> packet,
    absl::AnyInvocable<void()> rdma_pcie_transfer_callback) {
  CHECK(host_idx < rx_buffer_.size())
      << "Out of bound host idx" << static_cast<uint32_t>(host_idx);
  MemoryInterface* interface = (*host_interface_)[host_idx].get();
  HostBuffer* rx_buf = rx_buffer_[host_idx].get();

  // Falcon payload (i.e., ULP header and its payload) occupies the RDMA Rx
  // buffer. When the packet is transferred from RDMA Rx buffer to PCIe device
  // in HostInterface, we rip off the ULP header.
  uint32_t payload_length = packet ? packet->falcon.payload_length : 0;
  uint32_t pcie_payload = 0;
  // Remove ULP header length when transferring the packet for PCIe
  // transaction.
  if (payload_length >= kRdmaHeader) {
    pcie_payload = payload_length - kRdmaHeader;
  }

  auto status = interface->CanWriteToMemory();
  if (status.ok()) {
    interface->WriteToMemory(pcie_payload, std::move(pcie_callback));

    // When HostInterface accepts the packet, we right away call transfer
    // callback that might trigger ACK back to Falcon.
    if (rdma_pcie_transfer_callback) {
      rdma_pcie_transfer_callback();
    }
    return;
  }

  // Store the packet into the per host RDMA Rx buffers because the PCIe device
  // buffer is full. Note that HostBufferItem should take ownership or make a
  // copy of its fields because they will be used in a later callback.
  HostBufferItem item(std::move(pcie_callback),
                      std::move(rdma_pcie_transfer_callback), payload_length);
  rx_buf->buffer_.push(std::move(item));
  rx_buf->current_size_bytes_ += payload_length;
  // Record new outstanding RDMA bytes in Rx buffer.
  StatisticCollectionInterface* stats_collector =
      falcon_->get_stats_collector();
  if (stats_collector) {
    // Records TX Packet Credits available.
    CHECK_OK(stats_collector->UpdateStatistic(
        absl::Substitute(kRdmaHostRxBufferBytes,
                         static_cast<uint32_t>(host_idx)),
        rx_buf->current_size_bytes_,
        StatisticsCollectionConfig::TIME_SERIES_STAT));
  }

  // The below logic is little bit broken, it allows last packet to exceed the
  // max available buffer limits. We need to have a mechanism in Falcon that
  // is aware of the available capacity of the RDMA Rx buffer instead of
  // simply having the xoff signal.
  if (rx_buf->current_size_bytes_ >= rx_buf->max_size_bytes_) {
    // Rx buffer is exhausted, xoff Falcon to send any packets to RDMA for
    // this host.
    falcon_->SetXoffByRdma(host_idx, true);
  }
}

}  // namespace isekai
