#include <gtest/gtest.h>

#include "GString.h"

TEST(GNativeStringTest, NumericConstructorsAndConcatenation)
{
  GNativeString i(42);
  GNativeString d(3.5);
  EXPECT_STREQ("42", i);
  EXPECT_NE(-1, d.search("3.5"));

  GNativeString s("ab");
  s += "cd";
  EXPECT_STREQ("abcd", s);
}

TEST(GNativeStringTest, NativeUtf8RoundtripForAscii)
{
  GNativeString native("Hello_123");
  GUTF8String utf8 = native.getNative2UTF8();
  EXPECT_STREQ("Hello_123", utf8);

  GNativeString back = utf8.getUTF82Native();
  EXPECT_STREQ("Hello_123", back);
}

TEST(GNativeStringTest, SubstrSearchAndCaseConversion)
{
  GNativeString s("AbCdEf");
  EXPECT_STREQ("CdE", s.substr(2, 3));
  EXPECT_EQ(2, s.search("Cd"));
  EXPECT_EQ(3, s.rsearch('d'));
  EXPECT_STREQ("abcdef", s.downcase());
  EXPECT_STREQ("ABCDEF", s.upcase());
}
