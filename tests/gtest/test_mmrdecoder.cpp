#include <gtest/gtest.h>

#include "ByteStream.h"
#include "MMRDecoder.h"

TEST(MMRDecoderTest, DecodeHeaderParsesRegularFlagsAndSize)
{
  GP<ByteStream> bs = ByteStream::create();
  const unsigned char header[] = {'M', 'M', 'R', 0x00, 0x01, 0x2c, 0x00, 0xc8};
  ASSERT_EQ(sizeof(header), bs->write(header, sizeof(header)));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  int width = 0;
  int height = 0;
  int invert = 0;
  const bool striped = MMRDecoder::decode_header(*bs, width, height, invert);

  EXPECT_FALSE(striped);
  EXPECT_EQ(300, width);
  EXPECT_EQ(200, height);
  EXPECT_EQ(0, invert);
}

TEST(MMRDecoderTest, DecodeHeaderParsesStripedAndInvertedFlags)
{
  GP<ByteStream> bs = ByteStream::create();
  const unsigned char header[] = {'M', 'M', 'R', 0x03, 0x00, 0x10, 0x00, 0x20};
  ASSERT_EQ(sizeof(header), bs->write(header, sizeof(header)));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  int width = 0;
  int height = 0;
  int invert = 0;
  const bool striped = MMRDecoder::decode_header(*bs, width, height, invert);

  EXPECT_TRUE(striped);
  EXPECT_EQ(16, width);
  EXPECT_EQ(32, height);
  EXPECT_EQ(1, invert);
}

TEST(MMRDecoderTest, DecodeHeaderRejectsInvalidMagic)
{
  GP<ByteStream> bs = ByteStream::create();
  const unsigned char header[] = {'B', 'A', 'D', '!', 0x00, 0x01, 0x00, 0x01};
  ASSERT_EQ(sizeof(header), bs->write(header, sizeof(header)));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  int width = 0;
  int height = 0;
  int invert = 0;
  EXPECT_THROW(MMRDecoder::decode_header(*bs, width, height, invert), GException);
}
