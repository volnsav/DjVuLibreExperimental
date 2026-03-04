#include <gtest/gtest.h>

#include "GException.h"
#include "GRect.h"

TEST(GRectMapperInverseTest, MapThenUnmapRestoresPointAfterTransforms)
{
  GRectMapper mapper;
  mapper.set_input(GRect(10, 20, 30, 40));
  mapper.set_output(GRect(-50, 70, 30, 40));
  mapper.rotate(1);
  mapper.mirrorx();
  mapper.mirrory();

  int x = 25;
  int y = 35;
  const int original_x = x;
  const int original_y = y;

  mapper.map(x, y);
  mapper.unmap(x, y);

  EXPECT_EQ(original_x, x);
  EXPECT_EQ(original_y, y);
}

TEST(GRectMapperInverseTest, MapThenUnmapRestoresRectangle)
{
  GRectMapper mapper;
  mapper.set_input(GRect(0, 0, 20, 10));
  mapper.set_output(GRect(100, 200, 20, 10));

  GRect rect(2, 3, 7, 4);
  const GRect original = rect;

  mapper.map(rect);
  mapper.unmap(rect);

  EXPECT_EQ(original.xmin, rect.xmin);
  EXPECT_EQ(original.ymin, rect.ymin);
  EXPECT_EQ(original.xmax, rect.xmax);
  EXPECT_EQ(original.ymax, rect.ymax);
}

TEST(GRectMapperInverseTest, EmptyRectanglesAreRejected)
{
  GRectMapper mapper;
  EXPECT_THROW(mapper.set_input(GRect(0, 0, 0, 10)), GException);
  EXPECT_THROW(mapper.set_output(GRect(0, 0, 10, 0)), GException);
}
