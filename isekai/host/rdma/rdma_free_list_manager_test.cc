#include "isekai/host/rdma/rdma_free_list_manager.h"

#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "internal/testing.h"
#include "isekai/common/config.pb.h"

namespace isekai {
namespace {

TEST(RdmaFreeListManagerTest, TestAllocationAndFree) {
  RdmaConfig config;
  config.set_max_free_list_entries_per_qp(1);
  config.set_total_free_list_entries(2);

  RdmaFreeListManager rflm(config);

  EXPECT_OK(rflm.AllocateResource(1));

  // Should exceed limit of 1 free list entry per QP.
  EXPECT_THAT(rflm.AllocateResource(1),
              absl::ResourceExhaustedError("Not enough QP free list entries."));

  EXPECT_OK(rflm.AllocateResource(2));

  // Should exceed total free list entries.
  EXPECT_THAT(rflm.AllocateResource(3),
              absl::ResourceExhaustedError("Not enough free list entries."));

  EXPECT_OK(rflm.FreeResource(1));

  // Should be okay after previous free.
  EXPECT_OK(rflm.AllocateResource(3));

  EXPECT_OK(rflm.FreeResource(2));
  EXPECT_OK(rflm.FreeResource(3));
}

}  // namespace
}  // namespace isekai
