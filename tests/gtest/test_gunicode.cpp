#include <gtest/gtest.h>

#include <cstdint>

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
