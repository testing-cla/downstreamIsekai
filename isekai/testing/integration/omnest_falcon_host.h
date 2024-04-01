#ifndef ISEKAI_TESTING_INTEGRATION_OMNEST_FALCON_HOST_H_
#define ISEKAI_TESTING_INTEGRATION_OMNEST_FALCON_HOST_H_

#include "isekai/host/rnic/omnest_host.h"

// The host class that use a fake rdma model to send packets to the network
// directly.
class FalconHost : public OmnestHost {
 public:
  absl::Status ValidateSimulationConfigForTesting(
      const isekai::SimulationConfig& config) {
    return ValidateSimulationConfig(config);
  }

 protected:
  void initialize() override;
  void initialize(int stage) override;
  // Two stages to initialize FALCON host. We need to get RDMA and FALCON
  // registered to connection manager in the first stage, so that in the second
  // stage the connection manager can create QP at both ends. The first stage is
  // to initialize the components, such as rdma, falcon and packet builder, for
  // FALCON host. The second stage is to initialize traffic generator and to
  // generate RDMA traffics.
  int numInitStages() const override { return 2; }

 private:
  // Validates simulation configurations.
  static absl::Status ValidateSimulationConfig(
      const isekai::SimulationConfig& config);
};

#endif  // ISEKAI_TESTING_INTEGRATION_OMNEST_FALCON_HOST_H_
