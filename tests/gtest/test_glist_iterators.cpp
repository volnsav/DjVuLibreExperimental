#include <gtest/gtest.h>

#include <vector>

#include "GContainer.h"

TEST(GListIteratorsTest, OldIteratorNextTraversesForward)
{
  GList<int> list;
  list.append(1);
  list.append(2);
  list.append(3);

  GPosition pos;
  list.first(pos);

  std::vector<int> got;
  while (const int *v = list.next(pos))
    got.push_back(*v);

  EXPECT_EQ((std::vector<int>{1, 2, 3}), got);
}

TEST(GListIteratorsTest, OldIteratorPrevTraversesBackward)
{
  GList<int> list;
  list.append(4);
  list.append(5);
  list.append(6);

  GPosition pos;
  list.last(pos);

  std::vector<int> got;
  while (const int *v = list.prev(pos))
    got.push_back(*v);

  EXPECT_EQ((std::vector<int>{6, 5, 4}), got);
}

TEST(GListIteratorsTest, DelWithInvalidPositionIsNoop)
{
  GList<int> list;
  list.append(1);
  list.append(2);

  GPosition invalid;
  list.del(invalid);

  EXPECT_EQ(2, list.size());
}
