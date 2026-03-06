#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "BSByteStream.h"
#include "ByteStream.h"
#include "GException.h"

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

TEST(BSByteStreamTest, SmallBlockSizeStillRoundtripsLargePayload)
{
  std::vector<char> payload(60000);
  for (size_t i = 0; i < payload.size(); ++i)
    payload[i] = static_cast<char>((i * 13 + (i >> 3)) & 0xff);

  GP<ByteStream> storage = ByteStream::create();
  {
    GP<ByteStream> enc = BSByteStream::create(storage, 1);
    ASSERT_TRUE(enc != 0);
    enc->writall(&payload[0], payload.size());
    enc->flush();
  }

  storage->seek(0, SEEK_SET);
  GP<ByteStream> dec = BSByteStream::create(storage);
  ASSERT_TRUE(dec != 0);

  std::vector<char> decoded(payload.size());
  const size_t got = dec->readall(&decoded[0], decoded.size());
  EXPECT_EQ(payload.size(), got);
  EXPECT_EQ(payload, decoded);
}

TEST(BSByteStreamTest, RandomCorruptedInputThrowsDuringDecode)
{
  GP<ByteStream> storage = ByteStream::create();
  unsigned char garbage[64];
  for (int i = 0; i < 64; ++i)
    garbage[i] = static_cast<unsigned char>((i * 19 + 7) & 0xff);
  ASSERT_EQ(sizeof(garbage), storage->write(garbage, sizeof(garbage)));
  storage->seek(0, SEEK_SET);

  GP<ByteStream> dec = BSByteStream::create(storage);
  char out[128];
  EXPECT_THROW((void)dec->readall(out, sizeof(out)), GException);
}

TEST(BSByteStreamTest, TruncatedCompressedDataThrows)
{
  std::vector<char> payload(20000);
  for (size_t i = 0; i < payload.size(); ++i)
    payload[i] = static_cast<char>((i * 5 + (i >> 1)) & 0xff);

  GP<ByteStream> full = ByteStream::create();
  {
    GP<ByteStream> enc = BSByteStream::create(full, 32);
    enc->writall(&payload[0], payload.size());
    enc->flush();
  }

  ASSERT_EQ(0, full->seek(0, SEEK_END));
  const int len = full->tell();
  ASSERT_GT(len, 8);
  ASSERT_EQ(0, full->seek(0, SEEK_SET));

  std::vector<char> raw(len);
  ASSERT_EQ(static_cast<size_t>(len), full->readall(&raw[0], raw.size()));

  GP<ByteStream> truncated = ByteStream::create();
  const int cut = len / 2;
  ASSERT_GT(cut, 0);
  ASSERT_EQ(cut, truncated->write(&raw[0], cut));
  ASSERT_EQ(0, truncated->seek(0, SEEK_SET));

  GP<ByteStream> dec = BSByteStream::create(truncated);
  std::vector<char> out(payload.size());
  EXPECT_THROW((void)dec->readall(&out[0], out.size()), GException);
}

TEST(BSByteStreamTest, FlushBetweenWritesKeepsFullDecodeOrder)
{
  const char part1[] = "alpha-beta-";
  const char part2[] = "gamma-delta-epsilon";

  GP<ByteStream> storage = ByteStream::create();
  {
    GP<ByteStream> enc = BSByteStream::create(storage, 10);
    ASSERT_TRUE(enc != 0);
    enc->writall(part1, sizeof(part1) - 1);
    enc->flush();
    enc->writall(part2, sizeof(part2) - 1);
    enc->flush();
  }

  storage->seek(0, SEEK_SET);
  GP<ByteStream> dec = BSByteStream::create(storage);
  ASSERT_TRUE(dec != 0);

  const size_t expected_len = (sizeof(part1) - 1) + (sizeof(part2) - 1);
  std::vector<char> out(expected_len + 1, '\0');
  ASSERT_EQ(expected_len, dec->readall(&out[0], expected_len));

  const std::string expected = std::string(part1) + std::string(part2);
  EXPECT_EQ(expected, std::string(&out[0], expected_len));
}

