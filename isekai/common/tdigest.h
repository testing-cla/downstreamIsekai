#ifndef ISEKAI_OPEN_SOURCE_DEFAULT_TDIGEST_H_
#define ISEKAI_OPEN_SOURCE_DEFAULT_TDIGEST_H_

#include <memory>

#include "folly/stats/TDigest.h"
#include "isekai/common/tdigest.pb.h"

namespace isekai {

// The class is a wrapper of folly::TDigest for open source purpose.
class TDigest {
 public:
  explicit TDigest(double compression) {
    tdigest_ = folly::TDigest(compression);
  }
  static std::unique_ptr<TDigest> New(double compression) {
    return std::make_unique<TDigest>(compression);
  }
  // Add a value to tdigest.
  void Add(double val);
  // Dump the tdigest to proto.
  void ToProto(proto::TDigest* proto);
  // `quantile` can be any real value between 0 and 1.
  inline double Quantile(double quantile) const {
    return tdigest_.estimateQuantile(quantile);
  }
  inline double Min() const { return tdigest_.min(); }
  inline double Max() const { return tdigest_.max(); }
  inline double Sum() const { return tdigest_.sum(); }
  inline double Count() const { return tdigest_.count(); }

 private:
  folly::TDigest tdigest_;
};

}  // namespace isekai

#endif  // ISEKAI_OPEN_SOURCE_DEFAULT_TDIGEST_H_
