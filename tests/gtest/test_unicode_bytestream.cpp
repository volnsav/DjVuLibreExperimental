#include <gtest/gtest.h>

#include <cstring>

#include "ByteStream.h"
#include "GString.h"
#include "UnicodeByteStream.h"

TEST(UnicodeByteStreamTest, GetsSplitsLinesAndCountsReads)
{
  const char payload[] = "line1\nline2\n";
  GP<ByteStream> bs = ByteStream::create(payload, sizeof(payload) - 1);
  GP<UnicodeByteStream> ubs = UnicodeByteStream::create(bs, GStringRep::XUTF8);

  const GUTF8String first = ubs->gets();
  const GUTF8String second = ubs->gets();
  const GUTF8String third = ubs->gets();

  EXPECT_STREQ("line1\n", first);
  EXPECT_STREQ("line2\n", second);
  EXPECT_STREQ("", third);
  EXPECT_EQ(2, ubs->get_lines_read());
}

TEST(UnicodeByteStreamTest, SeekResetsReadAheadBuffer)
{
  const char payload[] = "abcdxyz";
  GP<ByteStream> bs = ByteStream::create(payload, sizeof(payload) - 1);
  GP<UnicodeByteStream> ubs = UnicodeByteStream::create(bs, GStringRep::XUTF8);

  EXPECT_STREQ("abc", ubs->gets(3, 'd', false));
  ASSERT_EQ(0, ubs->seek(0));
  EXPECT_STREQ("abc", ubs->gets(3, 'd', false));
}

TEST(XMLByteStreamTest, Utf8BomIsIgnored)
{
  const unsigned char payload[] = {0xEF, 0xBB, 0xBF, '<', 'a', '/', '>'};
  GP<ByteStream> bs = ByteStream::create(payload, sizeof(payload));
  GP<XMLByteStream> xml = XMLByteStream::create(bs);

  const GUTF8String token = xml->gets(0, '>', true);
  EXPECT_STREQ("<a/>", token);
}

TEST(UnicodeByteStreamTest, WriteReadAndEncodingResetPaths)
{
  GP<ByteStream> bs = ByteStream::create();
  GP<UnicodeByteStream> ubs = UnicodeByteStream::create(bs, GStringRep::XUTF8);

  const char payload[] = "alpha\n";
  ASSERT_EQ(sizeof(payload) - 1, ubs->write(payload, sizeof(payload) - 1));
  ubs->flush();
  ASSERT_EQ(0, ubs->seek(0, SEEK_SET));

  char out[16] = {0};
  ASSERT_EQ(sizeof(payload) - 1, ubs->read(out, sizeof(payload) - 1));
  EXPECT_EQ(0, std::memcmp(payload, out, sizeof(payload) - 1));

  ubs->set_encodetype(GStringRep::XUTF8);
  EXPECT_EQ(0L, ubs->tell());
  EXPECT_STREQ("alpha\n", ubs->gets());

  ubs->set_encoding("UTF-8");
  EXPECT_EQ(0L, ubs->tell());
  EXPECT_STREQ("alpha\n", ubs->gets());
}

TEST(UnicodeByteStreamTest, SetEncodingToUcs4LittleEndianDecodesContent)
{
  const unsigned char payload[] = {
      0x3C, 0x00, 0x00, 0x00,
      0x61, 0x00, 0x00, 0x00,
      0x2F, 0x00, 0x00, 0x00,
      0x3E, 0x00, 0x00, 0x00
  };
  GP<ByteStream> bs = ByteStream::create(payload, sizeof(payload));
  GP<UnicodeByteStream> ubs = UnicodeByteStream::create(bs, GStringRep::XUTF8);

  ubs->set_encoding("UCS-4");
  EXPECT_STREQ("<a/>", ubs->gets(0, '>', true));
}

TEST(XMLByteStreamTest, Utf16LeBomIsDecoded)
{
  const unsigned char payload[] = {
      0xFF, 0xFE, 0x3C, 0x00, 0x61, 0x00, 0x2F, 0x00, 0x3E, 0x00};
  GP<ByteStream> bs = ByteStream::create(payload, sizeof(payload));
  GP<XMLByteStream> xml = XMLByteStream::create(bs);

  const GUTF8String token = xml->gets(0, '>', true);
  EXPECT_STREQ("<a/>", token);
}

TEST(XMLByteStreamTest, Utf16BeBomIsDecoded)
{
  const unsigned char payload[] = {
      0xFE, 0xFF, 0x00, 0x3C, 0x00, 0x61, 0x00, 0x2F, 0x00, 0x3E};
  GP<ByteStream> bs = ByteStream::create(payload, sizeof(payload));
  GP<XMLByteStream> xml = XMLByteStream::create(bs);

  const GUTF8String token = xml->gets(0, '>', true);
  EXPECT_STREQ("<a/>", token);
}

TEST(XMLByteStreamTest, Ucs4BigEndianSignatureIsDetected)
{
  const unsigned char payload[] = {
      0x00, 0x00, 0x00, 0x3C,
      0x00, 0x00, 0x00, 0x61,
      0x00, 0x00, 0x00, 0x2F,
      0x00, 0x00, 0x00, 0x3E
  };
  GP<ByteStream> bs = ByteStream::create(payload, sizeof(payload));
  GP<XMLByteStream> xml = XMLByteStream::create(bs);

  EXPECT_STREQ("<a/>", xml->gets(0, '>', true));
}

TEST(XMLByteStreamTest, Ucs4LittleEndianSignatureIsDetected)
{
  const unsigned char payload[] = {
      0x3C, 0x00, 0x00, 0x00,
      0x61, 0x00, 0x00, 0x00,
      0x2F, 0x00, 0x00, 0x00,
      0x3E, 0x00, 0x00, 0x00
  };
  GP<ByteStream> bs = ByteStream::create(payload, sizeof(payload));
  GP<XMLByteStream> xml = XMLByteStream::create(bs);

  EXPECT_STREQ("<a/>", xml->gets(0, '>', true));
}

TEST(XMLByteStreamTest, Ucs4_2143SignatureIsDetected)
{
  const unsigned char payload[] = {
      0x00, 0x00, 0x3C, 0x00,
      0x00, 0x00, 0x61, 0x00,
      0x00, 0x00, 0x2F, 0x00,
      0x00, 0x00, 0x3E, 0x00
  };
  GP<ByteStream> bs = ByteStream::create(payload, sizeof(payload));
  GP<XMLByteStream> xml = XMLByteStream::create(bs);

  EXPECT_STREQ("<a/>", xml->gets(0, '>', true));
}

TEST(XMLByteStreamTest, Ucs4_3412SignatureIsDetected)
{
  const unsigned char payload[] = {
      0x00, 0x3C, 0x00, 0x00,
      0x00, 0x61, 0x00, 0x00,
      0x00, 0x2F, 0x00, 0x00,
      0x00, 0x3E, 0x00, 0x00
  };
  GP<ByteStream> bs = ByteStream::create(payload, sizeof(payload));
  GP<XMLByteStream> xml = XMLByteStream::create(bs);

  EXPECT_STREQ("<a/>", xml->gets(0, '>', true));
}
