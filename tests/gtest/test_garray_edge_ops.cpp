#include <gtest/gtest.h>

#include "GContainer.h"

TEST(GArrayEdgeOpsTest, TouchAndResizePreserveValues)
{
  GArray<int> a;
  a.touch(5);
  a[5] = 55;
  a.touch(2);
  a[2] = 22;

  EXPECT_EQ(2, a.lbound());
  EXPECT_EQ(5, a.hbound());
  EXPECT_EQ(22, a[2]);
  EXPECT_EQ(55, a[5]);

  a.resize(1, 6);
  EXPECT_EQ(1, a.lbound());
  EXPECT_EQ(6, a.hbound());
  EXPECT_EQ(22, a[2]);
  EXPECT_EQ(55, a[5]);
}

TEST(GArrayEdgeOpsTest, DelAndInsAtEdges)
{
  GArray<int> a(0, 3);
  a[0] = 10;
  a[1] = 20;
  a[2] = 30;
  a[3] = 40;

  a.del(0);
  EXPECT_EQ(20, a[0]);
  EXPECT_EQ(30, a[1]);
  EXPECT_EQ(40, a[2]);

  a.ins(0, 15, 1);
  EXPECT_EQ(15, a[0]);
  EXPECT_EQ(20, a[1]);
  EXPECT_EQ(30, a[2]);
  EXPECT_EQ(40, a[3]);
}

TEST(GArrayEdgeOpsTest, InvalidDelThrows)
{
  GArray<int> a(0, 1);
  a[0] = 1;
  a[1] = 2;
  EXPECT_ANY_THROW(a.del(2));
  EXPECT_ANY_THROW(a.del(0, -1));
}
