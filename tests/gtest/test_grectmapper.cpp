#include <gtest/gtest.h>
#include "GRect.h"

TEST(GRectMapperTest, IdentityMap)
{
    GRectMapper mapper;
    GRect rect(10, 20, 30, 40);
    mapper.set_input(rect);
    mapper.set_output(rect);

    int x = 15, y = 25;
    mapper.map(x, y);
    EXPECT_EQ(15, x);
    EXPECT_EQ(25, y);

    GRect r(12, 22, 10, 10);
    mapper.map(r);
    EXPECT_EQ(12, r.xmin);
    EXPECT_EQ(22, r.ymin);
    EXPECT_EQ(22, r.xmax);
    EXPECT_EQ(32, r.ymax);
}

TEST(GRectMapperTest, TranslateMap)
{
    GRectMapper mapper;
    GRect input(0, 0, 10, 10);
    GRect output(100, 200, 10, 10);
    mapper.set_input(input);
    mapper.set_output(output);

    int x = 5, y = 5;
    mapper.map(x, y);
    EXPECT_EQ(105, x);
    EXPECT_EQ(205, y);
}

TEST(GRectMapperTest, ScaleMap)
{
    GRectMapper mapper;
    GRect input(0, 0, 10, 10);
    GRect output(0, 0, 20, 30);
    mapper.set_input(input);
    mapper.set_output(output);

    int x = 5, y = 5;
    mapper.map(x, y);
    EXPECT_EQ(10, x);
    EXPECT_EQ(15, y);
}

TEST(GRectMapperTest, RotateMap)
{
    GRectMapper mapper;
    GRect input(0, 0, 10, 20);
    mapper.set_input(input);
    mapper.set_output(input);
    mapper.rotate(1); // 90 degrees counter-clockwise

    int x = 2, y = 3;
    mapper.map(x, y);
    EXPECT_EQ(9, x);
    EXPECT_EQ(4, y);
}

TEST(GRectMapperTest, MirrorMap)
{
    GRectMapper mapper;
    GRect input(0, 0, 10, 20);
    mapper.set_input(input);
    mapper.set_output(input);
    mapper.mirrorx();

    int x = 3, y = 5;
    mapper.map(x, y);
    EXPECT_EQ(10 - 3, x);
    EXPECT_EQ(5, y);
    
    mapper.mirrory();
    int x2 = 3, y2 = 5;
    mapper.map(x2, y2);
    EXPECT_EQ(10 - 3, x2);
    EXPECT_EQ(20 - 5, y2);
}
