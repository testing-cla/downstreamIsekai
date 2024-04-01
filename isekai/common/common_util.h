#ifndef ISEKAI_COMMON_COMMON_UTIL_H_
#define ISEKAI_COMMON_COMMON_UTIL_H_

#include <type_traits>

#include "absl/strings/string_view.h"
#include "isekai/common/config.pb.h"

namespace isekai {

template <typename To, typename From>
inline To down_cast(From* f) {
  static_assert((std::is_base_of<From, std::remove_pointer_t<To>>::value),
                "Error: target type is not derived from source type");
  return static_cast<To>(f);
}

constexpr size_t PspHeaderLength = 24;
constexpr size_t PspIntegrityChecksumValueLength = 16;

HostConfigProfile GetHostConfigProfile(absl::string_view host_id,
                                       const NetworkConfig& network);
}  // namespace isekai
#endif  // ISEKAI_COMMON_COMMON_UTIL_H_
