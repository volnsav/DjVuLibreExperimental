#include <gtest/gtest.h>

#include "GContainer.h"

TEST(GContainerPositionsTest, ListNthCompatibilityOverload)
{
  GList<int> list;
  list.append(5);
  list.append(6);
  list.append(7);

  GPosition pos;
  EXPECT_TRUE(list.nth(1, pos));
  ASSERT_TRUE(pos);
  EXPECT_EQ(6, list[pos]);

  pos = GPosition();
  EXPECT_FALSE(list.nth(10, pos));
  EXPECT_FALSE((bool)pos);
}

TEST(GContainerPositionsTest, MapContainsOverloadSetsPosition)
{
  GMap<int, int> map;
  map[3] = 30;

  GPosition pos;
  ASSERT_TRUE(map.contains(3, pos));
  ASSERT_TRUE(pos);
  EXPECT_EQ(3, map.key(pos));
  EXPECT_EQ(30, map[pos]);
}

TEST(GContainerPositionsTest, DeletingByPositionInvalidatesPosition)
{
  GMap<int, int> map;
  map[1] = 11;
  GPosition pos = map.contains(1);
  ASSERT_TRUE(pos);

  map.del(pos);
  EXPECT_FALSE((bool)pos);
  EXPECT_FALSE((bool)map.contains(1));
}
