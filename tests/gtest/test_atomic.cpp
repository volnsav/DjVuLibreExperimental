#include <gtest/gtest.h>

#include <thread>
#include <vector>

extern "C" {
#include "atomic.h"
}

TEST(AtomicTest, IncrementDecrementExchangeAndCas)
{
  volatile int value = 0;

  EXPECT_EQ(1, atomicIncrement(&value));
  EXPECT_EQ(2, atomicIncrement(&value));
  EXPECT_EQ(1, atomicDecrement(&value));

  EXPECT_EQ(1, atomicCompareAndSwap(&value, 1, 7));
  EXPECT_EQ(7, value);
  EXPECT_EQ(7, atomicCompareAndSwap(&value, 1, 9)); // no swap
  EXPECT_EQ(7, value);

  EXPECT_EQ(7, atomicExchange(&value, 11));
  EXPECT_EQ(11, value);
}

TEST(AtomicTest, ExchangePointerReturnsOldValue)
{
  int a = 1;
  int b = 2;
  void *volatile p = &a;

  void *old = atomicExchangePointer(&p, &b);
  EXPECT_EQ(&a, old);
  EXPECT_EQ(&b, p);
}

TEST(AtomicTest, MultiThreadedIncrementProducesExpectedTotal)
{
  volatile int value = 0;
  constexpr int kThreads = 4;
  constexpr int kIters = 10000;

  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t)
  {
    threads.emplace_back([&]() {
      for (int i = 0; i < kIters; ++i)
        atomicIncrement(&value);
    });
  }

  for (std::thread &th : threads)
    th.join();

  EXPECT_EQ(kThreads * kIters, value);
}
