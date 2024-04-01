#include "isekai/common/tdigest.h"

#include <vector>

#include "folly/stats/TDigest.h"
#include "isekai/common/tdigest.pb.h"

namespace isekai {

void TDigest::Add(double val) {
  std::vector<double> range = {val};
  tdigest_ = tdigest_.merge(range);
}

void TDigest::ToProto(proto::TDigest* proto) {
  proto->Clear();

  proto->set_count(tdigest_.count());
  proto->set_max(tdigest_.max());
  proto->set_min(tdigest_.min());
  proto->set_sum(tdigest_.sum());

  for (const auto& centroid : tdigest_.getCentroids()) {
    auto* new_centroid = proto->add_centroid();
    new_centroid->set_mean(centroid.mean());
    new_centroid->set_weight(centroid.weight());
  }
}

}  // namespace isekai
