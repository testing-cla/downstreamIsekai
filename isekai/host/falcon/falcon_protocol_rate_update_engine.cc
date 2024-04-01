#include "isekai/host/falcon/falcon_protocol_rate_update_engine.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/substitute.h"
#include "absl/time/time.h"
#include "glog/logging.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/environment.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/common/status_util.h"
#include "isekai/host/falcon/event_response_format_adapter.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/falcon_component_interfaces.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/falcon_rate_update_engine_adapter.h"
#include "isekai/host/falcon/falcon_types.h"
#include "isekai/host/falcon/rue/algorithm/swift.h"
#include "isekai/host/falcon/rue/algorithm/swift.pb.h"
#include "isekai/host/falcon/rue/bits.h"
#include "isekai/host/falcon/rue/fixed.h"
#include "isekai/host/falcon/rue/format.h"
#include "isekai/host/falcon/rue/format_bna.h"
#include "isekai/host/falcon/rue/latency_generator.h"

namespace isekai {

using RueConfig = ::isekai::FalconConfig::Rue;

namespace {

// Flag: enable_rue_cc_metrics
constexpr std::string_view kStatVectorRueFabricRttUs =
    "falcon.rue.fabric_rtt_us.cid$0";
constexpr std::string_view kStatVectorRueForwardOwdUs =
    "falcon.rue.forward_owd_us.cid$0";
constexpr std::string_view kStatVectorRueReverseOwdUs =
    "falcon.rue.reverse_owd_us.cid$0";
constexpr std::string_view kStatVectorRueTotalRttUs =
    "falcon.rue.total_rtt_us.cid$0";
constexpr std::string_view kStatVectorRueRxBufferLevel =
    "falcon.rue.rx_buffer_level.cid$0";
constexpr std::string_view kStatVectorRueRetransmitTimeoutUs =
    "falcon.rue.retransmit_timeout_us.cid$0";
constexpr std::string_view kStatVectorRueFabricCwnd =
    "falcon.rue.fabric_cwnd.cid$0";
constexpr std::string_view kStatVectorRueNicCwnd = "falcon.rue.nic_cwnd.cid$0";
constexpr std::string_view kStatVectorPlbRerouteCount =
    "falcon.rue.plb_reroute_count.cid$0";
constexpr std::string_view kStatVectorRueNumAcked =
    "falcon.rue.num_acked_count.cid$0";

// Flag: enable_rue_event_queue_length
constexpr std::string_view kStatVectorRueEventQueueLength =
    "falcon.rue.event_queue_length";

static absl::StatusOr<std::unique_ptr<LatencyGeneratorInterface>>
GetRueProcessingLatencyGenerator(const RueConfig& rue) {
  switch (rue.latency_model_case()) {
    case RueConfig::kGaussianLatencyModel:
      return GaussianLatencyGenerator::Create(
          absl::Nanoseconds(rue.gaussian_latency_model().mean_ns()),
          absl::Nanoseconds(rue.gaussian_latency_model().stddev_ns()),
          absl::Nanoseconds(rue.gaussian_latency_model().min_ns()));
    case RueConfig::kBurstLatencyModel:
      return BurstLatencyGenerator::Create(
          rue.burst_latency_model().interval(),
          absl::Nanoseconds(rue.burst_latency_model().base_ns()),
          absl::Nanoseconds(rue.burst_latency_model().burst_ns()));
    case RueConfig::kFixedLatencyModel:
      return FixedLatencyGenerator::Create(
          absl::Nanoseconds(rue.fixed_latency_model().latency_ns()));
    default:
      return absl::NotFoundError("RUE latency model is unspecified.");
  }
}

}  // namespace

template <typename EventT, typename ResponseT>
ProtocolRateUpdateEngine<EventT, ResponseT>::ProtocolRateUpdateEngine(
    FalconModelInterface* falcon)
    : falcon_(falcon), queue_scheduled_(false) {
  const RueConfig& rue_config = falcon_->get_config()->rue();
  nanoseconds_per_falcon_time_unit_ = rue_config.falcon_unit_time_ns();
  nanoseconds_per_timing_wheel_unit_ = rue_config.tw_unit_time_ns();
  format_adapter_ =
      std::make_unique<EventResponseFormatAdapter<EventT, ResponseT>>(falcon);
  if (rue_config.has_initial_retransmit_timeout_ns()) {
    retransmission_timeout_ = FromFalconTimeUnits(ToFalconTimeUnits(
        absl::Nanoseconds(rue_config.initial_retransmit_timeout_ns())));
  } else {
    // Initialize the retransmission timeout based on the required timeout and
    // Falcon time unit.
    retransmission_timeout_ =
        FromFalconTimeUnits(ToFalconTimeUnits(kDefaultRetransmitTimeout));
  }
  if (rue_config.has_initial_fcwnd()) {
    initial_fcwnd_ = falcon_rue::FloatToFixed<double, uint32_t>(
        rue_config.initial_fcwnd(), falcon_rue::kFractionalBits);
  }
  if (rue_config.has_initial_ncwnd()) {
    initial_ncwnd_ = rue_config.initial_ncwnd();
  }
  if (rue_config.has_delay_select()) {
    switch (rue_config.delay_select()) {
      case FalconConfig::Rue::FULL:
        delay_select_ = falcon::DelaySelect::kFull;
        break;
      case FalconConfig::Rue::FABRIC:
        delay_select_ = falcon::DelaySelect::kFabric;
        break;
      case FalconConfig::Rue::FORWARD:
        delay_select_ = falcon::DelaySelect::kForward;
        break;
      case FalconConfig::Rue::REVERSE:
        delay_select_ = falcon::DelaySelect::kReverse;
        break;
    }
  }
  if (rue_config.has_base_delay_us()) {
    base_delay_ = absl::Microseconds(rue_config.base_delay_us());
  }
  default_falcon_latency_ns_ =
      absl::Nanoseconds(rue_config.falcon_latency_ns());
  event_queue_size_ = rue_config.event_queue_size();
  event_queue_threshold_1_ = rue_config.event_queue_threshold_1();
  event_queue_threshold_2_ = rue_config.event_queue_threshold_2();
  event_queue_threshold_3_ = rue_config.event_queue_threshold_3();
  predicate_1_time_threshold_ =
      absl::Nanoseconds(rue_config.predicate_1_time_threshold_ns());
  predicate_2_packet_count_threshold_ =
      rue_config.predicate_2_packet_count_threshold();
  absl::StatusOr<std::unique_ptr<LatencyGeneratorInterface>>
      rue_processing_latency_gen = GetRueProcessingLatencyGenerator(rue_config);
  CHECK_OK(rue_processing_latency_gen.status());
  rue_processing_latency_gen_ = std::move(*rue_processing_latency_gen);

  // Create the RUE adapter depending on the RUE algorithm.
  const std::string& algorithm = rue_config.algorithm();
  if (algorithm == "swift") {
    auto configuration = rue::Swift<EventT, ResponseT>::DefaultConfiguration();
    // The maximum value for CWNDs.
    configuration.set_max_fabric_congestion_window(1024.0);
    configuration.set_max_nic_congestion_window(1024.0);
    // Set the retransmit timeout value.
    if (rue_config.has_initial_retransmit_timeout_ns()) {
      configuration.set_min_retransmit_timeout(
          std::round(rue_config.initial_retransmit_timeout_ns() /
                     nanoseconds_per_falcon_time_unit_));
    }
    CHECK(rue_config.has_swift())
        << "RUE algorithm set to swift, but no swift configuration provided.";
    if (rue_config.swift().has_max_fcwnd()) {
      configuration.set_max_fabric_congestion_window(
          rue_config.swift().max_fcwnd());
    }
    if (rue_config.swift().has_max_ncwnd()) {
      configuration.set_max_nic_congestion_window(
          rue_config.swift().max_ncwnd());
    }
    if (rue_config.swift().has_randomize_path()) {
      configuration.set_randomize_path(rue_config.swift().randomize_path());
    }
    if (rue_config.has_base_delay_us()) {
      configuration.set_fabric_base_delay(
          ToFalconTimeUnits(absl::Microseconds(rue_config.base_delay_us())));
    }
    if (rue_config.swift().has_plb_target_rtt_multiplier()) {
      configuration.set_plb_target_rtt_multiplier(
          rue_config.swift().plb_target_rtt_multiplier());
    }
    if (rue_config.swift().has_plb_congestion_threshold()) {
      configuration.set_plb_congestion_threshold(
          rue_config.swift().plb_congestion_threshold());
    }
    if (rue_config.swift().has_plb_attempt_threshold()) {
      configuration.set_plb_attempt_threshold(
          rue_config.swift().plb_attempt_threshold());
    }
    if (rue_config.swift().has_target_rx_buffer_level()) {
      configuration.set_target_rx_buffer_level(
          rue_config.swift().target_rx_buffer_level());
    }
    if (rue_config.swift().has_max_flow_scaling()) {
      configuration.set_max_flow_scaling(rue_config.swift().max_flow_scaling());
    }
    if (rue_config.swift().has_max_decrease_on_eack_nack_drop()) {
      configuration.set_max_decrease_on_eack_nack_drop(
          rue_config.swift().max_decrease_on_eack_nack_drop());
    }
    if (rue_config.swift().has_fabric_additive_increment_factor()) {
      configuration.set_fabric_additive_increment_factor(
          rue_config.swift().fabric_additive_increment_factor());
    }
    configuration.set_ipg_time_scalar(
        static_cast<double>(rue_config.falcon_unit_time_ns()) /
        rue_config.tw_unit_time_ns());
    configuration.set_max_flow_scaling_window(
        configuration.max_fabric_congestion_window());

    // Ensuring determinism across runs by providing host id as seed for random
    // flow label generation.
    {
      auto host_id = falcon_->get_host_id();
      std::seed_seq seq(host_id.begin(), host_id.end());
      // As the configuration takes a single integer seed value for random flow
      // label generation, we convert the seed_seq into an integer.
      std::vector<uint32_t> seeds(1);
      seq.generate(seeds.begin(), seeds.end());
      configuration.set_flow_label_rng_seed(seeds[0]);
    }

    rue_adapter_ =
        format_adapter_->GetSwiftRueAdapter(configuration, initial_fcwnd_);
  } else {
    LOG(FATAL) << " Unknown RUE algorithm: " << algorithm;
  }
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::InitializeGenSpecificMetadata(
    ConnectionState::CongestionControlMetadata& metadata) {
  // The following metadata fields are Gen1-specific.
  metadata.flow_label = GenerateRandomFlowLabel();
  metadata.last_rue_event_time = -absl::InfiniteDuration();
  metadata.num_acked = 0;
  metadata.outstanding_rue_event = false;
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::InitializeDelayState(
    ConnectionState::CongestionControlMetadata& metadata) const {
  auto& config = falcon_->get_config()->rue();
  if (config.algorithm() == "swift" && config.swift().randomize_path()) {
    // In this case with Swift and PLB enabled, delay_state is instead used to
    // hold the PLB state. An initial value of 0 for PLB state sets the fields
    // of the isekai::rue::PlbState struct
    // (packets_congestion_acknowledged, packets_acknowledged,
    // plb_reroute_attempted) to 0.
    metadata.delay_state = 0;
  } else {
    metadata.delay_state = ToFalconTimeUnits(kDefaultDelayState);
  }
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::InitializeMetadata(
    ConnectionState::CongestionControlMetadata& metadata) {
  metadata.fabric_congestion_window = initial_fcwnd_;
  InitializeDelayState(metadata);
  metadata.inter_packet_gap = kDefaultInterPacketGap;
  metadata.nic_congestion_window = initial_ncwnd_;
  metadata.retransmit_timeout = retransmission_timeout_;
  metadata.cc_metadata = 0;
  metadata.fabric_window_time_marker = 0;
  metadata.nic_window_time_marker = 0;
  metadata.nic_window_direction = falcon::WindowDirection::kDecrease;
  metadata.delay_select = delay_select_;
  if (falcon_->get_config()->rue().algorithm() == "swift") {
    metadata.base_delay = rue::Swift<EventT, ResponseT>::MakeBaseDelayField(0);
  } else {
    // This is required for non-swift algorithms that use the original
    // definition and format of base_delay (specifically SwiftINT).

    // can remove this else statement.
    metadata.base_delay = ToFalconTimeUnits(base_delay_);
  }
  metadata.rtt_state = ToFalconTimeUnits(kDefaultRttState);
  metadata.cc_opaque = 0;
  metadata.rx_buffer_level = 0;
  // Initialize the Gen-specific fields in congestion control metadata.
  InitializeGenSpecificMetadata(metadata);
  rue_adapter_->InitializeMetadata(metadata);
}

template <typename EventT, typename ResponseT>
std::unique_ptr<RueKey>
ProtocolRateUpdateEngine<EventT, ResponseT>::GetRueKeyFromResponse(
    const ResponseT& response) const {
  return std::make_unique<RueKey>(response.connection_id);
}

template <typename EventT, typename ResponseT>
std::unique_ptr<RueKey>
ProtocolRateUpdateEngine<EventT, ResponseT>::GetRueKeyFromIncomingPacket(
    const Packet* packet) const {
  return falcon_->get_ack_coalescing_engine()
      ->GenerateAckCoalescingKeyFromIncomingPacket(packet);
}

template <typename EventT, typename ResponseT>
absl::Duration ProtocolRateUpdateEngine<EventT, ResponseT>::GetLastEventTime(
    const RueKey* rue_key) const {
  CHECK_OK_THEN_ASSIGN(
      ConnectionState * connection_state,
      falcon_->get_state_manager()->PerformDirectLookup(rue_key->scid));
  return connection_state->congestion_control_metadata.last_rue_event_time;
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::UpdateLastEventTime(
    const RueKey* rue_key) const {
  CHECK_OK_THEN_ASSIGN(
      ConnectionState * connection_state,
      falcon_->get_state_manager()->PerformDirectLookup(rue_key->scid));
  connection_state->congestion_control_metadata.last_rue_event_time =
      falcon_->get_environment()->ElapsedTime();
}

template <typename EventT, typename ResponseT>
bool ProtocolRateUpdateEngine<EventT, ResponseT>::GetOutstandingEvent(
    const RueKey* rue_key) const {
  CHECK_OK_THEN_ASSIGN(
      ConnectionState * connection_state,
      falcon_->get_state_manager()->PerformDirectLookup(rue_key->scid));
  return connection_state->congestion_control_metadata.outstanding_rue_event;
}

template <typename EventT, typename ResponseT>
bool ProtocolRateUpdateEngine<EventT, ResponseT>::UpdateOutstandingEvent(
    const RueKey* rue_key, bool value) const {
  CHECK_OK_THEN_ASSIGN(
      ConnectionState * connection_state,
      falcon_->get_state_manager()->PerformDirectLookup(rue_key->scid));
  bool old_value =
      connection_state->congestion_control_metadata.outstanding_rue_event;
  connection_state->congestion_control_metadata.outstanding_rue_event = value;
  return old_value != value;
}

template <typename EventT, typename ResponseT>
uint32_t ProtocolRateUpdateEngine<EventT, ResponseT>::GetNumAcked(
    const RueKey* rue_key) const {
  CHECK_OK_THEN_ASSIGN(
      ConnectionState * connection_state,
      falcon_->get_state_manager()->PerformDirectLookup(rue_key->scid));
  return connection_state->congestion_control_metadata.num_acked;
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::ResetNumAcked(
    const RueKey* rue_key) const {
  CHECK_OK_THEN_ASSIGN(
      ConnectionState * connection_state,
      falcon_->get_state_manager()->PerformDirectLookup(rue_key->scid));
  connection_state->congestion_control_metadata.num_acked = 0;
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::CollectAckStats(
    const RueKey* rue_key, const Packet* packet) {
  uint32_t cid = rue_key->scid;
  StatisticCollectionInterface* stats_collector =
      falcon_->get_stats_collector();
  bool collect_cc_metrics =
      falcon_->get_stats_manager()->GetStatsConfig().enable_rue_cc_metrics();
  if (stats_collector && collect_cc_metrics) {
    double total_rtt_us = absl::ToDoubleMicroseconds(
        packet->timestamps.received_timestamp - packet->ack.timestamp_1);
    double forward_owd_us = absl::ToDoubleMicroseconds(packet->ack.timestamp_2 -
                                                       packet->ack.timestamp_1);
    double reverse_owd_us =
        absl::ToDoubleMicroseconds(packet->timestamps.received_timestamp -
                                   packet->timestamps.sent_timestamp);
    double fabric_rtt_us = forward_owd_us + reverse_owd_us;
    CHECK_OK(stats_collector->UpdateStatistic(
        absl::Substitute(kStatVectorRueFabricRttUs, cid), fabric_rtt_us,
        StatisticsCollectionConfig::TIME_SERIES_STAT));
    CHECK_OK(stats_collector->UpdateStatistic(
        absl::Substitute(kStatVectorRueForwardOwdUs, cid), forward_owd_us,
        StatisticsCollectionConfig::TIME_SERIES_STAT));
    CHECK_OK(stats_collector->UpdateStatistic(
        absl::Substitute(kStatVectorRueReverseOwdUs, cid), reverse_owd_us,
        StatisticsCollectionConfig::TIME_SERIES_STAT));
    CHECK_OK(stats_collector->UpdateStatistic(
        absl::Substitute(kStatVectorRueTotalRttUs, cid), total_rtt_us,
        StatisticsCollectionConfig::TIME_SERIES_STAT));
    CHECK_OK(stats_collector->UpdateStatistic(
        absl::Substitute(kStatVectorRueRxBufferLevel, cid),
        packet->ack.rx_buffer_level,
        StatisticsCollectionConfig::TIME_SERIES_STAT));
  }
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::CollectNumAckedStats(
    const RueKey* rue_key, uint32_t num_packets_acked) {
  uint32_t cid = rue_key->scid;
  StatisticCollectionInterface* stats_collector =
      falcon_->get_stats_collector();
  bool collect_cc_metrics =
      falcon_->get_stats_manager()->GetStatsConfig().enable_rue_cc_metrics();
  if (stats_collector && collect_cc_metrics) {
    CHECK_OK(stats_collector->UpdateStatistic(
        absl::Substitute(kStatVectorRueNumAcked, cid), num_packets_acked,
        StatisticsCollectionConfig::TIME_SERIES_STAT));
  }
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::
    CollectCongestionControlMetricsAfterResponse(
        const RueKey* rue_key,
        const ConnectionState::CongestionControlMetadata& metadata,
        const ResponseT* response) {
  uint32_t cid = rue_key->scid;
  StatisticCollectionInterface* stats_collector =
      falcon_->get_stats_collector();
  bool collect_cc_metrics =
      falcon_->get_stats_manager()->GetStatsConfig().enable_rue_cc_metrics();
  if (collect_cc_metrics) {
    double fcwnd = falcon_rue::FixedToFloat<uint32_t, double>(
        metadata.fabric_congestion_window, falcon_rue::kFractionalBits);
    CHECK_NE(fcwnd, 0);
    CHECK_OK(stats_collector->UpdateStatistic(
        absl::Substitute(kStatVectorRueFabricCwnd, cid), fcwnd,
        StatisticsCollectionConfig::TIME_SERIES_STAT));

    CHECK_OK(stats_collector->UpdateStatistic(
        absl::Substitute(kStatVectorRueNicCwnd, cid),
        metadata.nic_congestion_window,
        StatisticsCollectionConfig::TIME_SERIES_STAT));

    if (format_adapter_->IsRandomizePath(response)) {
      CHECK_OK(stats_collector->UpdateStatistic(
          absl::Substitute(kStatVectorPlbRerouteCount, cid),
          ++plb_reroute_count[cid],
          StatisticsCollectionConfig::TIME_SERIES_STAT));
    }

    double rto_us = absl::ToDoubleMicroseconds(metadata.retransmit_timeout);
    CHECK_OK(stats_collector->UpdateStatistic(
        absl::Substitute(kStatVectorRueRetransmitTimeoutUs, cid), rto_us,
        StatisticsCollectionConfig::TIME_SERIES_STAT));
  }
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::ExplicitAckReceived(
    const Packet* packet, bool eack, bool eack_drop) {
  CHECK(packet->packet_type == falcon::PacketType::kAck);
  uint32_t cid = packet->ack.dest_cid;
  falcon_rue::CheckBits(falcon_rue::kConnectionIdBits, cid);

  std::unique_ptr<RueKey> rue_key = GetRueKeyFromIncomingPacket(packet);
  // Collect fabric and total RTT samples in stats logging framework.
  CollectAckStats(rue_key.get(), packet);

  if (!CanEnqueueEvent(rue_key.get())) {
    // There is already an outstanding RUE event for this connection or the
    // event queue is full or this event cannot enqueued due to rate limiter.
    // Here we just accumulate the number of packets acknowledged.
    falcon_->get_stats_manager()->UpdateRueDroppedEventCounters(
        cid, falcon::RueEventType::kAck, eack, eack_drop);
  } else {
    // RUE can enqueue an event for this connection. Performs saturating
    // arithmetic and updates RUE state in the connection state.
    uint32_t num_packets_acked = GetNumAcked(rue_key.get());
    num_packets_acked = falcon_rue::SaturateHigh(
        falcon_rue::kNumPacketsAckedBits, num_packets_acked);

    CollectNumAckedStats(rue_key.get(), num_packets_acked);

    // Update last event time and reset num_acked.
    ResetNumAcked(rue_key.get());
    UpdateLastEventTime(rue_key.get());

    CHECK_OK_THEN_ASSIGN(
        auto connection_state,
        falcon_->get_state_manager()->PerformDirectLookup(cid));
    const ConnectionState::CongestionControlMetadata& ccmeta =
        connection_state->congestion_control_metadata;

    EventT event = {};
    format_adapter_->FillExplicitAckEvent(event, rue_key.get(), packet, ccmeta,
                                          num_packets_acked, eack, eack_drop);

    PrepareForEnqueueEvent(event, rue_key.get());
    rue_adapter_->EnqueueAck(event, packet, ccmeta);
  }
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::NackReceived(
    const Packet* packet) {
  CHECK(packet->packet_type == falcon::PacketType::kNack);
  uint32_t cid = packet->nack.dest_cid;
  falcon_rue::CheckBits(falcon_rue::kConnectionIdBits, cid);

  std::unique_ptr<RueKey> rue_key = GetRueKeyFromIncomingPacket(packet);

  if (!CanEnqueueEvent(rue_key.get())) {
    // There is already an outstanding RUE event for this connection or the
    // event queue is full or this event cannot enqueued due to rate limiter.
    // This is a problem because this NACK packet will be completely ignored.
    // However, NACK's are per packet, so if there are more packets afterwards,
    // they should generate NACK as well; if there are no more packet
    // afterwards, then we probably don't care that NACK event got lost.
    falcon_->get_stats_manager()->UpdateRueDroppedEventCounters(
        cid, falcon::RueEventType::kNack,
        /*eack=*/false, /*eack_drop=*/false);
  } else {
    // RUE can enqueue an event for this connection so we combine the current
    // acknowledged amount with the prior acknowledged amount (if any) and issue
    // a new NACK event to the algorithm. Performs saturating arithmetic
    uint32_t num_packets_acked = GetNumAcked(rue_key.get());
    num_packets_acked = falcon_rue::SaturateHigh(
        falcon_rue::kNumPacketsAckedBits, num_packets_acked);

    // Update last event time and reset num_acked.
    ResetNumAcked(rue_key.get());
    UpdateLastEventTime(rue_key.get());

    CHECK_OK_THEN_ASSIGN(
        auto connection_state,
        falcon_->get_state_manager()->PerformDirectLookup(cid));
    const ConnectionState::CongestionControlMetadata& ccmeta =
        connection_state->congestion_control_metadata;

    EventT event = {};
    format_adapter_->FillNackEvent(event, rue_key.get(), packet, ccmeta,
                                   num_packets_acked);

    PrepareForEnqueueEvent(event, rue_key.get());
    rue_adapter_->EnqueueNack(event, packet, ccmeta);
  }
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::PacketTimeoutRetransmitted(
    uint32_t cid, const Packet* packet, uint8_t retransmit_count) {
  auto rue_key = std::make_unique<RueKey>(cid);
  HandlePacketTimeoutRetransmitted(rue_key.get(), packet, retransmit_count);
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::
    HandlePacketTimeoutRetransmitted(const RueKey* rue_key,
                                     const Packet* packet,
                                     uint8_t retransmit_count) {
  CHECK(packet->packet_type != falcon::PacketType::kAck);
  CHECK(packet->packet_type != falcon::PacketType::kNack);
  if (retransmit_count >= 7) {
    retransmit_count = 6;
  }
  falcon_rue::CheckBits(falcon_rue::kDnaRetransmitCountBits, retransmit_count);
  uint32_t cid = rue_key->scid;
  falcon_rue::CheckBits(falcon_rue::kConnectionIdBits, cid);

  // Clears num_acked for this connection.
  ResetNumAcked(rue_key);

  if (!CanEnqueueEvent(rue_key)) {
    // There is already an outstanding RUE event for this connection or the
    // event is full or this event cannot enqueued due to rate limiter. This
    // event will be ignored.
    falcon_->get_stats_manager()->UpdateRueDroppedEventCounters(
        cid, falcon::RueEventType::kRetransmit,
        /*eack=*/false, /*eack_drop=*/false);
  } else {
    // Update last event time.
    UpdateLastEventTime(rue_key);

    CHECK_OK_THEN_ASSIGN(
        auto connection_state,
        falcon_->get_state_manager()->PerformDirectLookup(cid));
    const ConnectionState::CongestionControlMetadata& ccmeta =
        connection_state->congestion_control_metadata;

    EventT event = {};
    format_adapter_->FillTimeoutRetransmittedEvent(event, rue_key, packet,
                                                   ccmeta, retransmit_count);

    PrepareForEnqueueEvent(event, rue_key);
    rue_adapter_->EnqueueTimeoutRetransmit(event, packet, ccmeta);
  }
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::PacketEarlyRetransmitted(
    uint32_t cid, const Packet* packet, uint8_t retransmit_count) {
  //
}

template <typename EventT, typename ResponseT>
uint32_t ProtocolRateUpdateEngine<EventT, ResponseT>::ToFalconTimeUnits(
    absl::Duration time) const {
  uint64_t nanoseconds = time / absl::Nanoseconds(1);
  uint64_t falcon_units = nanoseconds / nanoseconds_per_falcon_time_unit_;
  falcon_rue::CheckBits(falcon_rue::kTimeBits, falcon_units);
  return static_cast<uint32_t>(falcon_units);
}

template <typename EventT, typename ResponseT>
absl::Duration ProtocolRateUpdateEngine<EventT, ResponseT>::FromFalconTimeUnits(
    uint32_t time) const {
  return absl::Nanoseconds(time * nanoseconds_per_falcon_time_unit_);
}

template <typename EventT, typename ResponseT>
uint32_t ProtocolRateUpdateEngine<EventT, ResponseT>::ToTimingWheelTimeUnits(
    absl::Duration time) const {
  uint64_t nanoseconds = time / absl::Nanoseconds(1);
  uint64_t falcon_units = nanoseconds / nanoseconds_per_timing_wheel_unit_;
  falcon_rue::CheckBits(falcon_rue::kInterPacketGapBits, falcon_units);
  return static_cast<uint32_t>(falcon_units);
}

template <typename EventT, typename ResponseT>
absl::Duration
ProtocolRateUpdateEngine<EventT, ResponseT>::FromTimingWheelTimeUnits(
    uint32_t time) const {
  return absl::Nanoseconds(time * nanoseconds_per_timing_wheel_unit_);
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::PrepareForEnqueueEvent(
    const EventT& event, const RueKey* rue_key) {
  CHECK(UpdateOutstandingEvent(rue_key, true));
  if (!queue_scheduled_) {
    queue_scheduled_ = true;
    CHECK_OK(falcon_->get_environment()->ScheduleEvent(
        absl::Nanoseconds(1), [this]() { this->ProcessNextEvent(); }));
  }
  // Update RUE event counters.
  falcon_->get_stats_manager()->UpdateRueEventCounters(
      event.connection_id, event.event_type, event.eack, event.eack_drop);
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::ProcessNextEvent() {
  CHECK(queue_scheduled_);

  // This deschedules queue processing if it ran empty
  if (rue_adapter_->GetNumEvents() == 0) {
    queue_scheduled_ = false;
    return;
  }
  rue_adapter_->ProcessNextEvent(
      ToFalconTimeUnits(falcon_->get_environment()->ElapsedTime()));

  absl::Duration rue_processing_latency_ns =
      rue_processing_latency_gen_->GenerateLatency();
  CHECK_OK(falcon_->get_environment()->ScheduleEvent(
      rue_processing_latency_ns, [this]() { this->ProcessNextEvent(); }));

  absl::Duration falcon_response_latency_ns =
      default_falcon_latency_ns_ + rue_processing_latency_ns;
  CHECK_OK(falcon_->get_environment()->ScheduleEvent(
      falcon_response_latency_ns, [this]() { this->HandleNextResponse(); }));
}

template <typename EventT, typename ResponseT>
void ProtocolRateUpdateEngine<EventT, ResponseT>::HandleNextResponse() {
  // Writes the response information to the connection state
  // and dequeues the response from the response queue.

  auto response = rue_adapter_->DequeueResponse(
      [this](uint32_t connection_id)
          -> ConnectionState::CongestionControlMetadata& {
        CHECK_OK_THEN_ASSIGN(
            auto connection_state,
            falcon_->get_state_manager()->PerformDirectLookup(connection_id));
        return connection_state->congestion_control_metadata;
      });
  std::unique_ptr<RueKey> rue_key = GetRueKeyFromResponse(*response);

  // Update RUE counters.
  falcon_->get_stats_manager()->UpdateRueResponseCounters(
      response->connection_id);

  CHECK_OK_THEN_ASSIGN(auto connection_state,
                       falcon_->get_state_manager()->PerformDirectLookup(
                           response->connection_id));
  auto& congestion_control_metadata =
      connection_state->congestion_control_metadata;

  absl::Duration original_retransmit_timeout =
      congestion_control_metadata.retransmit_timeout;
  // Updates connection_state from the response fields.
  format_adapter_->UpdateConnectionStateFromResponse(connection_state,
                                                     response.get());

  // Record CWND and RTO updates in stats collection framework.
  StatisticCollectionInterface* stats_collector =
      falcon_->get_stats_collector();
  bool collect_event_queue_length = falcon_->get_stats_manager()
                                        ->GetStatsConfig()
                                        .enable_rue_event_queue_length();
  CollectCongestionControlMetricsAfterResponse(
      rue_key.get(), congestion_control_metadata, response.get());
  if (collect_event_queue_length) {
    CHECK_OK(stats_collector->UpdateStatistic(
        kStatVectorRueEventQueueLength, rue_adapter_->GetNumEvents(),
        StatisticsCollectionConfig::TIME_SERIES_STAT));
  }

  // Indicates to the packet reliability manager that RTO has decreased.
  if (congestion_control_metadata.retransmit_timeout <
      original_retransmit_timeout) {
    CHECK_OK(falcon_->get_packet_reliability_manager()->HandleRtoReduction(
        response->connection_id));
  }

  // Disables the outstanding flag so that more events can be generated for
  // this connection.
  CHECK(UpdateOutstandingEvent(rue_key.get(), false));
}

template <typename EventT, typename ResponseT>
uint32_t ProtocolRateUpdateEngine<EventT, ResponseT>::GenerateRandomFlowLabel()
    const {
  uint32_t random32 = (*falcon_->get_environment()->GetPrng())();
  return falcon_rue::ValidBits<uint32_t>(kIpv6FlowLabelNumBits, random32);
}

template <typename EventT, typename ResponseT>
bool ProtocolRateUpdateEngine<EventT, ResponseT>::CanEnqueueEvent(
    const RueKey* rue_key) const {
  falcon_->get_stats_manager()->UpdateRueEnqueueAttempts(rue_key->scid);
  // If an event for this connection is already enqueued, skip.
  //
  // connection.
  if (GetOutstandingEvent(rue_key)) return false;
  // If below threshold 1, always enqueue.
  if (rue_adapter_->GetNumEvents() < event_queue_threshold_1_) return true;
  // Calculate predicate 1.
  absl::Duration time_gap =
      falcon_->get_environment()->ElapsedTime() - GetLastEventTime(rue_key);
  bool predicate_1 = time_gap >= predicate_1_time_threshold_;
  // Calculate predicate 2.
  bool predicate_2 =
      GetNumAcked(rue_key) >= predicate_2_packet_count_threshold_;
  // If [threshold_1, threshold_2), check predicate 1 || predicate 2.
  if (rue_adapter_->GetNumEvents() < event_queue_threshold_2_)
    return predicate_1 || predicate_2;
  // If [threshold_2, threshold_3), check predicate 1.
  if (rue_adapter_->GetNumEvents() < event_queue_threshold_3_)
    return predicate_1;
  // If >= threshold_3, check predicate 1 && predicate 2.
  if (rue_adapter_->GetNumEvents() < event_queue_size_)
    return predicate_1 && predicate_2;
  return false;
}

// Explicit template instantiations.
template class ProtocolRateUpdateEngine<falcon_rue::Event,
                                        falcon_rue::Response>;
template class ProtocolRateUpdateEngine<falcon_rue::Event_BNA,
                                        falcon_rue::Response_BNA>;

}  // namespace isekai
