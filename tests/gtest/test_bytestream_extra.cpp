#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <string>

#include "ByteStream.h"
#include "GException.h"
#include "GString.h"
#include "GURL.h"

TEST(ByteStreamExtraTest, EndianReadWriteRoundtrip)
{
  GP<ByteStream> bs = ByteStream::create();
  ASSERT_TRUE(bs != 0);

  bs->write8(0x12);
  bs->write16(0x3456);
  bs->write24(0x789abc);
  bs->write32(0xdef01234);

  EXPECT_EQ(10L, bs->size());
  EXPECT_EQ(0, bs->seek(0, SEEK_SET));

  EXPECT_EQ(0x12u, bs->read8());
  EXPECT_EQ(0x3456u, bs->read16());
  EXPECT_EQ(0x789abcu, bs->read24());
  EXPECT_EQ(0xdef01234u, bs->read32());
}

TEST(ByteStreamExtraTest, CopyCopiesAllWhenSizeIsZero)
{
  const char payload[] = "copy-payload";
  GP<ByteStream> src = ByteStream::create(payload, sizeof(payload) - 1);
  GP<ByteStream> dst = ByteStream::create();

  EXPECT_EQ(sizeof(payload) - 1, dst->copy(*src, 0));
  EXPECT_EQ((long)(sizeof(payload) - 1), dst->size());

  EXPECT_EQ(0, dst->seek(0, SEEK_SET));
  char out[32] = {0};
  EXPECT_EQ(sizeof(payload) - 1, dst->readall(out, sizeof(payload) - 1));
  EXPECT_EQ(0, std::memcmp(payload, out, sizeof(payload) - 1));
}

TEST(ByteStreamExtraTest, ReadAtDoesNotMoveCurrentPosition)
{
  const char payload[] = "0123456789";
  GP<ByteStream> bs = ByteStream::create(payload, sizeof(payload) - 1);
  ASSERT_TRUE(bs != 0);

  EXPECT_EQ(0, bs->seek(5, SEEK_SET));
  const long before = bs->tell();

  char out[4] = {0};
  EXPECT_EQ((size_t)3, bs->readat(out, 3, 1));
  EXPECT_EQ('1', out[0]);
  EXPECT_EQ('2', out[1]);
  EXPECT_EQ('3', out[2]);
  EXPECT_EQ(before, bs->tell());
}

TEST(ByteStreamExtraTest, StaticStreamRejectsWrites)
{
  const char payload[] = "static";
  GP<ByteStream> bs = ByteStream::create_static(payload, sizeof(payload) - 1);
  ASSERT_TRUE(bs != 0);

  EXPECT_THROW(bs->write("x", 1), GException);
}

TEST(ByteStreamExtraTest, SeekBeforeStartThrows)
{
  GP<ByteStream> mem = ByteStream::create();
  ASSERT_TRUE(mem != 0);
  EXPECT_THROW(mem->seek(-1, SEEK_SET), GException);

  const char payload[] = "abc";
  GP<ByteStream> st = ByteStream::create_static(payload, sizeof(payload) - 1);
  ASSERT_TRUE(st != 0);
  EXPECT_THROW(st->seek(-1, SEEK_SET), GException);
}

TEST(ByteStreamExtraTest, FormatWriteStringAndMessageHelpersAppendExpectedText)
{
  GP<ByteStream> bs = ByteStream::create();
  ASSERT_TRUE(bs != 0);

  EXPECT_EQ((size_t)7, bs->format("value=%d", 7));
  EXPECT_EQ((size_t)4, bs->writestring(GUTF8String("|utf")));
  EXPECT_EQ((size_t)7, bs->writestring(GNativeString("|native")));
  bs->writemessage("|plain");
  bs->formatmessage("|fmt-%d", 9);

  EXPECT_EQ(0, bs->seek(0, SEEK_SET));
  std::string out;
  char chunk[64];
  while (const size_t got = bs->read(chunk, sizeof(chunk)))
    out.append(chunk, got);

  EXPECT_EQ("value=7|utf|native|plain|fmt-9", out);
}

TEST(ByteStreamExtraTest, IntegerReadsThrowOnUnexpectedEndOfStream)
{
  GP<ByteStream> empty = ByteStream::create();
  ASSERT_TRUE(empty != 0);
  EXPECT_THROW(empty->read8(), GException);

  const unsigned char one[] = {0x12};
  GP<ByteStream> short16 = ByteStream::create(one, sizeof(one));
  ASSERT_TRUE(short16 != 0);
  EXPECT_THROW(short16->read16(), GException);

  const unsigned char two[] = {0x12, 0x34};
  GP<ByteStream> short24 = ByteStream::create(two, sizeof(two));
  ASSERT_TRUE(short24 != 0);
  EXPECT_THROW(short24->read24(), GException);

  const unsigned char three[] = {0x12, 0x34, 0x56};
  GP<ByteStream> short32 = ByteStream::create(three, sizeof(three));
  ASSERT_TRUE(short32 != 0);
  EXPECT_THROW(short32->read32(), GException);
}

TEST(ByteStreamExtraTest, FilePointerAndUrlFactoriesRoundtripData)
{
  const std::filesystem::path dir =
      std::filesystem::temp_directory_path() / "djvu_bytestream_gtest";
  std::filesystem::create_directories(dir);
  const std::filesystem::path path = dir / "payload.bin";

  {
    FILE *f = std::fopen(path.string().c_str(), "wb+");
    ASSERT_NE(nullptr, f);

    GP<ByteStream> out = ByteStream::create(f, "wb+", true);
    ASSERT_TRUE(out != 0);
    const char payload[] = "stdio/url roundtrip";
    EXPECT_EQ(sizeof(payload) - 1, out->writall(payload, sizeof(payload) - 1));
    out->flush();
  }

  const GURL url = GURL::Filename::UTF8(path.string().c_str());
  GP<ByteStream> in = ByteStream::create(url, "rb");
  ASSERT_TRUE(in != 0);

  char out[64] = {0};
  EXPECT_EQ((size_t)19, in->readall(out, 19));
  EXPECT_STREQ("stdio/url roundtrip", out);
}
