#include <gtest/gtest.h>

#include <cstring>
#include <vector>

#include "BSByteStream.h"
#include "ByteStream.h"
#include "GException.h"

TEST(BSEncodeByteStreamTest, EncoderRejectsBlockSizeAboveMax)
{
  GP<ByteStream> storage = ByteStream::create();
  EXPECT_THROW((void)BSByteStream::create(storage, BSByteStream::MAXBLOCK + 1),
               GException);
}

TEST(BSEncodeByteStreamTest, ExactFlushBoundaryRoundtrips)
{
  const size_t block_payload = static_cast<size_t>(BSByteStream::MINBLOCK) * 1024 - 1;
  std::vector<char> payload(block_payload * 2 + 17);
  for (size_t i = 0; i < payload.size(); ++i)
    payload[i] = static_cast<char>((i * 17 + (i >> 2)) & 0xff);

  GP<ByteStream> storage = ByteStream::create();
  {
    GP<ByteStream> enc = BSByteStream::create(storage, BSByteStream::MINBLOCK);
    ASSERT_TRUE(enc != 0);
    ASSERT_EQ(payload.size(), enc->write(&payload[0], payload.size()));
    enc->flush();
  }

  ASSERT_EQ(0, storage->seek(0, SEEK_SET));
  GP<ByteStream> dec = BSByteStream::create(storage);
  ASSERT_TRUE(dec != 0);

  std::vector<char> out(payload.size());
  ASSERT_EQ(payload.size(), dec->readall(&out[0], out.size()));
  EXPECT_EQ(payload, out);
}

TEST(BSEncodeByteStreamTest, ZeroLengthWriteReturnsZeroAndDoesNotCorruptStream)
{
  GP<ByteStream> storage = ByteStream::create();
  GP<ByteStream> enc = BSByteStream::create(storage, BSByteStream::MINBLOCK);
  ASSERT_TRUE(enc != 0);

  const char marker[] = "abc";
  EXPECT_EQ(0u, enc->write(marker, 0));
  ASSERT_EQ(sizeof(marker), enc->write(marker, sizeof(marker)));
  enc->flush();
  enc = 0;

  ASSERT_EQ(0, storage->seek(0, SEEK_SET));
  GP<ByteStream> dec = BSByteStream::create(storage);
  ASSERT_TRUE(dec != 0);
  char out[sizeof(marker)] = {0};
  ASSERT_EQ(sizeof(marker), dec->readall(out, sizeof(out)));
  EXPECT_EQ(0, memcmp(marker, out, sizeof(marker)));
}
