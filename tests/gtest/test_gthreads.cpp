#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "GThreads.h"

TEST(GThreadsMonitorTest, LeaveWithoutEnterThrows)
{
  GMonitor monitor;
  EXPECT_ANY_THROW(monitor.leave());
}

TEST(GThreadsMonitorTest, ReentrantEnterLeaveWorks)
{
  GMonitor monitor;
  monitor.enter();
  monitor.enter();
  monitor.leave();
  monitor.leave();
}

TEST(GThreadsSafeFlagsTest, BasicBitOperationsAndTestModify)
{
  GSafeFlags flags(0x1);
  EXPECT_EQ(0x1L, (long)flags);

  flags |= 0x2;
  EXPECT_EQ(0x3L, (long)flags);

  flags &= 0x2;
  EXPECT_EQ(0x2L, (long)flags);

  flags.modify(0x4, 0);
  EXPECT_EQ(0x6L, (long)flags);

  EXPECT_TRUE(flags.test_and_modify(0x2, 0x1, 0x8, 0x4));
  EXPECT_EQ(0xAL, (long)flags);
  EXPECT_FALSE(flags.test_and_modify(0x1, 0, 0x10, 0));
  EXPECT_EQ(0xAL, (long)flags);
}

TEST(GThreadsSafeFlagsTest, WaitAndModifyUnblocksWhenMaskSatisfied)
{
  GSafeFlags flags(0);
  std::atomic<bool> finished(false);

  std::thread waiter([&]() {
    flags.wait_and_modify(0x1, 0, 0x2, 0);
    finished.store(true, std::memory_order_release);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  EXPECT_FALSE(finished.load(std::memory_order_acquire));

  flags.modify(0x1, 0);
  waiter.join();

  EXPECT_TRUE(finished.load(std::memory_order_acquire));
  EXPECT_EQ(0x3L, (long)flags);
}

TEST(GThreadsEventTest, SetThenWaitConsumesSignal)
{
  GEvent ev;
  ev.set();
  ev.wait();

  std::atomic<bool> woke(false);
  std::thread producer([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    ev.set();
  });
  ev.wait();
  woke.store(true, std::memory_order_release);
  producer.join();

  EXPECT_TRUE(woke.load(std::memory_order_acquire));
}
