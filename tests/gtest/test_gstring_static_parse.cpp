#include <gtest/gtest.h>

#include "GString.h"

TEST(GStringStaticParseTest, StaticToLongAndToULong)
{
  GUTF8String s1("255x");
  int end = -1;
  EXPECT_EQ(255L, GBaseString::toLong(s1, 0, end, 10));
  EXPECT_EQ(3, end);

  GNativeString s2("1fZ");
  EXPECT_EQ(31UL, GBaseString::toULong(s2, 0, end, 16));
  EXPECT_EQ(2, end);
}

TEST(GStringStaticParseTest, StaticToDoubleFromUtf8AndNative)
{
  GUTF8String s1("-12.75rest");
  int end = -1;
  EXPECT_DOUBLE_EQ(-12.75, GBaseString::toDouble(s1, 0, end));
  EXPECT_EQ(6, end);

  GNativeString s2("3.125x");
  EXPECT_DOUBLE_EQ(3.125, GBaseString::toDouble(s2, 0, end));
  EXPECT_EQ(5, end);
}
