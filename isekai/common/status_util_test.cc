#include "isekai/common/status_util.h"

#include "absl/status/status.h"
#include "gtest/gtest.h"

namespace isekai {
namespace {

absl::Status TestReturnIfError(absl::Status error_status) {
  absl::Status status1 = absl::OkStatus();
  RETURN_IF_ERROR(status1);

  RETURN_IF_ERROR(error_status) << "error 2";
  return absl::OkStatus();
}

TEST(StatusUtilTest, TestStatusBuilder) {
  absl::Status status = absl::UnknownError("error 1");
  isekai::status_macro_internal::StatusBuilder status_builder = {status};
  absl::Status built_status = status_builder << "error 2" << "error 3";
  EXPECT_EQ(built_status.message(), "error 1; error 2; error 3; ");
  EXPECT_EQ(built_status.code(), status.code());
}

TEST(StatusUtilTest, TestReturnIfError) {
  absl::Status status1 = absl::UnknownError("error 1");
  auto status2 = TestReturnIfError(status1);

  EXPECT_EQ(status2.code(), status1.code());
  EXPECT_EQ(status2.message(), "error 1; error 2; ");
}

}  // namespace
}  // namespace isekai
