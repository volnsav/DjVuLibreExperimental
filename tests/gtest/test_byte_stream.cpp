#include <gtest/gtest.h>

#include "ByteStream.h"

TEST(ByteStreamTest, MemoryRoundtripAndIntegers)
{
  GP<ByteStream> bs = ByteStream::create();
  ASSERT_TRUE(bs != 0);

  const char payload[] = "djvu";
  EXPECT_EQ(sizeof(payload) - 1, bs->write(payload, sizeof(payload) - 1));
  EXPECT_EQ(4L, bs->tell());
  EXPECT_EQ(4L, bs->size());

  EXPECT_EQ(0, bs->seek(0, SEEK_SET));
  char out[8] = {0};
  EXPECT_EQ((size_t)4, bs->readall(out, 4));
  EXPECT_EQ('d', out[0]);
  EXPECT_EQ('j', out[1]);
  EXPECT_EQ('v', out[2]);
  EXPECT_EQ('u', out[3]);

  EXPECT_EQ(0, bs->seek(0, SEEK_END));
  bs->write32(0x01020304U);
  EXPECT_EQ(8L, bs->size());

  EXPECT_EQ(0, bs->seek(4, SEEK_SET));
  EXPECT_EQ(0x01020304U, bs->read32());
}
