#include "isekai/common/status_util.h"

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "glog/logging.h"

namespace isekai {
namespace status_macro_internal {

void PrintFailure(absl::string_view expression, const absl::Status& status) {
  LOG(FATAL) << absl::StrCat(expression,
                             " returned error: ", status.ToString());
}

}  // namespace status_macro_internal
}  // namespace isekai
