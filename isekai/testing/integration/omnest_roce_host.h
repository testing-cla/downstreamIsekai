#ifndef ISEKAI_TESTING_INTEGRATION_OMNEST_ROCE_HOST_H_
#define ISEKAI_TESTING_INTEGRATION_OMNEST_ROCE_HOST_H_

#include "isekai/host/rnic/omnest_host.h"

// The RoCE host class that integrates real traffic generator, rdma roce,
// and packet builder.
class RoceHost : public OmnestHost {
 protected:
  // The default method in OMNest to initialize the module.
  void initialize() override;
  void initialize(int stage) override;
  // Two stages to initialize Roce host. The first stage is to initiliaze the
  // components, such as rdma roce and packet builder, for Roce host. The second
  // stage is to initialize traffic generator and to generate rdma traffics.
  int numInitStages() const override { return 2; }
};

#endif  // ISEKAI_TESTING_INTEGRATION_OMNEST_ROCE_HOST_H_
