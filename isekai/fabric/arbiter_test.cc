#include "isekai/fabric/arbiter.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "isekai/common/config.pb.h"
#include "isekai/fabric/routing_test_util.h"
#include "omnetpp/cmessage.h"

namespace isekai {

namespace {

TEST(ArbiterTest, PriorityScheduling) {
  omnetpp::cStaticFlag dummy;
  omnetpp::cSimulation* dummy_simulator = SetupDummyOmnestSimulation();

  auto arbiter = CreateArbiter(RouterConfigProfile::FIXED_PRIORITY);
  std::vector<const omnetpp::cMessage*> packet_list(3, nullptr);
  EXPECT_EQ(-1, arbiter->Arbitrate(packet_list));

  omnetpp::cMessage packet;
  packet_list[0] = &packet;
  EXPECT_EQ(0, arbiter->Arbitrate(packet_list));

  packet_list[0] = nullptr;
  packet_list[1] = &packet;
  packet_list[2] = &packet;
  EXPECT_EQ(1, arbiter->Arbitrate(packet_list));

  CloseOmnestSimulation(dummy_simulator);
}

}  // namespace

}  // namespace isekai
