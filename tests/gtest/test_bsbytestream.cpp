#include <gtest/gtest.h>

#include "BSByteStream.h"
#include "ByteStream.h"

TEST(BSByteStreamTest, CompressAndDecompressRoundtrip)
{
  GP<ByteStream> storage = ByteStream::create();

  const char payload[] =
      "DjVuLibre BSByteStream roundtrip payload with repeated repeated repeated data.";

  {
    GP<ByteStream> enc = BSByteStream::create(storage, 32);
    ASSERT_TRUE(enc != 0);
    enc->writall(payload, sizeof(payload) - 1);
    enc->flush();
  }

  storage->seek(0, SEEK_SET);

  GP<ByteStream> dec = BSByteStream::create(storage);
  ASSERT_TRUE(dec != 0);

  char out[256] = {0};
  const size_t got = dec->readall(out, sizeof(payload) - 1);
  EXPECT_EQ(sizeof(payload) - 1, got);
  EXPECT_STREQ(payload, out);
}

TEST(BSByteStreamTest, EmptyInputDecodesToZeroBytes)
{
  GP<ByteStream> storage = ByteStream::create();

  {
    GP<ByteStream> enc = BSByteStream::create(storage, 32);
    enc->flush();
  }

  storage->seek(0, SEEK_SET);
  GP<ByteStream> dec = BSByteStream::create(storage);

  char out[8] = {0};
  EXPECT_EQ((size_t)0, dec->read(out, sizeof(out)));
}
