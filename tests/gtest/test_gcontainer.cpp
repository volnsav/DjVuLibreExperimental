#include <gtest/gtest.h>

#include <set>
#include <vector>

#include "GContainer.h"

TEST(GContainerArrayTest, ResizeTouchShiftDeleteInsertSort)
{
  GArray<int> arr;
  EXPECT_EQ(0, arr.size());

  arr.touch(3);
  EXPECT_EQ(3, arr.lbound());
  EXPECT_EQ(3, arr.hbound());
  arr[3] = 30;

  arr.resize(2, 5);
  arr[2] = 20;
  arr[4] = 40;
  arr[5] = 50;

  arr.shift(-2);
  EXPECT_EQ(0, arr.lbound());
  EXPECT_EQ(3, arr.hbound());
  EXPECT_EQ(20, arr[0]);
  EXPECT_EQ(30, arr[1]);
  EXPECT_EQ(40, arr[2]);
  EXPECT_EQ(50, arr[3]);

  arr.del(1);
  EXPECT_EQ(3, arr.size());
  EXPECT_EQ(20, arr[0]);
  EXPECT_EQ(40, arr[1]);
  EXPECT_EQ(50, arr[2]);

  arr.ins(1, 35, 1);
  EXPECT_EQ(4, arr.size());
  EXPECT_EQ(35, arr[1]);

  arr.sort();
  EXPECT_EQ(20, arr[0]);
  EXPECT_EQ(35, arr[1]);
  EXPECT_EQ(40, arr[2]);
  EXPECT_EQ(50, arr[3]);
}

TEST(GContainerArrayTest, OutOfBoundsThrows)
{
  GArray<int> arr(0, 0);
  arr[0] = 7;
  EXPECT_ANY_THROW((void)arr[1]);
}

TEST(GContainerListTest, InsertSearchDeleteAndOrder)
{
  GList<int> list;
  list.append(2);
  list.prepend(1);
  list.append(4);

  GPosition pos2 = list.contains(2);
  ASSERT_TRUE(pos2);
  list.insert_after(pos2, 3);
  list.insert_before(list.firstpos(), 0);
  list.insert_before(GPosition(), 5); // null => append

  std::vector<int> values;
  for (GPosition p = list.firstpos(); p; ++p)
    values.push_back(list[p]);
  EXPECT_EQ((std::vector<int>{0, 1, 2, 3, 4, 5}), values);

  GPosition nth3 = list.nth(3);
  ASSERT_TRUE(nth3);
  EXPECT_EQ(3, list[nth3]);

  GPosition p4 = list.contains(4);
  ASSERT_TRUE(p4);
  list.del(p4);
  EXPECT_FALSE((bool)p4);
  EXPECT_FALSE((bool)list.contains(4));
}

TEST(GContainerListTest, MoveNodeBetweenListsWithInsertBefore)
{
  GList<int> src;
  src.append(1);
  src.append(2);
  src.append(3);
  GList<int> dst;
  dst.append(10);
  dst.append(20);

  GPosition from = src.firstpos();
  ++from; // points to 2
  GPosition at = dst.lastpos(); // before 20
  dst.insert_before(at, src, from);

  std::vector<int> src_values;
  for (GPosition p = src.firstpos(); p; ++p)
    src_values.push_back(src[p]);
  EXPECT_EQ((std::vector<int>{1, 3}), src_values);

  std::vector<int> dst_values;
  for (GPosition p = dst.firstpos(); p; ++p)
    dst_values.push_back(dst[p]);
  EXPECT_EQ((std::vector<int>{10, 2, 20}), dst_values);

  ASSERT_TRUE(from);
  EXPECT_EQ(3, src[from]);
}

TEST(GContainerMapTest, CreateReadDeleteAndIterateKeys)
{
  GMap<int, int> m;
  EXPECT_TRUE(m.isempty());

  m[1] = 10;
  m[2] = 20;
  m[3] = 30;
  EXPECT_EQ(3, m.size());
  EXPECT_EQ(20, m[2]);

  const GMap<int, int> &cm = m;
  EXPECT_EQ(30, cm[3]);
  EXPECT_ANY_THROW((void)cm[999]);

  std::set<int> keys;
  for (GPosition p = m.firstpos(); p; ++p)
    keys.insert(m.key(p));
  EXPECT_EQ((std::set<int>{1, 2, 3}), keys);

  m.del(2);
  EXPECT_FALSE((bool)m.contains(2));

  GPosition p1 = m.contains(1);
  ASSERT_TRUE(p1);
  m.del(p1);
  EXPECT_FALSE((bool)p1);
  EXPECT_EQ(1, m.size());
}
