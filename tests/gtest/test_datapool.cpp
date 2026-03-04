#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DataPool.h"
#include "GException.h"

TEST(DataPoolTest, SequentialAddGetAndLength)
{
  GP<DataPool> pool = DataPool::create();
  ASSERT_TRUE(pool != 0);

  const char payload[] = "abcdef";
  pool->add_data(payload, 6);

  EXPECT_TRUE(pool->has_data(0, 6));
  EXPECT_FALSE(pool->is_eof());

  char out[8] = {0};
  EXPECT_EQ(6, pool->get_data(out, 0, 6));
  EXPECT_STREQ("abcdef", out);

  pool->set_eof();
  EXPECT_TRUE(pool->is_eof());
  EXPECT_EQ(6, pool->get_length());
}

TEST(DataPoolTest, SlaveRangeReadsSubset)
{
  GP<DataPool> master = DataPool::create();
  const char payload[] = "0123456789";
  master->add_data(payload, 10);
  master->set_eof();

  GP<DataPool> slave = DataPool::create(master, 3, 4);
  EXPECT_EQ(4, slave->get_length());

  char out[8] = {0};
  EXPECT_EQ(4, slave->get_data(out, 0, 10));
  EXPECT_STREQ("3456", out);
}

TEST(DataPoolTest, StopMakesFurtherReadsFail)
{
  GP<DataPool> pool = DataPool::create();
  const char payload[] = "xyz";
  pool->add_data(payload, 3);
  pool->set_eof();
  pool->stop(false);

  char out[4] = {0};
  try
  {
    (void)pool->get_data(out, 0, 1);
    FAIL() << "Expected GException";
  }
  catch (const GException &ex)
  {
    EXPECT_EQ(0, ex.cmp_cause(DataPool::Stop));
  }
}