TEST(BSByteStreamTest, IncrementalReadMatchesOriginalAndTellAdvances)
{
  std::vector<char> payload(12345);
  for (size_t i = 0; i < payload.size(); ++i)
    payload[i] = static_cast<char>((i * 9 + (i >> 4)) & 0xff);

  GP<ByteStream> storage = ByteStream::create();
  {
    GP<ByteStream> enc = BSByteStream::create(storage, 32);
    enc->writall(&payload[0], payload.size());
    enc->flush();
  }

  storage->seek(0, SEEK_SET);
  GP<ByteStream> dec = BSByteStream::create(storage);
  ASSERT_TRUE(dec != 0);
  EXPECT_EQ(0, dec->tell());

  std::vector<char> out(payload.size(), 0);
  size_t pos = 0;
  while (pos < out.size())
  {
    const size_t chunk = std::min<size_t>(37, out.size() - pos);
    const size_t got = dec->read(&out[pos], chunk);
    ASSERT_EQ(chunk, got);
    pos += got;
    EXPECT_EQ(static_cast<long>(pos), dec->tell());
  }

  EXPECT_EQ(payload, out);
  char extra = 0;
  EXPECT_EQ((size_t)0, dec->read(&extra, 1));
  EXPECT_EQ(static_cast<long>(out.size()), dec->tell());
}

TEST(BSByteStreamTest, ReadWithNullBufferSkipsBytesAndAdvancesTell)
{
  std::vector<char> payload(5000);
  for (size_t i = 0; i < payload.size(); ++i)
    payload[i] = static_cast<char>((i * 7 + (i >> 3)) & 0xff);

  GP<ByteStream> storage = ByteStream::create();
  {
    GP<ByteStream> enc = BSByteStream::create(storage, 32);
    enc->writall(&payload[0], payload.size());
    enc->flush();
  }

  storage->seek(0, SEEK_SET);
  GP<ByteStream> dec = BSByteStream::create(storage);
  ASSERT_TRUE(dec != 0);

  const size_t skip = 777;
  EXPECT_EQ(skip, dec->read(0, skip));
  EXPECT_EQ(static_cast<long>(skip), dec->tell());

  std::vector<char> tail(payload.size() - skip, 0);
  ASSERT_EQ(tail.size(), dec->readall(&tail[0], tail.size()));
  EXPECT_TRUE(std::equal(tail.begin(), tail.end(), payload.begin() + skip));
}

TEST(BSByteStreamTest, MultipleFlushCallsKeepOutputDecodable)
{
  const char part1[] = "block-one-";
  const char part2[] = "block-two";

  GP<ByteStream> storage = ByteStream::create();
  {
    GP<ByteStream> enc = BSByteStream::create(storage, 4);
    enc->writall(part1, sizeof(part1) - 1);
    enc->flush();
    enc->flush();
    enc->writall(part2, sizeof(part2) - 1);
    enc->flush();
    enc->flush();
  }

  storage->seek(0, SEEK_SET);
  GP<ByteStream> dec = BSByteStream::create(storage);
  ASSERT_TRUE(dec != 0);

  const std::string expected = std::string(part1) + std::string(part2);
  std::vector<char> out(expected.size() + 1, '\0');
  ASSERT_EQ(expected.size(), dec->readall(&out[0], expected.size()));
  EXPECT_EQ(expected, std::string(&out[0], expected.size()));
}

TEST(BSByteStreamTest, ReadZeroBytesReturnsZeroWithoutChangingTell)
{
  const char payload[] = "tiny-payload";
  GP<ByteStream> storage = ByteStream::create();
  {
    GP<ByteStream> enc = BSByteStream::create(storage, 8);
    enc->writall(payload, sizeof(payload) - 1);
    enc->flush();
  }

  storage->seek(0, SEEK_SET);
  GP<ByteStream> dec = BSByteStream::create(storage);
  ASSERT_TRUE(dec != 0);
  EXPECT_EQ(0, dec->tell());

  char dummy = 0;
  EXPECT_EQ((size_t)0, dec->read(&dummy, 0));
  EXPECT_EQ(0, dec->tell());
}
