#include "isekai/host/falcon/rue/util.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "isekai/host/falcon/falcon.h"
#include "isekai/host/falcon/rue/bits.h"
#include "isekai/host/falcon/rue/format.h"

TEST(Rue, WindowDelta) {
  EXPECT_EQ(falcon_rue::GetWindowDelta(1000, 0), 1000);
  EXPECT_EQ(falcon_rue::GetWindowDelta(1000, 400), 600);
  uint32_t toobig = 1lu << falcon_rue::kTimeBits;
  EXPECT_EQ(falcon_rue::GetWindowDelta(1000, toobig - 100), 1100);
  EXPECT_EQ(falcon_rue::GetWindowDelta(1000, toobig - 1000), 2000);
  EXPECT_EQ(falcon_rue::GetWindowDelta(1000, 1100), toobig - 100);
}

TEST(Rue, PacketTiming) {
  falcon_rue::Event event;

  // straight forward tests
  event.timestamp_1 = 1000;
  event.timestamp_2 = 2000;
  event.timestamp_3 = 2500;
  event.timestamp_4 = 2600;

  falcon_rue::PacketTiming timing;
  timing = {0, 0};
  event.delay_select = falcon::DelaySelect::kFull;
  timing = falcon_rue::GetPacketTiming(event);
  EXPECT_EQ(timing.rtt, 1600);
  EXPECT_EQ(timing.delay, 1600);

  timing = {0, 0};
  event.delay_select = falcon::DelaySelect::kFabric;
  timing = falcon_rue::GetPacketTiming(event);
  EXPECT_EQ(timing.rtt, 1600);
  EXPECT_EQ(timing.delay, 1100);

  timing = {0, 0};
  event.delay_select = falcon::DelaySelect::kForward;
  timing = falcon_rue::GetPacketTiming(event);
  EXPECT_EQ(timing.rtt, 1600);
  EXPECT_EQ(timing.delay, 1000);

  timing = {0, 0};
  event.delay_select = falcon::DelaySelect::kReverse;
  timing = falcon_rue::GetPacketTiming(event);
  EXPECT_EQ(timing.rtt, 1600);
  EXPECT_EQ(timing.delay, 100);

  // high number space tests
  uint32_t toobig = 1lu << falcon_rue::kTimeBits;
  event.timestamp_1 = toobig - 3000;
  event.timestamp_2 = toobig - 2000;
  event.timestamp_3 = toobig - 1500;
  event.timestamp_4 = toobig - 1400;

  timing = {0, 0};
  event.delay_select = falcon::DelaySelect::kFull;
  timing = falcon_rue::GetPacketTiming(event);
  EXPECT_EQ(timing.rtt, 1600);
  EXPECT_EQ(timing.delay, 1600);

  timing = {0, 0};
  event.delay_select = falcon::DelaySelect::kFabric;
  timing = falcon_rue::GetPacketTiming(event);
  EXPECT_EQ(timing.rtt, 1600);
  EXPECT_EQ(timing.delay, 1100);

  timing = {0, 0};
  event.delay_select = falcon::DelaySelect::kForward;
  timing = falcon_rue::GetPacketTiming(event);
  EXPECT_EQ(timing.rtt, 1600);
  EXPECT_EQ(timing.delay, 1000);

  timing = {0, 0};
  event.delay_select = falcon::DelaySelect::kReverse;
  timing = falcon_rue::GetPacketTiming(event);
  EXPECT_EQ(timing.rtt, 1600);
  EXPECT_EQ(timing.delay, 100);

  // kTimeBit wrap around tests
  event.timestamp_1 = toobig - 1100;
  event.timestamp_2 = toobig - 100;
  event.timestamp_3 = 400;
  event.timestamp_4 = 500;

  timing = {0, 0};
  event.delay_select = falcon::DelaySelect::kFull;
  timing = falcon_rue::GetPacketTiming(event);
  EXPECT_EQ(timing.rtt, 1600);
  EXPECT_EQ(timing.delay, 1600);

  timing = {0, 0};
  event.delay_select = falcon::DelaySelect::kFabric;
  timing = falcon_rue::GetPacketTiming(event);
  EXPECT_EQ(timing.rtt, 1600);
  EXPECT_EQ(timing.delay, 1100);

  timing = {0, 0};
  event.delay_select = falcon::DelaySelect::kForward;
  timing = falcon_rue::GetPacketTiming(event);
  EXPECT_EQ(timing.rtt, 1600);
  EXPECT_EQ(timing.delay, 1000);

  timing = {0, 0};
  event.delay_select = falcon::DelaySelect::kReverse;
  timing = falcon_rue::GetPacketTiming(event);
  EXPECT_EQ(timing.rtt, 1600);
  EXPECT_EQ(timing.delay, 100);
}

