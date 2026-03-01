#include <gtest/gtest.h>

#include "ByteStream.h"
#include "GString.h"
#include "GURL.h"

TEST(GStringTest, BasicOperations)
{
  GUTF8String text("abc");
  EXPECT_EQ(3u, text.length());
  EXPECT_EQ(1, text.contains("b"));
  EXPECT_EQ(1, text.rcontains("b"));
  EXPECT_TRUE(text == "abc");
}

TEST(GUrlTest, LocalAndRemoteProtocols)
{
  GURL::UTF8 local_url("file:///tmp/sample.djvu");
  EXPECT_TRUE(local_url.is_local_file_url());
  EXPECT_TRUE(local_url.protocol() == "file");

  GURL::UTF8 remote_url("https://example.com/file%201.djvu");
  EXPECT_FALSE(remote_url.is_local_file_url());
  EXPECT_TRUE(remote_url.protocol() == "https");
  EXPECT_TRUE(remote_url.name() == "file%201.djvu");
  EXPECT_TRUE(remote_url.fname() == "file 1.djvu");
  EXPECT_TRUE(remote_url.extension() == "djvu");
}

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

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
