#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "GThreads.h"

namespace {

struct ThreadState {
  std::atomic<int> ran{0};
  void *thread_id = nullptr;
};

void ThreadEntry(void *arg)
{
  auto *state = static_cast<ThreadState *>(arg);
  state->thread_id = GThread::current();
  state->ran.fetch_add(1, std::memory_order_release);
}

}  // namespace

TEST(GThreadLowLevelTest, CreateRunsEntryAndCurrentIsSet)
{
  ThreadState state;
  GThread thread;

  EXPECT_EQ(0, thread.create(&ThreadEntry, &state));

  for (int i = 0; i < 200 && state.ran.load(std::memory_order_acquire) == 0; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }

  EXPECT_EQ(1, state.ran.load(std::memory_order_acquire));
  EXPECT_NE(nullptr, state.thread_id);
  EXPECT_EQ(0, GThread::yield());
}

TEST(GThreadLowLevelTest, CreateCannotBeCalledTwice)
{
  ThreadState state;
  GThread thread;

  EXPECT_EQ(0, thread.create(&ThreadEntry, &state));
  EXPECT_NE(0, thread.create(&ThreadEntry, &state));

  for (int i = 0; i < 200 && state.ran.load(std::memory_order_acquire) == 0; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  EXPECT_EQ(1, state.ran.load(std::memory_order_acquire));
}

TEST(GCriticalSectionTest, LockProtectsSharedCounter)
{
  GCriticalSection lock;
  int value = 0;

  {
    GCriticalSectionLock guard(&lock);
    value = 1;
  }
  {
    GCriticalSectionLock guard(&lock);
    value += 2;
  }

  lock.lock();
  lock.lock();
  value += 3;
  lock.unlock();
  lock.unlock();

  EXPECT_EQ(6, value);
}
