#include <gtest/gtest.h>

#include <vector>

#include "GContainer.h"

TEST(GContainerCopyTest, ArrayStealTransfersOwnership)
{
  GArray<int> src(0, 2);
  src[0] = 10;
  src[1] = 20;
  src[2] = 30;

  GArray<int> dst;
  dst.steal(src);

  EXPECT_EQ(0, src.size());
  EXPECT_EQ(3, dst.size());
  EXPECT_EQ(10, dst[0]);
  EXPECT_EQ(20, dst[1]);
  EXPECT_EQ(30, dst[2]);
}

TEST(GContainerCopyTest, ListAssignmentCopiesElementsIndependently)
{
  GList<int> a;
  a.append(1);
  a.append(2);

  GList<int> b;
  b = a;
  EXPECT_TRUE(a == b);

  a.append(3);
  EXPECT_FALSE(a == b);

  std::vector<int> bvals;
  for (GPosition p = b.firstpos(); p; ++p)
    bvals.push_back(b[p]);
  EXPECT_EQ((std::vector<int>{1, 2}), bvals);
}

TEST(GContainerCopyTest, MapAssignmentKeepsOriginalEntries)
{
  GMap<int, int> m1;
  m1[1] = 11;
  m1[2] = 22;

  GMap<int, int> m2;
  m2 = m1;
  EXPECT_EQ(2, m2.size());
  EXPECT_EQ(11, m2[1]);
  EXPECT_EQ(22, m2[2]);

  m1.del(2);
  m1[3] = 33;

  EXPECT_EQ(2, m2.size());
  EXPECT_TRUE((bool)m2.contains(2));
  EXPECT_FALSE((bool)m2.contains(3));
}
