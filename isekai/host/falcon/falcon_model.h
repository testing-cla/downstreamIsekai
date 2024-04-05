#ifndef ISEKAI_HOST_FALCON_FALCON_MODEL_H_
#define ISEKAI_HOST_FALCON_FALCON_MODEL_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/time/time.h"
#include "glog/logging.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_histograms.h"
#include "isekai/host/falcon/falcon_inter_host_rx_scheduler.h"

namespace isekai {

// The FalconModel class implements a software model of the FALCON module within
// something. It integrates with RdmaModel and TrafficShaper class to provide
// end-to-end simulations of the FALCON protocol.
class FalconModel : public FalconModelInterface {
 public:
  FalconModel(const FalconConfig& configuration, Environment* env,
              StatisticCollectionInterface* stats_collector,
              ConnectionManagerInterface* connection_manager,
              std::string_view host_id, uint8_t number_of_hosts);

  int GetVersion() const override {
    CHECK_EQ(configuration_.has_version(), true);
    return configuration_.version();
  }

  void InitiateTransaction(std::unique_ptr<Packet> packet) override;
  void TransferRxPacket(std::unique_ptr<Packet> packet) override;
  void AckTransaction(uint32_t scid, uint32_t rsn, Packet::Syndrome ack_code,
                      absl::Duration rnr_timeout,
                      std::unique_ptr<OpaqueCookie> cookie) override;
  // This is called by the RDMA model when a new QP is set up. If qp_type is RC,
  // This function will store the QpId into the scid connection's metadata.
  uint32_t SetupNewQp(uint32_t scid, QpId qp_id, QpType qp_type,
                      OrderingMode ordering_mode) override;
  // The callback is called by the reorder engine when it releases a packet.
  void ReorderCallback(uint32_t cid, uint32_t rsn,
                       falcon::PacketType type) override;

  void ConnectRdma(RdmaFalconInterface* rdma) { rdma_ = rdma; }
  void ConnectShaper(TrafficShaperInterface* shaper) { shaper_ = shaper; }

  // Connects scid to the (dcid, dst_ip_address).
  absl::Status EstablishConnection(
      uint32_t scid, uint32_t dcid, uint8_t source_bifurcation_id,
      uint8_t destination_bifurcation_id, absl::string_view dst_ip_address,
      OrderingMode ordering_mode,
      const FalconConnectionOptions& connection_options) override;
  // Update rx bytes.
  void UpdateRxBytes(std::unique_ptr<Packet> packet,
                     uint32_t pkt_size_bytes) override;
  // Update tx bytes.
  void UpdateTxBytes(std::unique_ptr<Packet> packet,
                     uint32_t pkt_size_bytes) override;

  // Getter for a pointer to the RDMA model interface.
  RdmaFalconInterface* get_rdma_model() const override { return rdma_; }
  // Getter for a pointer to traffic shaper.
  TrafficShaperInterface* get_traffic_shaper() const override {
    return shaper_;
  }
  // Getters for a pointer to submodules and FALCON configuration.
  ConnectionStateManager* get_state_manager() const override {
    return conn_state_manager_.get();
  }
  ResourceManager* get_resource_manager() const override {
    return resource_manager_.get();
  }
  InterHostRxScheduler* get_inter_host_rx_scheduler() const override {
    return inter_host_rx_scheduler_.get();
  }
  Scheduler* get_connection_scheduler() const override {
    return connection_scheduler_.get();
  }
  Scheduler* get_retransmission_scheduler() const override {
    return retransmission_scheduler_.get();
  }
  Scheduler* get_ack_nack_scheduler() const override {
    return ack_nack_scheduler_.get();
  }
  Arbiter* get_arbiter() const override { return arbiter_.get(); }
  AdmissionControlManager* get_admission_control_manager() const override {
    return admission_control_manager_.get();
  }
  PacketReliabilityManager* get_packet_reliability_manager() const override {
    return packet_reliability_manager_.get();
  }
  RateUpdateEngine* get_rate_update_engine() const override {
    return rate_update_engine_.get();
  }
  BufferReorderEngine* get_buffer_reorder_engine() const override {
    return buffer_reorder_engine_.get();
  }
  AckCoalescingEngineInterface* get_ack_coalescing_engine() const override {
    return ack_coalescing_engine_.get();
  }
  StatsManager* get_stats_manager() const override {
    return stats_manager_.get();
  }
  PacketMetadataTransformer* get_packet_metadata_transformer() const override {
    return packet_metadata_transformer_.get();
  }
  const FalconConfig* get_config() const override { return &configuration_; }
  Environment* get_environment() const override { return env_; }

  StatisticCollectionInterface* get_stats_collector() const override {
    return stats_collector_;
  }
  FalconHistogramCollector* get_histogram_collector() const override {
    return stats_manager_->GetHistogramCollector();
  }
  std::string_view get_host_id() const override { return host_id_; };

  void SetXoffByPacketBuilder(bool xoff) override;
  bool CanSendPacket() const override { return !packet_builder_xoff_; }

  void SetXoffByRdma(uint8_t bifurcation_id, bool xoff) override;

  // [For Testing] Creates the cookie that flows from Falcon to the ULP and then
  // back to Falcon along with a ULP ACK.
  virtual std::unique_ptr<OpaqueCookie> CreateCookieForTesting(
      const Packet& packet);

 protected:
  virtual ConnectionState::ConnectionMetadata CreateConnectionMetadata(
      uint32_t scid, uint32_t dcid, uint8_t source_bifurcation_id,
      uint8_t destination_bifurcation_id, absl::string_view dst_ip_address,
      OrderingMode ordering_mode,
      const FalconConnectionOptions& connection_options);

 private:
  // Creates the cookie that flows from Falcon to the ULP and then back to
  // Falcon along with a ULP ACK.
  virtual std::unique_ptr<OpaqueCookie> CreateCookie(const Packet& packet);
  // Sets the right connection type for a connection to be established based on
  // FalconConfig flags and Falcon version.
  virtual void SetConnectionType(ConnectionState::ConnectionMetadata& metadata);

  const FalconConfig configuration_;
  Environment* const env_;
  StatisticCollectionInterface* const stats_collector_;
  std::string_view host_id_;
  RdmaFalconInterface* rdma_ = nullptr;
  TrafficShaperInterface* shaper_ = nullptr;
  ConnectionManagerInterface* connection_manager_ = nullptr;

  const std::unique_ptr<StatsManager> stats_manager_;
  const std::unique_ptr<ConnectionStateManager> conn_state_manager_;
  const std::unique_ptr<ResourceManager> resource_manager_;
  const std::unique_ptr<InterHostRxScheduler> inter_host_rx_scheduler_;
  const std::unique_ptr<Scheduler> connection_scheduler_;
  const std::unique_ptr<Scheduler> retransmission_scheduler_;
  const std::unique_ptr<Scheduler> ack_nack_scheduler_;
  const std::unique_ptr<Arbiter> arbiter_;
  const std::unique_ptr<AdmissionControlManager> admission_control_manager_;
  const std::unique_ptr<PacketReliabilityManager> packet_reliability_manager_;
  const std::unique_ptr<RateUpdateEngine> rate_update_engine_;
  const std::unique_ptr<BufferReorderEngine> buffer_reorder_engine_;
  const std::unique_ptr<AckCoalescingEngineInterface> ack_coalescing_engine_;
  const std::unique_ptr<PacketMetadataTransformer> packet_metadata_transformer_;

  bool packet_builder_xoff_ = false;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_MODEL_H_
