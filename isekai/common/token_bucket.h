#ifndef ISEKAI_COMMON_TOKEN_BUCKET_H_
#define ISEKAI_COMMON_TOKEN_BUCKET_H_

#include <cstdint>

#include "absl/time/time.h"
namespace isekai {

class TokenBucket {
 public:
  // Constructor that accepts a rate, refill_interval and a max burst size.
  explicit TokenBucket(uint64_t tokens_per_sec,
                       absl::Duration bucket_refill_interval,
                       uint64_t max_burst_size);

  // Verifies if the bucket has the request number of tokens.
  bool AreTokensAvailable(uint64_t tokens, absl::Duration current_time);

  // Reduces the number of tokens per the input parameters. This method should
  // be called only after the AreTokensAvailable method returns true.
  void RequestTokens(uint64_t tokens);

  // Returns the time of last token refill.
  absl::Duration LastRefillTime() const { return last_refill_timestamp_; }

  // Returns the next time of token refill.
  absl::Duration NextRefillTime() const {
    return last_refill_timestamp_ + bucket_refill_interval_;
  }

 private:
  const uint64_t tokens_per_refill_interval_;
  const absl::Duration bucket_refill_interval_;
  const uint64_t bucket_size_;

  uint64_t available_tokens_;
  absl::Duration last_refill_timestamp_;
};

}  // namespace isekai

#endif  // ISEKAI_COMMON_TOKEN_BUCKET_H_
