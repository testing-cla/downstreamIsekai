#ifndef ISEKAI_HOST_RDMA_RDMA_CONFIGURATION_H_
#define ISEKAI_HOST_RDMA_RDMA_CONFIGURATION_H_

namespace isekai {

// CC options.
struct RdmaCcOptions {
  enum class Type {
    kDcqcn,
    kNone,
  } type = Type::kDcqcn;
  virtual ~RdmaCcOptions() = default;
};

}  // namespace isekai

#endif  // ISEKAI_HOST_RDMA_RDMA_CONFIGURATION_H_
