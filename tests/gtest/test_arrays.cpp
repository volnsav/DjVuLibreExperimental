#include <gtest/gtest.h>

#include "Arrays.h"
#include "GException.h"
#include "GString.h"

TEST(ArraysTest, TArrayResizeInsertDeleteAndBounds)
{
  TArray<int> arr;
  arr.resize(2);
  EXPECT_EQ(3, arr.size());
  EXPECT_EQ(0, arr.lbound());
  EXPECT_EQ(2, arr.hbound());

  arr[0] = 10;
  arr[1] = 20;
  arr[2] = 30;
  arr.ins(1, 99);
  EXPECT_EQ(4, arr.size());
  EXPECT_EQ(99, arr[1]);
  EXPECT_EQ(20, arr[2]);

  arr.del(2);
  EXPECT_EQ(3, arr.size());
  EXPECT_EQ(30, arr[2]);
  EXPECT_THROW((void)arr[10], GException);
}

TEST(ArraysTest, DArrayCopyOnDemandKeepsOriginalUnchanged)
{
  DArray<GUTF8String> src(0, 1);
  src[0] = "left";
  src[1] = "right";

  DArray<GUTF8String> copied(src);
  copied[0] = "changed";

  EXPECT_STREQ("left", (const char *)src[0]);
  EXPECT_STREQ("changed", (const char *)copied[0]);
  EXPECT_STREQ("right", (const char *)copied[1]);
}

TEST(ArraysTest, SortOrdersAscending)
{
  TArray<int> arr(0, 4);
  arr[0] = 7;
  arr[1] = 1;
  arr[2] = 4;
  arr[3] = 2;
  arr[4] = 6;

  arr.sort();
  EXPECT_EQ(1, arr[0]);
  EXPECT_EQ(2, arr[1]);
  EXPECT_EQ(4, arr[2]);
  EXPECT_EQ(6, arr[3]);
  EXPECT_EQ(7, arr[4]);
}
