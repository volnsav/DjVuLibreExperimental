#include <gtest/gtest.h>

#include <cstring>

#include "GString.h"

TEST(GStringBufferOpsTest, Utf8GetBufCanGrowAndWrite)
{
  GUTF8String s("abc");
  char *buf = s.getbuf(8);
  ASSERT_NE(nullptr, buf);
  std::strcpy(buf, "abcXYZ");
  EXPECT_STREQ("abcXYZ", s);
}

TEST(GStringBufferOpsTest, NativeGetBufCanTruncate)
{
  GNativeString s("hello");
  char *buf = s.getbuf(2);
  ASSERT_NE(nullptr, buf);
  EXPECT_STREQ("he", s);
}

TEST(GStringBufferOpsTest, PlusWithCharsAndStrings)
{
  GUTF8String a("A");
  a += 'B';
  a += "C";
  EXPECT_STREQ("ABC", a);

  GNativeString n("x");
  n += 'y';
  n += "z";
  EXPECT_STREQ("xyz", n);
}
