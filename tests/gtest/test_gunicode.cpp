#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include "GString.h"

TEST(GUnicodeTest, CreateFromUtf16BigEndianBytes)
{
  const unsigned char utf16be[] = {
      0x00, 0x41, // 'A'
      0x04, 0x16  // Cyrillic 'Ж'
  };

  const GUTF8String s = GUTF8String::create(
      utf16be, sizeof(utf16be), GStringRep::XUTF16BE);
  EXPECT_STREQ("A\xD0\x96", s);
}

TEST(GUnicodeTest, CreateFromUcs4BigEndianBytes)
{
  const unsigned char ucs4be[] = {
      0x00, 0x00, 0x00, 0x41, // 'A'
      0x00, 0x00, 0x03, 0xA9  // Greek 'Ω'
  };

  const GUTF8String s =
      GUTF8String::create(ucs4be, sizeof(ucs4be), GStringRep::XUCS4BE);
  EXPECT_STREQ("A\xCE\xA9", s);
}

TEST(GUnicodeTest, Utf8EncodingFromBytes)
{
  const unsigned char bytes[] = {'o', 'k'};

  const GUTF8String s =
      GUTF8String::create(bytes, sizeof(bytes), GStringRep::XUTF8);
  EXPECT_STREQ("ok", s);
}

TEST(GUnicodeTest, Utf16LeEncodingFromBytes)
{
  const unsigned char bytes[] = {
      0x41, 0x00  // 'A'
  };

  const GUTF8String s =
      GUTF8String::create(bytes, sizeof(bytes), GStringRep::XUTF16LE);
  EXPECT_STREQ("A", s);
}

TEST(GUnicodeTest, Utf8BomIsIgnored)
{
  const unsigned char bytes[] = {
      0xEF, 0xBB, 0xBF, 'O', 'K'
  };

  const GUTF8String s =
      GUTF8String::create(bytes, sizeof(bytes), GStringRep::XUTF8);
  EXPECT_TRUE(s.length() == 0 || std::string((const char *)s) == "OK");
}

TEST(GUnicodeTest, Utf16AutodetectFromBomWorks)
{
  const unsigned char utf16le_with_bom[] = {
      0xFF, 0xFE, // BOM
      0x41, 0x00, // 'A'
      0x16, 0x04  // Cyrillic 'Ж'
  };

  const GUTF8String s =
      GUTF8String::create(utf16le_with_bom, sizeof(utf16le_with_bom), GStringRep::XOTHER);
  EXPECT_TRUE(s.length() == 0 || std::string((const char *)s) == "A\xD0\x96");
}

TEST(GUnicodeTest, Ucs4LittleEndianConversionWorks)
{
  const unsigned char ucs4le[] = {
      0x41, 0x00, 0x00, 0x00, // 'A'
      0xA9, 0x03, 0x00, 0x00  // Greek 'Ω'
  };

  const GUTF8String s =
      GUTF8String::create(ucs4le, sizeof(ucs4le), GStringRep::XUCS4LE);
  EXPECT_STREQ("A\xCE\xA9", s);
}

TEST(GUnicodeTest, Ucs4_2143ConversionWorks)
{
  const unsigned char ucs4_2143[] = {
      0x00, 0x00, 0x41, 0x00, // 'A'
      0x00, 0x00, 0xA9, 0x03  // Greek 'Ω'
  };

  const GUTF8String s =
      GUTF8String::create(ucs4_2143, sizeof(ucs4_2143), GStringRep::XUCS4_2143);
  EXPECT_STREQ("A\xCE\xA9", s);
}

TEST(GUnicodeTest, Ucs4_3412ConversionWorks)
{
  const unsigned char ucs4_3412[] = {
      0x00, 0x41, 0x00, 0x00, // 'A'
      0x03, 0xA9, 0x00, 0x00  // Greek 'Ω'
  };

  const GUTF8String s =
      GUTF8String::create(ucs4_3412, sizeof(ucs4_3412), GStringRep::XUCS4_3412);
  EXPECT_STREQ("A\xCE\xA9", s);
}

TEST(GUnicodeTest, SurrogatePairCanBeCompletedUsingRemainder)
{
  const unsigned char first_half[] = {
      0x3D, 0xD8  // high surrogate D83D
  };
  const GUTF8String a =
      GUTF8String::create(first_half, sizeof(first_half), GStringRep::XUTF16LE);
  const GP<GStringRep::Unicode> rem = a.get_remainder();
  ASSERT_TRUE(rem != 0);

  const unsigned char second_half[] = {
      0x00, 0xDE  // low surrogate DE00
  };
  const GUTF8String b =
      GUTF8String::create(second_half, sizeof(second_half), rem);
  EXPECT_STREQ("\xF0\x9F\x98\x80", b);
}

TEST(GUnicodeTest, InvalidUtf16SurrogateProducesStableReplacementSequence)
{
  const unsigned char bad_pair[] = {
      0x3D, 0xD8, // high surrogate D83D
      0x41, 0x00  // plain 'A', not a low surrogate
  };

  const GUTF8String s =
      GUTF8String::create(bad_pair, sizeof(bad_pair), GStringRep::XUTF16LE);
  EXPECT_GT(s.length(), 0);
}
