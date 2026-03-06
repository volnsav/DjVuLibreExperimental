#include <gtest/gtest.h>

#include <atomic>

#include "DataPool.h"

namespace {

struct TriggerState {
  std::atomic<int> count{0};
};

void CountTrigger(void *cl_data)
{
  auto *state = static_cast<TriggerState *>(cl_data);
  state->count.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace

TEST(DataPoolTriggerTest, FiresWhenRequestedRangeBecomesAvailable)
{
  GP<DataPool> pool = DataPool::create();
  ASSERT_TRUE(pool != 0);

  TriggerState state;
  pool->add_trigger(0, 4, &CountTrigger, &state);

  pool->add_data("ab", 2);
  EXPECT_EQ(0, state.count.load(std::memory_order_relaxed));

  pool->add_data("cdef", 4);
  EXPECT_EQ(1, state.count.load(std::memory_order_relaxed));

  pool->set_eof();
  EXPECT_EQ(1, state.count.load(std::memory_order_relaxed));
}

TEST(DataPoolTriggerTest, NegativeLengthTriggerFiresOnEof)
{
  GP<DataPool> pool = DataPool::create();
  ASSERT_TRUE(pool != 0);

  TriggerState state;
  pool->add_trigger(0, -1, &CountTrigger, &state);

  pool->add_data("xyz", 3);
  EXPECT_EQ(0, state.count.load(std::memory_order_relaxed));

  pool->set_eof();
  EXPECT_EQ(1, state.count.load(std::memory_order_relaxed));
}

TEST(DataPoolTriggerTest, DeletedTriggerIsNotCalled)
{
  GP<DataPool> pool = DataPool::create();
  ASSERT_TRUE(pool != 0);

  TriggerState state;
  pool->add_trigger(0, 3, &CountTrigger, &state);
  pool->del_trigger(&CountTrigger, &state);

  pool->add_data("abcdef", 6);
  pool->set_eof();

  EXPECT_EQ(0, state.count.load(std::memory_order_relaxed));
}

TEST(DataPoolTriggerTest, SlaveUnknownLengthResolvesAfterMasterEof)
{
  GP<DataPool> master = DataPool::create();
  ASSERT_TRUE(master != 0);
  master->add_data("0123", 4);

  GP<DataPool> slave = DataPool::create(master, 1, -1);
  ASSERT_TRUE(slave != 0);
  EXPECT_EQ(-1, slave->get_length());

  master->add_data("456", 3);
  master->set_eof();

  EXPECT_EQ(6, slave->get_length());
  char out[16] = {0};
  EXPECT_EQ(6, slave->get_data(out, 0, 16));
  EXPECT_STREQ("123456", out);
}

