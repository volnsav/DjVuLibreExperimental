#include <gtest/gtest.h>

#include "GRect.h"

TEST(GRectMapperClearTest, ClearRestoresCanonicalIdentityState)
{
  GRectMapper mapper;
  mapper.set_input(GRect(10, 10, 20, 30));
  mapper.set_output(GRect(100, 200, 20, 30));
  mapper.rotate(1);
  mapper.mirrorx();

  mapper.clear();

  GRect input = mapper.get_input();
  GRect output = mapper.get_output();
  EXPECT_EQ(0, input.xmin);
  EXPECT_EQ(0, input.ymin);
  EXPECT_EQ(1, input.width());
  EXPECT_EQ(1, input.height());
  EXPECT_EQ(0, output.xmin);
  EXPECT_EQ(0, output.ymin);
  EXPECT_EQ(1, output.width());
  EXPECT_EQ(1, output.height());

  int x = 0;
  int y = 0;
  mapper.map(x, y);
  EXPECT_EQ(0, x);
  EXPECT_EQ(0, y);
}
