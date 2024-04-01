#include "isekai/common/simple_environment.h"

#include <string>
#include <vector>

#include "absl/time/time.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "internal/testing.h"

namespace isekai {
namespace {

using testing::ElementsAre;

TEST(SimpleEnvironmentTest, ScheduleEvent) {
  SimpleEnvironment env;
  std::vector<std::string> log;

  EXPECT_OK(env.ScheduleEvent(absl::Seconds(2), [&]() {
    log.push_back("b");
    EXPECT_OK(
        env.ScheduleEvent(absl::Seconds(2), [&]() { log.push_back("d"); }));
  }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(3), [&]() { log.push_back("c"); }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(1), [&]() { log.push_back("a"); }));

  env.Run();
  EXPECT_THAT(log, ElementsAre("a", "b", "c", "d"));
}

TEST(SimpleEnvironmentTest, ElapsedTime) {
  SimpleEnvironment env;
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(2), [&]() {
    EXPECT_EQ(env.ElapsedTime(), absl::Seconds(2));
    EXPECT_OK(env.ScheduleEvent(absl::Seconds(3), [&]() {
      EXPECT_EQ(env.ElapsedTime(), absl::Seconds(5));
    }));
  }));

  env.Run();
}

TEST(SimpleEnvironmentTest, Stop) {
  SimpleEnvironment env;
  int counter = 0;
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(1), [&]() { ++counter; }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(2), [&]() { ++counter; }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(3), [&]() { env.Stop(); }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(4), [&]() { ++counter; }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(5), [&]() { ++counter; }));

  // env.Stop() should break out of this loop at 3 seconds.
  env.Run();

  EXPECT_EQ(counter, 2);
  EXPECT_EQ(env.ElapsedTime(), absl::Seconds(3));
}

TEST(SimpleEnvironmentTest, StableOrder) {
  // Schedule multiple events at the same time, and verify they are executed in
  // the order they were scheduled.
  SimpleEnvironment env;
  std::vector<int> log;

  for (int i = 0; i < 10; ++i) {
    EXPECT_OK(
        env.ScheduleEvent(absl::Seconds(0), [i, &log]() { log.push_back(i); }));
    EXPECT_OK(env.ScheduleEvent(absl::Seconds(1),
                                [i, &log]() { log.push_back(i + 10); }));
  }

  env.Run();

  EXPECT_THAT(log, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                               15, 16, 17, 18, 19));
}

TEST(SimpleEnvironmentTest, RunAndPause) {
  SimpleEnvironment env;
  std::vector<std::string> log;

  // a @ 1s -> e b @ 2s -> c @ 3s -> f d @ 4s -> g @ 5s
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(2), [&]() { log.push_back("e"); }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(2), [&]() {
    log.push_back("b");
    EXPECT_OK(
        env.ScheduleEvent(absl::Seconds(2), [&]() { log.push_back("d"); }));
  }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(3), [&]() { log.push_back("c"); }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(1), [&]() { log.push_back("a"); }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(4), [&]() { log.push_back("f"); }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(5), [&]() { log.push_back("g"); }));

  env.RunFor(absl::Seconds(1));
  EXPECT_THAT(log, ElementsAre("a"));

  env.RunUntil(absl::Seconds(2));
  EXPECT_THAT(log, ElementsAre("a", "e", "b"));

  env.RunFor(absl::Seconds(1));
  EXPECT_THAT(log, ElementsAre("a", "e", "b", "c"));

  env.RunFor(absl::Seconds(1));
  EXPECT_THAT(log, ElementsAre("a", "e", "b", "c", "f", "d"));

  env.RunUntil(absl::Seconds(10));
  EXPECT_THAT(log, ElementsAre("a", "e", "b", "c", "f", "d", "g"));
}

TEST(SimpleEnvironmentTest, TestRunUtil) {
  SimpleEnvironment env;
  std::vector<std::string> log;

  // a @ 1s -> e b @ 2s -> c @ 3s -> d @ 4s
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(2), [&]() { log.push_back("e"); }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(2), [&]() {
    log.push_back("b");
    EXPECT_OK(
        env.ScheduleEvent(absl::Seconds(2), [&]() { log.push_back("d"); }));
  }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(3), [&]() { log.push_back("c"); }));
  EXPECT_OK(env.ScheduleEvent(absl::Seconds(1), [&]() { log.push_back("a"); }));

  env.RunFor(absl::Seconds(1));
  EXPECT_THAT(log, ElementsAre("a"));

  env.RunFor(absl::Seconds(1));
  EXPECT_THAT(log, ElementsAre("a", "e", "b"));

  env.RunFor(absl::Seconds(1));
  EXPECT_THAT(log, ElementsAre("a", "e", "b", "c"));

  env.RunFor(absl::Seconds(1));
  EXPECT_THAT(log, ElementsAre("a", "e", "b", "c", "d"));
}

TEST(SimpleEnvironmentTest, NumEvents) {
  SimpleEnvironment env;
  int counter = 0;

  EXPECT_EQ(env.ScheduledEvents(), 0);
  EXPECT_EQ(env.ExecutedEvents(), 0);

  EXPECT_OK(env.ScheduleEvent(absl::Seconds(1), [&]() { ++counter; }));

  EXPECT_EQ(env.ScheduledEvents(), 1);
  EXPECT_EQ(env.ExecutedEvents(), 0);

  env.Run();

  EXPECT_EQ(env.ScheduledEvents(), 1);
  EXPECT_EQ(env.ExecutedEvents(), 1);

  EXPECT_OK(env.ScheduleEvent(absl::Seconds(1), [&]() { ++counter; }));

  EXPECT_EQ(env.ScheduledEvents(), 2);
  EXPECT_EQ(env.ExecutedEvents(), 1);

  env.Run();

  EXPECT_EQ(env.ScheduledEvents(), 2);
  EXPECT_EQ(env.ExecutedEvents(), 2);
}

}  // namespace
}  // namespace isekai
