#include <gtest/gtest.h>

#include "GSmartPointer.h"

TEST(GPBufferExtendedTest, ResizeShrinkKeepsPrefixData)
{
  int *buf = nullptr;
  GPBuffer<int> holder(buf, 5);
  ASSERT_NE(nullptr, buf);
  for (int i = 0; i < 5; ++i)
    buf[i] = i + 1;

  holder.resize(3);
  EXPECT_EQ(1, buf[0]);
  EXPECT_EQ(2, buf[1]);
  EXPECT_EQ(3, buf[2]);
}

TEST(GPBufferExtendedTest, ClearSetsAllBytesToZero)
{
  unsigned char *buf = nullptr;
  GPBuffer<unsigned char> holder(buf, 6);
  ASSERT_NE(nullptr, buf);
  holder.set(static_cast<char>(0xAB));
  holder.clear();

  for (int i = 0; i < 6; ++i)
    EXPECT_EQ(0, buf[i]);
}

TEST(GPBufferExtendedTest, SwapExchangesStorage)
{
  int *a = nullptr;
  int *b = nullptr;
  GPBuffer<int> ga(a, 2);
  GPBuffer<int> gb(b, 3);
  a[0] = 10;
  b[0] = 20;

  ga.swap(gb);
  EXPECT_EQ(20, a[0]);
  EXPECT_EQ(10, b[0]);
}
