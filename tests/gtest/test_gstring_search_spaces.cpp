#include <gtest/gtest.h>

#include "GString.h"

TEST(GStringSearchSpacesTest, NextSpaceAndFirstEndSpace)
{
  GUTF8String s("  one   two\tthree  ");

  EXPECT_EQ(0, s.nextSpace(0, -1));
  EXPECT_EQ(17, s.firstEndSpace(0, -1));  // beginning of trailing spaces
  EXPECT_EQ(5, s.nextSpace(2, -1));       // after "one"
  EXPECT_EQ(17, s.firstEndSpace(5, -1));  // same scan semantics with offset
  EXPECT_EQ(11, s.nextSpace(8, -1));      // tab before "three"
}

TEST(GStringSearchSpacesTest, ContainsAndRcontainsCharacterClasses)
{
  GUTF8String s("abc123xyz");
  EXPECT_EQ(3, s.contains("0123456789"));
  EXPECT_EQ(5, s.rcontains("0123456789"));
  EXPECT_EQ(-1, s.contains("!@#"));
}

TEST(GStringSearchSpacesTest, CmpVariants)
{
  GUTF8String s("prefix-value");
  EXPECT_EQ(0, s.cmp("prefix-value"));
  EXPECT_EQ(0, s.cmp("prefix-zzz", 7)); // compare only "prefix-"
  EXPECT_NE(0, s.cmp("other"));
}
