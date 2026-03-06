#include <gtest/gtest.h>

#include "GRect.h"

TEST(GRectTest, DefaultAndSizedConstruction)
{
  GRect empty;
  EXPECT_EQ(0, empty.xmin);
  EXPECT_EQ(0, empty.ymin);
  EXPECT_EQ(0, empty.xmax);
  EXPECT_EQ(0, empty.ymax);
  EXPECT_TRUE(empty.isempty());
  EXPECT_EQ(0, empty.width());
  EXPECT_EQ(0, empty.height());
  EXPECT_EQ(0, empty.area());

  GRect rect(2, 3, 5, 7);
  EXPECT_EQ(2, rect.xmin);
  EXPECT_EQ(3, rect.ymin);
  EXPECT_EQ(7, rect.xmax);
  EXPECT_EQ(10, rect.ymax);
  EXPECT_FALSE(rect.isempty());
  EXPECT_EQ(5, rect.width());
  EXPECT_EQ(7, rect.height());
  EXPECT_EQ(35, rect.area());
}

TEST(GRectTest, ConstructionWithZeroDimensionIsEmpty)
{
  GRect zero_w(2, 3, 0, 5);
  EXPECT_TRUE(zero_w.isempty());
  EXPECT_EQ(0, zero_w.area());

  GRect zero_h(2, 3, 5, 0);
  EXPECT_TRUE(zero_h.isempty());
  EXPECT_EQ(0, zero_h.area());
}

TEST(GRectTest, ContainsPointUsesInclusiveExclusiveBounds)
{
  GRect rect(10, 20, 4, 3); // x:[10,14), y:[20,23)
  EXPECT_TRUE(rect.contains(10, 20));
  EXPECT_TRUE(rect.contains(13, 22));

  EXPECT_FALSE(rect.contains(9, 20));
  EXPECT_FALSE(rect.contains(10, 19));
  EXPECT_FALSE(rect.contains(14, 22));
  EXPECT_FALSE(rect.contains(13, 23));
}

TEST(GRectTest, ClearResetsToCanonicalEmptyRectangle)
{
  GRect rect(3, 4, 5, 6);
  rect.clear();
  EXPECT_TRUE(rect.isempty());
  EXPECT_EQ(0, rect.xmin);
  EXPECT_EQ(0, rect.ymin);
  EXPECT_EQ(0, rect.xmax);
  EXPECT_EQ(0, rect.ymax);
}

TEST(GRectTest, InflateExpandsAndShrinksRectangle)
{
  GRect rect(10, 20, 4, 3);
  EXPECT_EQ(1, rect.inflate(2, 1));
  EXPECT_EQ(8, rect.xmin);
  EXPECT_EQ(16, rect.xmax);
  EXPECT_EQ(19, rect.ymin);
  EXPECT_EQ(24, rect.ymax);
  EXPECT_FALSE(rect.isempty());

  EXPECT_EQ(1, rect.inflate(-1, -1));
  EXPECT_EQ(9, rect.xmin);
  EXPECT_EQ(15, rect.xmax);
  EXPECT_EQ(20, rect.ymin);
  EXPECT_EQ(23, rect.ymax);
}

TEST(GRectTest, InflateToEmptyResetsToCanonicalEmptyRectangle)
{
  GRect rect(1, 2, 2, 2);
  EXPECT_EQ(0, rect.inflate(-2, 0));
  EXPECT_TRUE(rect.isempty());
  EXPECT_EQ(0, rect.xmin);
  EXPECT_EQ(0, rect.ymin);
  EXPECT_EQ(0, rect.xmax);
  EXPECT_EQ(0, rect.ymax);
}

TEST(GRectTest, TranslateMovesRectangle)
{
  GRect rect(2, 3, 5, 7);
  EXPECT_EQ(1, rect.translate(-1, 4));
  EXPECT_EQ(1, rect.xmin);
  EXPECT_EQ(2 + 5 - 1, rect.xmax);
  EXPECT_EQ(7, rect.ymin);
  EXPECT_EQ(3 + 7 + 4, rect.ymax);
}

TEST(GRectTest, TranslateKeepsCanonicalEmptyForEmptyRect)
{
  GRect rect(5, 6, 0, 0);
  EXPECT_EQ(0, rect.translate(10, 20));
  EXPECT_TRUE(rect.isempty());
  EXPECT_EQ(0, rect.xmin);
  EXPECT_EQ(0, rect.ymin);
  EXPECT_EQ(0, rect.xmax);
  EXPECT_EQ(0, rect.ymax);
}

