#include <gtest/gtest.h>

#include "GString.h"

TEST(GStringMutationTest, SetAtOutOfBoundsThrows)
{
  GUTF8String s("ab");
  EXPECT_ANY_THROW(s.setat(3, 'x'));
  EXPECT_ANY_THROW(s.setat(-3, 'x'));
}

TEST(GStringMutationTest, SubstrNegativeIndexAndFormatting)
{
  GUTF8String s("abcdef");
  EXPECT_STREQ("def", s.substr(-3, 3));
  EXPECT_STREQ("bc", s.substr(1, 2));

  GUTF8String f;
  f.format("%s-%d", "x", 5);
  EXPECT_STREQ("x-5", f);
}

TEST(GStringMutationTest, NumericParsingWithDifferentBases)
{
  GUTF8String s("7f tail");
  int end = -1;
  EXPECT_EQ(127L, s.toLong(0, end, 16));
  EXPECT_EQ(2, end);

  GUTF8String b("10110");
  EXPECT_EQ(22UL, b.toULong(0, end, 2));
  EXPECT_EQ(5, end);
}
