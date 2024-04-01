#ifndef ISEKAI_HOST_FALCON_FALCON_RATE_UPDATE_ENGINE_ADAPTER_H_
#define ISEKAI_HOST_FALCON_FALCON_RATE_UPDATE_ENGINE_ADAPTER_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <queue>
#include <string_view>
#include <utility>

#include "absl/status/statusor.h"
#include "isekai/common/config.pb.h"
#include "isekai/common/model_interfaces.h"
#include "isekai/common/packet.h"
#include "isekai/host/falcon/falcon_connection_state.h"
#include "isekai/host/falcon/rue/format.h"

namespace isekai {
// Adapts the Falcon interaction to a custom RUE implementation.
template <typename EventT, typename ResponseT>
class RueAdapterInterface {
 public:
  RueAdapterInterface() = default;
  virtual ~RueAdapterInterface() = default;

  // Disallow copy/assign
  RueAdapterInterface(const RueAdapterInterface&) = delete;
  RueAdapterInterface& operator=(const RueAdapterInterface&) = delete;

  // Processes one RUE Event.
  virtual void ProcessNextEvent(uint32_t now) = 0;

  // Returns the number of queued event in the RUE mailbox.
  virtual int GetNumEvents() const = 0;

  virtual void EnqueueAck(
      const EventT& event, const Packet* packet,
      const ConnectionState::CongestionControlMetadata& ccmeta) = 0;
  virtual void EnqueueNack(
      const EventT& event, const Packet* packet,
      const ConnectionState::CongestionControlMetadata& ccmeta) = 0;
  virtual void EnqueueTimeoutRetransmit(
      const EventT& event, const Packet* packet,
      const ConnectionState::CongestionControlMetadata& ccmeta) = 0;

  virtual std::unique_ptr<ResponseT> DequeueResponse(
      std::function<
          ConnectionState::CongestionControlMetadata&(uint32_t connection_id)>
          ccmeta_lookup) = 0;
  virtual void InitializeMetadata(
      ConnectionState::CongestionControlMetadata& metadata) const = 0;
};

// The RUE Adapter is templatized on (1) the CC algorithm, (2) Event format, and
// (3) Response format. The CC algorithm should implement a `Process` function
// with the following signature:
//   void Process(const EventT& event, ResponseT& response, uint32_t now)
// The RUE Adapter class is responsible for (1) queueing and sending Events to
// the CC algorithm, and (2) accepting responses back from the CC algorithm and
// relaying them back to the RUE class.
template <typename AlgorithmT, typename EventT, typename ResponseT>
class RueAdapter : public RueAdapterInterface<EventT, ResponseT> {
 public:
  explicit RueAdapter(std::unique_ptr<AlgorithmT> algorithm)
      : algorithm_(std::move(algorithm)) {}
  ~RueAdapter() override = default;
  void ProcessNextEvent(uint32_t now) override;
  void EnqueueAck(
      const EventT& event, const Packet* packet,
      const ConnectionState::CongestionControlMetadata& ccmeta) override;
  void EnqueueNack(
      const EventT& event, const Packet* packet,
      const ConnectionState::CongestionControlMetadata& ccmeta) override;
  void EnqueueTimeoutRetransmit(
      const EventT& event, const Packet* packet,
      const ConnectionState::CongestionControlMetadata& ccmetas) override;
  std::unique_ptr<ResponseT> DequeueResponse(
      std::function<
          ConnectionState::CongestionControlMetadata&(uint32_t connection_id)>
          ccmeta_lookup) override;
  int GetNumEvents() const override { return event_queue_.size(); }
  void InitializeMetadata(
      ConnectionState::CongestionControlMetadata& metadata) const override {}

 private:
  std::queue<std::unique_ptr<EventT>> event_queue_;
  std::queue<std::unique_ptr<ResponseT>> response_queue_;
  std::unique_ptr<AlgorithmT> algorithm_;
};

template <typename AlgorithmT, typename EventT, typename ResponseT>
void RueAdapter<AlgorithmT, EventT, ResponseT>::ProcessNextEvent(uint32_t now) {
  // Pulls out the next event and sets a scheduled event to process the queue
  auto event = std::move(event_queue_.front());
  event_queue_.pop();

  // Uses the algorithm implementation to get the RUE response
  auto response_ptr = std::make_unique<ResponseT>();
  response_queue_.push(std::move(response_ptr));

  algorithm_->Process(*event, *(response_queue_.back()), now);
}

template <typename AlgorithmT, typename EventT, typename ResponseT>
void RueAdapter<AlgorithmT, EventT, ResponseT>::EnqueueAck(
    const EventT& event, const Packet* packet,
    const ConnectionState::CongestionControlMetadata& ccmeta) {
  event_queue_.push(std::make_unique<EventT>(event));
}

template <typename AlgorithmT, typename EventT, typename ResponseT>
void RueAdapter<AlgorithmT, EventT, ResponseT>::EnqueueNack(
    const EventT& event, const Packet* packet,
    const ConnectionState::CongestionControlMetadata& ccmeta) {
  event_queue_.push(std::make_unique<EventT>(event));
}

template <typename AlgorithmT, typename EventT, typename ResponseT>
void RueAdapter<AlgorithmT, EventT, ResponseT>::EnqueueTimeoutRetransmit(
    const EventT& event, const Packet* packet,
    const ConnectionState::CongestionControlMetadata& ccmeta) {
  event_queue_.push(std::make_unique<EventT>(event));
}

template <typename AlgorithmT, typename EventT, typename ResponseT>
std::unique_ptr<ResponseT>
RueAdapter<AlgorithmT, EventT, ResponseT>::DequeueResponse(
    std::function<
        ConnectionState::CongestionControlMetadata&(uint32_t connection_id)>
        ccmeta_lookup) {
  auto response = std::move(response_queue_.front());
  response_queue_.pop();
  return response;
}

}  // namespace isekai

#endif  // ISEKAI_HOST_FALCON_FALCON_RATE_UPDATE_ENGINE_ADAPTER_H_
