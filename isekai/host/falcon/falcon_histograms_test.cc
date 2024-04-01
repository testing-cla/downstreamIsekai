#include "isekai/host/falcon/falcon_histograms.h"

#include <vector>

#include "gtest/gtest.h"

namespace isekai {
namespace {

constexpr double kValue = 100.0;

TEST(FalconHistogramCollector, TestPushUnsolicited) {
  std::vector<PushUnsolicitedLatencyTypes> types = {
      PushUnsolicitedLatencyTypes::kRxUlpDataToTxNetworkData,
      PushUnsolicitedLatencyTypes::kTxNetworkDataToRxNetworkAck,
      PushUnsolicitedLatencyTypes::kRxNetworkAckToTxUlpCompletion,
      PushUnsolicitedLatencyTypes::kRxNetworkDataToTxUlpData,
      PushUnsolicitedLatencyTypes::kTxUlpDataToRxUlpAck};
  FalconHistogramCollector stats;
  for (const auto& type : types) {
    stats.Add(type, kValue);
    TDigest& added_tdigest = stats.GetTdigest(type);
    ASSERT_EQ(added_tdigest.Count(), 1);
    ASSERT_EQ(added_tdigest.Min(), kValue);
    ASSERT_EQ(added_tdigest.Max(), kValue);
    ASSERT_EQ(added_tdigest.Sum(), kValue);
    ASSERT_EQ(added_tdigest.Quantile(0.5), kValue);
  }
}

TEST(FalconHistogramCollector, TestPushSolicited) {
  std::vector<PushSolicitedLatencyTypes> types = {
      PushSolicitedLatencyTypes::kRxUlpDataToTxNetworkRequest,
      PushSolicitedLatencyTypes::kTxNetworkRequestToRxNetworkGrant,
      PushSolicitedLatencyTypes::kRxNetworkGrantToTxNetworkData,
      PushSolicitedLatencyTypes::kTxNetworkDataToRxNetworkAck,
      PushSolicitedLatencyTypes::kRxNetworkAckToTxUlpCompletion,
      PushSolicitedLatencyTypes::kRxNetworkRequestToTxNetworkGrant,
      PushSolicitedLatencyTypes::kTxNetworkGrantToRxNetworkData,
      PushSolicitedLatencyTypes::kRxNetworkDataToTxUlpData,
      PushSolicitedLatencyTypes::kTxUlpDataToRxUlpAck,
      PushSolicitedLatencyTypes::kTxNetworkGrantToTxUlpData};
  FalconHistogramCollector stats;
  for (const auto& type : types) {
    stats.Add(type, kValue);
    TDigest& added_tdigest = stats.GetTdigest(type);
    ASSERT_EQ(added_tdigest.Count(), 1);
    ASSERT_EQ(added_tdigest.Min(), kValue);
    ASSERT_EQ(added_tdigest.Max(), kValue);
    ASSERT_EQ(added_tdigest.Sum(), kValue);
    ASSERT_EQ(added_tdigest.Quantile(0.5), kValue);
  }
}

TEST(FalconHistogramCollector, TestPull) {
  std::vector<PullLatencyTypes> types = {
      PullLatencyTypes::kRxUlpRequestToTxNetworkRequest,
      PullLatencyTypes::kTxNetworkRequestToRxNetworkData,
      PullLatencyTypes::kRxNetworkDataToTxUlpData,
      PullLatencyTypes::kTxNetworkRequestToTxUlpData,
      PullLatencyTypes::kRxNetworkRequestToTxUlpRequest,
      PullLatencyTypes::kTxUlpRequestToRxUlpRequestAck,
      PullLatencyTypes::kRxUlpRequestAckToRxUlpData,
      PullLatencyTypes::kRxUlpDataToTxNetworkData,
      PullLatencyTypes::kTxNetworkDataToRxNetworkAck};
  FalconHistogramCollector stats;
  for (const auto& type : types) {
    stats.Add(type, kValue);
    TDigest& added_tdigest = stats.GetTdigest(type);
    ASSERT_EQ(added_tdigest.Count(), 1);
    ASSERT_EQ(added_tdigest.Min(), kValue);
    ASSERT_EQ(added_tdigest.Max(), kValue);
    ASSERT_EQ(added_tdigest.Sum(), kValue);
    ASSERT_EQ(added_tdigest.Quantile(0.5), kValue);
  }
}

TEST(FalconHistogramCollector, TestXoff) {
  std::vector<XoffTypes> types = {XoffTypes::kFalcon, XoffTypes::kRdma};
  FalconHistogramCollector stats;
  for (const auto& type : types) {
    stats.Add(type, kValue);
    TDigest& added_tdigest = stats.GetTdigest(type);
    ASSERT_EQ(added_tdigest.Count(), 1);
    ASSERT_EQ(added_tdigest.Min(), kValue);
    ASSERT_EQ(added_tdigest.Max(), kValue);
    ASSERT_EQ(added_tdigest.Sum(), kValue);
    ASSERT_EQ(added_tdigest.Quantile(0.5), kValue);
  }
}

}  // namespace
}  // namespace isekai
