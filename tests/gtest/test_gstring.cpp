#include <gtest/gtest.h>

#include "GString.h"

TEST(GStringTest, BasicOperations)
{
  GUTF8String text("abc");
  EXPECT_EQ(3u, text.length());
  EXPECT_EQ(1, text.contains("b"));
  EXPECT_EQ(1, text.rcontains("b"));
  EXPECT_TRUE(text == "abc");
}
