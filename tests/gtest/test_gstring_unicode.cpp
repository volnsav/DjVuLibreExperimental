#include <gtest/gtest.h>

#include <cstdint>

#include "GString.h"

TEST(GStringUnicodeTest, CreateFromUtf16AndUcs4Buffers)
{
  const uint16_t utf16_data[] = {0x0041, 0x0416}; // "AЖ"
  GUTF8String from_utf16 = GUTF8String::create(utf16_data, 2);
  EXPECT_STREQ("A\xD0\x96", from_utf16);

  const unsigned char ucs4be_data[] = {
      0x00, 0x00, 0x00, 0x42, // 'B'
      0x00, 0x00, 0x03, 0xA9  // 'Ω'
  };
  GUTF8String from_ucs4 =
      GUTF8String::create(ucs4be_data, sizeof(ucs4be_data), GStringRep::XUCS4BE);
  EXPECT_STREQ("B\xCE\xA9", from_ucs4);
}

TEST(GStringUnicodeTest, EscapingAndUnescapingRoundtrip)
{
  GUTF8String plain("<a&b>\"'");
  GUTF8String escaped = plain.toEscaped(false);

  EXPECT_NE(-1, escaped.search("&lt;"));
  EXPECT_NE(-1, escaped.search("&amp;"));
  EXPECT_NE(-1, escaped.search("&gt;"));
  EXPECT_NE(-1, escaped.search("&quot;"));
  EXPECT_NE(-1, escaped.search("&apos;"));

  GUTF8String decoded = escaped.fromEscaped();
  EXPECT_STREQ(plain, decoded);
}

TEST(GStringUnicodeTest, NumericParsingWithPositions)
{
  GUTF8String s1("12345xyz");
  int end = -1;
  long value = s1.toLong(0, end, 10);
  EXPECT_EQ(12345L, value);
  EXPECT_EQ(5, end);

  GUTF8String s2("ff-rest");
  unsigned long uvalue = s2.toULong(0, end, 16);
  EXPECT_EQ(255UL, uvalue);
  EXPECT_EQ(2, end);

  GUTF8String s3("-3.5x");
  double dvalue = s3.toDouble(0, end);
  EXPECT_DOUBLE_EQ(-3.5, dvalue);
  EXPECT_EQ(4, end);
}

TEST(GStringUnicodeTest, SearchAndContainsVariants)
{
  GUTF8String s("abc-xyz-abc");
  EXPECT_EQ(0, s.search("abc"));
  EXPECT_EQ(8, s.rsearch("abc"));
  EXPECT_EQ(3, s.search('-'));
  EXPECT_EQ(7, s.rsearch('-'));

  EXPECT_EQ(4, s.contains("xyz"));
  EXPECT_EQ(10, s.rcontains("abc"));
}