TEST(GRectTest, IntersectProducesOverlap)
{
  GRect a(0, 0, 10, 10);
  GRect b(3, 4, 10, 10);
  GRect out;
  EXPECT_EQ(1, out.intersect(a, b));
  EXPECT_EQ(3, out.xmin);
  EXPECT_EQ(4, out.ymin);
  EXPECT_EQ(10, out.xmax);
  EXPECT_EQ(10, out.ymax);
}

TEST(GRectTest, IntersectOfDisjointOrTouchingRectanglesIsEmpty)
{
  GRect a(0, 0, 2, 2);
  GRect b(3, 3, 2, 2);
  GRect c(2, 0, 2, 2); // touches a on vertical edge
  GRect out;

  EXPECT_EQ(0, out.intersect(a, b));
  EXPECT_TRUE(out.isempty());
  EXPECT_EQ(0, out.xmin);
  EXPECT_EQ(0, out.ymin);
  EXPECT_EQ(0, out.xmax);
  EXPECT_EQ(0, out.ymax);

  EXPECT_EQ(0, out.intersect(a, c));
  EXPECT_TRUE(out.isempty());
  EXPECT_EQ(0, out.xmin);
  EXPECT_EQ(0, out.ymin);
  EXPECT_EQ(0, out.xmax);
  EXPECT_EQ(0, out.ymax);
}

TEST(GRectTest, RecthullCombinesNonEmptyRectangles)
{
  GRect a(2, 3, 2, 2);   // x:[2,4), y:[3,5)
  GRect b(-1, 4, 3, 4);  // x:[-1,2), y:[4,8)
  GRect out;

  EXPECT_EQ(1, out.recthull(a, b));
  EXPECT_EQ(-1, out.xmin);
  EXPECT_EQ(3, out.ymin);
  EXPECT_EQ(4, out.xmax);
  EXPECT_EQ(8, out.ymax);
}

TEST(GRectTest, RecthullWithEmptyOperandReturnsOtherRectangle)
{
  GRect empty(10, 10, 0, 0);
  GRect nonempty(1, 2, 3, 4);
  GRect out;

  EXPECT_EQ(1, out.recthull(empty, nonempty));
  EXPECT_EQ(nonempty.xmin, out.xmin);
  EXPECT_EQ(nonempty.ymin, out.ymin);
  EXPECT_EQ(nonempty.xmax, out.xmax);
  EXPECT_EQ(nonempty.ymax, out.ymax);

  EXPECT_EQ(1, out.recthull(nonempty, empty));
  EXPECT_EQ(nonempty.xmin, out.xmin);
  EXPECT_EQ(nonempty.ymin, out.ymin);
  EXPECT_EQ(nonempty.xmax, out.xmax);
  EXPECT_EQ(nonempty.ymax, out.ymax);
}

TEST(GRectTest, RecthullWithBothEmptyReturnsEmpty)
{
  GRect e1(1, 1, 0, 0);
  GRect e2(2, 2, 0, 0);
  GRect out;

  EXPECT_EQ(0, out.recthull(e1, e2));
  EXPECT_TRUE(out.isempty());
}

TEST(GRectTest, ContainsRectangleUsesIntersectionSemantics)
{
  GRect outer(0, 0, 10, 10);
  GRect inner(2, 3, 4, 5);
  GRect overlapping(8, 8, 4, 4);
  GRect empty(100, 200, 0, 0);

  EXPECT_TRUE(outer.contains(inner));
  EXPECT_FALSE(outer.contains(overlapping));
  EXPECT_TRUE(outer.contains(empty));
}

TEST(GRectTest, UniformScaleScalesAllCoordinates)
{
  GRect rect(2, -3, 6, 8); // xmin=2, ymin=-3, xmax=8, ymax=5
  rect.scale(0.5f);

  EXPECT_EQ(1, rect.xmin);
  EXPECT_EQ(-1, rect.ymin);
  EXPECT_EQ(4, rect.xmax);
  EXPECT_EQ(2, rect.ymax);
}

TEST(GRectTest, AxisScaleScalesCoordinatesIndependently)
{
  GRect rect(-2, 4, 3, 5); // xmin=-2, ymin=4, xmax=1, ymax=9
  rect.scale(2.0f, 0.5f);

  EXPECT_EQ(-4, rect.xmin);
  EXPECT_EQ(2, rect.ymin);
  EXPECT_EQ(2, rect.xmax);
  EXPECT_EQ(4, rect.ymax);
}

TEST(GRectTest, NegativeScaleCanInvertBounds)
{
  GRect rect(1, 2, 3, 4); // xmin=1, ymin=2, xmax=4, ymax=6
  rect.scale(-1.0f);

  EXPECT_EQ(-1, rect.xmin);
  EXPECT_EQ(-2, rect.ymin);
  EXPECT_EQ(-4, rect.xmax);
  EXPECT_EQ(-6, rect.ymax);
  EXPECT_TRUE(rect.isempty());
}
