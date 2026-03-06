#include <gtest/gtest.h>

#include <vector>

#include "GContainer.h"

TEST(GContainerSortSearchTest, ArrayPartialSortOnlyAffectsRequestedRange)
{
  GArray<int> a(0, 5);
  a[0] = 9;
  a[1] = 5;
  a[2] = 4;
  a[3] = 3;
  a[4] = 2;
  a[5] = 1;

  a.sort(1, 4);

  EXPECT_EQ(9, a[0]);
  EXPECT_EQ(2, a[1]);
  EXPECT_EQ(3, a[2]);
  EXPECT_EQ(4, a[3]);
  EXPECT_EQ(5, a[4]);
  EXPECT_EQ(1, a[5]);
}

TEST(GContainerSortSearchTest, ListSearchFromPositionFindsNextMatch)
{
  GList<int> list;
  list.append(1);
  list.append(2);
  list.append(1);
  list.append(3);

  GPosition pos = list.firstpos();
  ASSERT_TRUE(pos);
  EXPECT_EQ(1, list[pos]);
  ++pos; // now points to 2

  EXPECT_TRUE(list.search(1, pos));
  ASSERT_TRUE(pos);
  EXPECT_EQ(1, list[pos]); // should find second "1"
}

TEST(GContainerSortSearchTest, MapDelByMissingKeyIsNoop)
{
  GMap<int, int> m;
  m[1] = 10;
  m[2] = 20;

  m.del(999);
  EXPECT_EQ(2, m.size());
  EXPECT_EQ(10, m[1]);
  EXPECT_EQ(20, m[2]);
}
