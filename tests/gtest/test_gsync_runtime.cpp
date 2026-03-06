#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "GThreads.h"

TEST(GSyncRuntimeTest, EventCoordinatesProducerConsumer)
{
  GEvent ready;
  std::atomic<int> value(0);

  std::thread worker([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    value.store(42, std::memory_order_release);
    ready.set();
  });

  ready.wait();
  worker.join();
  EXPECT_EQ(42, value.load(std::memory_order_acquire));
}

TEST(GSyncRuntimeTest, SafeFlagsWaitForFlagsWithConcurrentModify)
{
  GSafeFlags flags(0);
  std::atomic<bool> done(false);

  std::thread waiter([&]() {
    flags.wait_for_flags(0x4);
    done.store(true, std::memory_order_release);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_FALSE(done.load(std::memory_order_acquire));
  flags.modify(0x4, 0);

  waiter.join();
  EXPECT_TRUE(done.load(std::memory_order_acquire));
  EXPECT_EQ(0x4L, (long)flags);
}