TEST(Rue, FabricWindowTimeMarker) {
  // straight forward tests
  EXPECT_EQ(falcon_rue::GetFabricWindowTimeMarker(100000, 50000, 40000, 601,
                                                  600, 500),
            100000);  // next < last
  EXPECT_EQ(falcon_rue::GetFabricWindowTimeMarker(100000, 50000, 40000, 500,
                                                  500, 500),
            100000);  // next == MIN
  EXPECT_EQ(falcon_rue::GetFabricWindowTimeMarker(100000, 50000, 40000, 1000,
                                                  1000, 500),
            60000);  // one RTT
  EXPECT_EQ(falcon_rue::GetFabricWindowTimeMarker(100000, 50000, 60000, 1000,
                                                  1000, 500),
            50000);  // no change

  // wrap around tests
  uint32_t toobig = 1lu << falcon_rue::kTimeBits;
  EXPECT_EQ(falcon_rue::GetFabricWindowTimeMarker(25000, 15000000, 40000, 601,
                                                  600, 500),
            25000);  // next < last
  EXPECT_EQ(
      falcon_rue::GetFabricWindowTimeMarker(25000, 50000, 40000, 500, 500, 500),
      25000);  // next == MIN
  uint32_t time_marker = toobig - 25000;
  uint32_t back = toobig - (40000 - 25000);
  EXPECT_EQ(falcon_rue::GetFabricWindowTimeMarker(25000, time_marker, 40000,
                                                  1000, 1000, 500),
            back);  // one RTT
  EXPECT_EQ(falcon_rue::GetFabricWindowTimeMarker(25000, time_marker, 60000,
                                                  1000, 1000, 500),
            time_marker);  // no change
}

TEST(Rue, NicGuardInfo) {
  falcon_rue::NicWindowGuardInfo info;

  // simple increase test
  info = falcon_rue::GetNicWindowGuardInfo(100000, 50000, 40000,
                                           falcon::WindowDirection::kIncrease,
                                           600, 601, 500, 700);
  EXPECT_EQ(info.time_marker, 100000);
  EXPECT_EQ(info.direction, falcon::WindowDirection::kIncrease);

  // simple decrease test
  info = falcon_rue::GetNicWindowGuardInfo(100000, 50000, 40000,
                                           falcon::WindowDirection::kIncrease,
                                           600, 599, 500, 700);
  EXPECT_EQ(info.time_marker, 100000);
  EXPECT_EQ(info.direction, falcon::WindowDirection::kDecrease);

  // maximum window test
  info = falcon_rue::GetNicWindowGuardInfo(100000, 50000, 40000,
                                           falcon::WindowDirection::kIncrease,
                                           700, 700, 500, 700);
  EXPECT_EQ(info.time_marker, 100000);
  EXPECT_EQ(info.direction, falcon::WindowDirection::kIncrease);

  // minimum window test
  info = falcon_rue::GetNicWindowGuardInfo(100000, 50000, 40000,
                                           falcon::WindowDirection::kIncrease,
                                           500, 500, 500, 700);
  EXPECT_EQ(info.time_marker, 100000);
  EXPECT_EQ(info.direction, falcon::WindowDirection::kDecrease);

  // beyond current RTT, last direction was increase
  info = falcon_rue::GetNicWindowGuardInfo(100000, 50000, 40000,
                                           falcon::WindowDirection::kIncrease,
                                           600, 600, 500, 700);
  EXPECT_EQ(info.time_marker, 60000);
  EXPECT_EQ(info.direction, falcon::WindowDirection::kIncrease);

  // beyond current RTT, last direction was decrease
  info = falcon_rue::GetNicWindowGuardInfo(100000, 50000, 40000,
                                           falcon::WindowDirection::kDecrease,
                                           600, 600, 500, 700);
  EXPECT_EQ(info.time_marker, 60000);
  EXPECT_EQ(info.direction, falcon::WindowDirection::kDecrease);

  // within current RTT, last direction was increase
  info = falcon_rue::GetNicWindowGuardInfo(100000, 90000, 40000,
                                           falcon::WindowDirection::kIncrease,
                                           600, 600, 500, 700);
  EXPECT_EQ(info.time_marker, 90000);
  EXPECT_EQ(info.direction, falcon::WindowDirection::kIncrease);

  // within current RTT, last direction was decrease
  info = falcon_rue::GetNicWindowGuardInfo(100000, 90000, 40000,
                                           falcon::WindowDirection::kDecrease,
                                           600, 600, 500, 700);
  EXPECT_EQ(info.time_marker, 90000);
  EXPECT_EQ(info.direction, falcon::WindowDirection::kDecrease);
}
