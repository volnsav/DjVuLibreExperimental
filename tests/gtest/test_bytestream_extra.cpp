#include <gtest/gtest.h>

#include <cstring>

#include "ByteStream.h"
#include "GException.h"

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
