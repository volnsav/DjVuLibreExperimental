#include <gtest/gtest.h>

#include "GException.h"
#include "GMapAreas.h"

TEST(GMapAreasTest, RectInvalidXorBorderWidthFailsValidation)
{
  GP<GMapRect> rect = GMapRect::create(GRect(2, 3, 20, 10));
  rect->border_type = GMapArea::XOR_BORDER;
  rect->border_width = 2;

  EXPECT_STRNE("", rect->check_object());
  EXPECT_THROW((void)rect->print(), GException);
}

TEST(GMapAreasTest, SelfIntersectingPolyFailsValidation)
{
  const int xx[] = {0, 10, 0, 10};
  const int yy[] = {0, 10, 10, 0};
  EXPECT_ANY_THROW((void)GMapPoly::create(xx, yy, 4, false));
}

TEST(GMapAreasTest, OpenPolyNeverContainsPoints)
{
  const int xx[] = {0, 20, 20};
  const int yy[] = {0, 0, 20};
  GP<GMapPoly> poly = GMapPoly::create(xx, yy, 3, true);
  EXPECT_FALSE(poly->is_point_inside(10, 5));
}

TEST(GMapAreasTest, OvalShadowBorderWidthRangeIsChecked)
{
  GP<GMapOval> oval = GMapOval::create(GRect(0, 0, 30, 18));
  oval->border_type = GMapArea::SHADOW_EIN_BORDER;
  oval->border_width = 2;
  EXPECT_STRNE("", oval->check_object());
}

TEST(GMapAreasTest, RectGeometryMoveResizeTransformAndXml)
{
  GP<GMapRect> rect = GMapRect::create(GRect(10, 20, 30, 40));
  ASSERT_TRUE(rect != 0);
  EXPECT_EQ(GMapArea::RECT, rect->get_shape_type());
  EXPECT_STREQ("rect", rect->get_shape_name());
  EXPECT_TRUE(rect->is_point_inside(15, 25));
  EXPECT_FALSE(rect->is_point_inside(100, 100));

  rect->move(5, -10);
  EXPECT_EQ(15, rect->get_xmin());
  EXPECT_EQ(10, rect->get_ymin());

  rect->resize(60, 20);
  EXPECT_EQ(75, rect->get_xmax());
  EXPECT_EQ(30, rect->get_ymax());

  rect->transform(GRect(0, 0, 12, 8));
  EXPECT_EQ(0, rect->get_xmin());
  EXPECT_EQ(0, rect->get_ymin());
  EXPECT_EQ(12, rect->get_xmax());
  EXPECT_EQ(8, rect->get_ymax());

  GRectMapper mapper;
  mapper.set_input(GRect(0, 0, 12, 8));
  mapper.set_output(GRect(100, 200, 24, 16));
  rect->map(mapper);
  EXPECT_EQ(100, rect->get_xmin());
  EXPECT_EQ(200, rect->get_ymin());
  rect->unmap(mapper);
  EXPECT_EQ(0, rect->get_xmin());
  EXPECT_EQ(0, rect->get_ymin());

  rect->url = "https://example.com";
  rect->target = "_blank";
  rect->comment = "hint";
  rect->border_type = GMapArea::SOLID_BORDER;
  rect->border_width = 1;
  rect->border_color = 0x00112233;
  rect->hilite_color = 0x00aabbcc;
  EXPECT_STREQ("", rect->check_object());
  EXPECT_FALSE(rect->print().length() == 0);
  EXPECT_FALSE(rect->get_xmltag(100).length() == 0);

  GP<GMapArea> copy = rect->get_copy();
  ASSERT_TRUE(copy != 0);
  EXPECT_EQ(GMapArea::RECT, copy->get_shape_type());
}

TEST(GMapAreasTest, PolyGeometryAndMappingOperations)
{
  const int xx[] = {0, 20, 20, 0};
  const int yy[] = {0, 0, 20, 20};
  GP<GMapPoly> poly = GMapPoly::create(xx, yy, 4, false);
  ASSERT_TRUE(poly != 0);
  EXPECT_EQ(GMapArea::POLY, poly->get_shape_type());
  EXPECT_STREQ("poly", poly->get_shape_name());
  EXPECT_EQ(4, poly->get_points_num());
  EXPECT_EQ(4, poly->get_sides_num());
  EXPECT_TRUE(poly->is_point_inside(10, 10));
  EXPECT_FALSE(poly->is_point_inside(30, 30));
  EXPECT_STREQ("", poly->check_data());

  poly->move_vertex(0, 1, 1);
  EXPECT_EQ(1, poly->get_x(0));
  EXPECT_EQ(1, poly->get_y(0));

  EXPECT_EQ(5, poly->add_vertex(10, 30));
  poly->close_poly();
  poly->optimize_data();

  GList<int> coords;
  poly->get_coords(coords);
  EXPECT_GT(coords.size(), 0);

  EXPECT_TRUE(poly->does_side_cross_rect(GRect(5, 5, 8, 8), 0) ||
              !poly->does_side_cross_rect(GRect(5, 5, 8, 8), 0));

  GRectMapper mapper;
  mapper.set_input(GRect(0, 0, 40, 40));
  mapper.set_output(GRect(100, 100, 80, 80));
  poly->map(mapper);
  poly->unmap(mapper);

  EXPECT_FALSE(poly->print().length() == 0);
  EXPECT_FALSE(poly->get_xmltag(200).length() == 0);
}

TEST(GMapAreasTest, OvalGeometryAndCopy)
{
  GP<GMapOval> oval = GMapOval::create(GRect(2, 4, 30, 18));
  ASSERT_TRUE(oval != 0);
  EXPECT_EQ(GMapArea::OVAL, oval->get_shape_type());
  EXPECT_STREQ("oval", oval->get_shape_name());
  EXPECT_TRUE(oval->is_point_inside(15, 10));

  oval->move(3, 2);
  EXPECT_EQ(5, oval->get_xmin());
  EXPECT_EQ(6, oval->get_ymin());

  oval->resize(20, 10);
  EXPECT_EQ(25, oval->get_xmax());
  EXPECT_EQ(16, oval->get_ymax());

  oval->transform(GRect(0, 0, 12, 12));
  EXPECT_EQ(0, oval->get_xmin());
  EXPECT_EQ(0, oval->get_ymin());

  GRectMapper mapper;
  mapper.set_input(GRect(0, 0, 12, 12));
  mapper.set_output(GRect(20, 40, 24, 24));
  oval->map(mapper);
  EXPECT_EQ(20, oval->get_xmin());
  oval->unmap(mapper);
  EXPECT_EQ(0, oval->get_xmin());

  oval->border_type = GMapArea::NO_BORDER;
  oval->border_width = 1;
  EXPECT_STREQ("", oval->check_object());
  EXPECT_FALSE(oval->print().length() == 0);
  EXPECT_FALSE(oval->get_xmltag(120).length() == 0);

  GP<GMapArea> copy = oval->get_copy();
  ASSERT_TRUE(copy != 0);
  EXPECT_EQ(GMapArea::OVAL, copy->get_shape_type());
}
