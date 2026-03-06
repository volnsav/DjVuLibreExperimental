#include <gtest/gtest.h>

#include "GString.h"

TEST(GNativeStringBufferTest, GetBufCanTruncateAndAppend)
{
  GNativeString s("abcdef");
  char *buf = s.getbuf(3);
  ASSERT_NE(nullptr, buf);
  EXPECT_STREQ("abc", s);

  s += 'X';
  EXPECT_STREQ("abcX", s);
}

TEST(GNativeStringBufferTest, PlusOperatorsProduceExpectedResults)
{
  GNativeString a("foo");
  GNativeString b("bar");
  GNativeString c = a + b;
  EXPECT_STREQ("foobar", c);

  GNativeString d = ">" + a;
  EXPECT_STREQ(">foo", d);

  GUTF8String u = a + GUTF8String("_utf8");
  EXPECT_STREQ("foo_utf8", u);
}

TEST(GNativeStringBufferTest, SubstrWithNegativeIndex)
{
  GNativeString s("qwerty");
  EXPECT_STREQ("ty", s.substr(-2, 2));
  EXPECT_STREQ("wer", s.substr(1, 3));
}
