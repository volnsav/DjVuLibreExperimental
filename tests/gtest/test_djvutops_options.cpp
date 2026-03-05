#include <gtest/gtest.h>

#include "DjVuToPS.h"

TEST(DjVuToPSOptionsTest, DefaultsAreSane)
{
  DjVuToPS::Options opt;

  EXPECT_EQ(DjVuToPS::Options::PS, opt.get_format());
  EXPECT_EQ(2, opt.get_level());
  EXPECT_EQ(DjVuToPS::Options::AUTO, opt.get_orientation());
  EXPECT_EQ(DjVuToPS::Options::COLOR, opt.get_mode());
  EXPECT_EQ(0, opt.get_zoom());
  EXPECT_TRUE(opt.get_color());
  EXPECT_TRUE(opt.get_sRGB());
  EXPECT_DOUBLE_EQ(2.2, opt.get_gamma());
  EXPECT_EQ(1, opt.get_copies());
}

TEST(DjVuToPSOptionsTest, SettersAndValidation)
{
  DjVuToPS::Options opt;

  opt.set_format(DjVuToPS::Options::EPS);
  EXPECT_EQ(DjVuToPS::Options::EPS, opt.get_format());

  opt.set_level(3);
  EXPECT_EQ(3, opt.get_level());
  EXPECT_ANY_THROW(opt.set_level(0));
  EXPECT_ANY_THROW(opt.set_level(4));

  opt.set_orientation(DjVuToPS::Options::LANDSCAPE);
  EXPECT_EQ(DjVuToPS::Options::LANDSCAPE, opt.get_orientation());

  opt.set_mode(DjVuToPS::Options::BW);
  EXPECT_EQ(DjVuToPS::Options::BW, opt.get_mode());

  opt.set_zoom(5);
  EXPECT_EQ(5, opt.get_zoom());
  opt.set_zoom(999);
  EXPECT_EQ(999, opt.get_zoom());
  EXPECT_ANY_THROW(opt.set_zoom(4));
  EXPECT_ANY_THROW(opt.set_zoom(1000));

  opt.set_sRGB(false);
  EXPECT_FALSE(opt.get_sRGB());
  opt.set_gamma(0.3);
  EXPECT_DOUBLE_EQ(0.3, opt.get_gamma());
  opt.set_gamma(5.0);
  EXPECT_DOUBLE_EQ(5.0, opt.get_gamma());
  EXPECT_ANY_THROW(opt.set_gamma(0.2));
  EXPECT_ANY_THROW(opt.set_gamma(5.1));

  opt.set_sRGB(true);
  EXPECT_TRUE(opt.get_sRGB());
  EXPECT_DOUBLE_EQ(2.2, opt.get_gamma());

  opt.set_copies(2);
  EXPECT_EQ(2, opt.get_copies());
  EXPECT_ANY_THROW(opt.set_copies(0));
}

TEST(DjVuToPSOptionsTest, BookletHelpers)
{
  DjVuToPS::Options opt;

  opt.set_bookletmode(DjVuToPS::Options::RECTOVERSO);
  EXPECT_EQ(DjVuToPS::Options::RECTOVERSO, opt.get_bookletmode());

  opt.set_bookletmax(0);
  EXPECT_EQ(0, opt.get_bookletmax());
  opt.set_bookletmax(1);
  EXPECT_EQ(4, opt.get_bookletmax());
  opt.set_bookletmax(5);
  EXPECT_EQ(8, opt.get_bookletmax());

  opt.set_bookletalign(11);
  EXPECT_EQ(11, opt.get_bookletalign());

  opt.set_bookletfold(24, 500);
  EXPECT_EQ(24, opt.get_bookletfold(0));
  EXPECT_EQ(25, opt.get_bookletfold(2));

  opt.set_bookletfold(-1, -1);
  EXPECT_EQ(24, opt.get_bookletfold(0));
}
