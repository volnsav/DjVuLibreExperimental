#include <gtest/gtest.h>

#include <vector>

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

TEST(MMRDecoderTest, DecodeHeaderConsumesExactlyEightBytes)
{
  GP<ByteStream> bs = ByteStream::create();
  const unsigned char input[] = {
      'M', 'M', 'R', 0x00, 0x00, 0x10, 0x00, 0x20, 0xaa, 0xbb, 0xcc};
  ASSERT_EQ(sizeof(input), bs->write(input, sizeof(input)));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  int width = 0;
  int height = 0;
  int invert = 0;
  EXPECT_NO_THROW((void)MMRDecoder::decode_header(*bs, width, height, invert));
  EXPECT_EQ(8, bs->tell());
}

TEST(MMRDecoderTest, DecodeRejectsStripedHeaderWithoutStripePayload)
{
  GP<ByteStream> bs = ByteStream::create();
  // Striped bit set (0x02), but no rows-per-stripe and no strip payload.
  const unsigned char header[] = {'M', 'M', 'R', 0x02, 0x00, 0x10, 0x00, 0x10};
  ASSERT_EQ(sizeof(header), bs->write(header, sizeof(header)));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  EXPECT_THROW((void)MMRDecoder::decode(bs), GException);
}

TEST(MMRDecoderTest, DecodeHeaderRejectsTruncatedInput)
{
  GP<ByteStream> bs = ByteStream::create();
  const unsigned char short_header[] = {'M', 'M', 'R', 0x00, 0x00, 0x10, 0x00};
  ASSERT_EQ(sizeof(short_header), bs->write(short_header, sizeof(short_header)));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  int width = 0;
  int height = 0;
  int invert = 0;
  EXPECT_THROW((void)MMRDecoder::decode_header(*bs, width, height, invert), GException);
}

TEST(MMRDecoderTest, DecodeRejectsStripedHeaderWithIncompleteStripeMeta)
{
  GP<ByteStream> bs = ByteStream::create();
  // Striped image with rows-per-stripe present, but incomplete stripe length metadata.
  const unsigned char payload[] = {
      'M', 'M', 'R', 0x02, 0x00, 0x10, 0x00, 0x10,
      0x00, 0x04, // rows-per-stripe
      0x00, 0x00  // truncated bytes of first 32-bit stripe size
  };
  ASSERT_EQ(sizeof(payload), bs->write(payload, sizeof(payload)));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  EXPECT_THROW((void)MMRDecoder::decode(bs), GException);
}

TEST(MMRDecoderTest, DecodeHeaderRejectsUnsupportedFlagBits)
{
  GP<ByteStream> bs = ByteStream::create();
  // High bits in the 4th byte are not part of the accepted MMR header magic.
  const unsigned char header[] = {'M', 'M', 'R', 0x04, 0x00, 0x10, 0x00, 0x10};
  ASSERT_EQ(sizeof(header), bs->write(header, sizeof(header)));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  int width = 0;
  int height = 0;
  int invert = 0;
  EXPECT_THROW((void)MMRDecoder::decode_header(*bs, width, height, invert), GException);
}

TEST(MMRDecoderTest, DecodeRejectsStripedHeaderWithZeroRowsPerStripe)
{
  GP<ByteStream> bs = ByteStream::create();
  const unsigned char payload[] = {
      'M', 'M', 'R', 0x02, 0x00, 0x10, 0x00, 0x10,
      0x00, 0x00 // rows-per-stripe = 0 (invalid for striped stream)
  };
  ASSERT_EQ(sizeof(payload), bs->write(payload, sizeof(payload)));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  EXPECT_THROW((void)MMRDecoder::decode(bs), GException);
}

TEST(MMRDecoderTest, DecodeHeaderParsesInvertOnlyFlag)
{
  GP<ByteStream> bs = ByteStream::create();
  const unsigned char header[] = {'M', 'M', 'R', 0x01, 0x00, 0x21, 0x00, 0x11};
  ASSERT_EQ(sizeof(header), bs->write(header, sizeof(header)));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  int width = 0;
  int height = 0;
  int invert = 0;
  const bool striped = MMRDecoder::decode_header(*bs, width, height, invert);

  EXPECT_FALSE(striped);
  EXPECT_EQ(33, width);
  EXPECT_EQ(17, height);
  EXPECT_EQ(1, invert);
}

TEST(MMRDecoderTest, DecodeRejectsEmptyInputStream)
{
  GP<ByteStream> bs = ByteStream::create();
  EXPECT_THROW((void)MMRDecoder::decode(bs), GException);
}

TEST(MMRDecoderTest, DecodeRejectsStripedHeaderWithZeroFirstStripeLength)
{
  GP<ByteStream> bs = ByteStream::create();
  const unsigned char payload[] = {
      'M', 'M', 'R', 0x02, 0x00, 0x10, 0x00, 0x10,
      0x00, 0x04, // rows-per-stripe
      0x00, 0x00, 0x00, 0x00 // first stripe byte length = 0
  };
  ASSERT_EQ(sizeof(payload), bs->write(payload, sizeof(payload)));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  EXPECT_THROW((void)MMRDecoder::decode(bs), GException);
}

TEST(MMRDecoderTest, DecodeStripedPayloadWithMultipleStripesExercisesStripeSource)
{
  GP<ByteStream> bs = ByteStream::create();
  // MMR, striped, width=16, height=8, rows-per-stripe=4, two tiny stripes.
  const unsigned char payload[] = {
      'M', 'M', 'R', 0x02, 0x00, 0x10, 0x00, 0x08, 0x00, 0x04,
      0x00, 0x00, 0x00, 0x04, 0x00, 0x11, 0x22, 0x33,
      0x00, 0x00, 0x00, 0x04, 0x44, 0x55, 0x66, 0x77};
  ASSERT_EQ(sizeof(payload), bs->write(payload, sizeof(payload)));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  try
  {
    (void)MMRDecoder::decode(bs);
  }
  catch (...)
  {
    SUCCEED();
  }
}

TEST(MMRDecoderTest, DeterministicFuzzDecodeCoversMoreCodePaths)
{
  for (unsigned int seed = 1; seed <= 2048; ++seed)
  {
    GP<ByteStream> bs = ByteStream::create();

    unsigned int x = seed * 1103515245u + 12345u;
    auto next_u8 = [&x]() -> unsigned char {
      x = x * 1664525u + 1013904223u;
      return static_cast<unsigned char>((x >> 24) & 0xff);
    };

    const bool striped = (seed % 2) == 0;
    const unsigned char flags = static_cast<unsigned char>((striped ? 0x02 : 0x00) | (seed & 0x01));
    const unsigned short w = static_cast<unsigned short>(8 + (seed % 24));
    const unsigned short h = static_cast<unsigned short>(8 + ((seed * 3) % 24));

    const unsigned char header[] = {'M', 'M', 'R', flags,
                                    static_cast<unsigned char>((w >> 8) & 0xff),
                                    static_cast<unsigned char>(w & 0xff),
                                    static_cast<unsigned char>((h >> 8) & 0xff),
                                    static_cast<unsigned char>(h & 0xff)};
    ASSERT_EQ(sizeof(header), bs->write(header, sizeof(header)));

    if (striped)
    {
      const unsigned short rows = static_cast<unsigned short>(1 + (seed % 6));
      const unsigned char rows_be[] = {static_cast<unsigned char>((rows >> 8) & 0xff),
                                       static_cast<unsigned char>(rows & 0xff)};
      ASSERT_EQ(sizeof(rows_be), bs->write(rows_be, sizeof(rows_be)));
      for (int s = 0; s < 3; ++s)
      {
        const unsigned int stripe_size = 1 + (next_u8() % 24);
        const unsigned char len_be[] = {static_cast<unsigned char>((stripe_size >> 24) & 0xff),
                                        static_cast<unsigned char>((stripe_size >> 16) & 0xff),
                                        static_cast<unsigned char>((stripe_size >> 8) & 0xff),
                                        static_cast<unsigned char>(stripe_size & 0xff)};
        ASSERT_EQ(sizeof(len_be), bs->write(len_be, sizeof(len_be)));
        for (unsigned int i = 0; i < stripe_size; ++i)
        {
          const unsigned char b = next_u8();
          ASSERT_EQ(1u, bs->write(&b, 1));
        }
      }
    }
    else
    {
      for (int i = 0; i < 32; ++i)
      {
        const unsigned char b = next_u8();
        ASSERT_EQ(1u, bs->write(&b, 1));
      }
    }

    ASSERT_EQ(0, bs->seek(0, SEEK_SET));
    try
    {
      (void)MMRDecoder::decode(bs);
    }
    catch (...)
    {
      // Coverage-oriented: invalid streams are expected for many seeds.
    }
  }
}

TEST(MMRDecoderTest, PublicCreateAndScanrlePathForRegularStream)
{
  GP<ByteStream> bs = ByteStream::create();
  for (int i = 0; i < 64; ++i)
  {
    const unsigned char b = static_cast<unsigned char>((i * 37 + 11) & 0xff);
    ASSERT_EQ(1u, bs->write(&b, 1));
  }
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  GP<MMRDecoder> dec = MMRDecoder::create(bs, 16, 8, false);
  ASSERT_TRUE(dec != 0);

  for (int i = 0; i < 8; ++i)
  {
    try
    {
      const unsigned char *endptr = 0;
      (void)dec->scanrle((i % 2) == 0, &endptr);
      if (!endptr)
        break;
    }
    catch (...)
    {
      break;
    }
  }
}

TEST(MMRDecoderTest, PublicCreateAndScanrunsPathForStripedStream)
{
  GP<ByteStream> bs = ByteStream::create();
  // rows-per-stripe = 4; then two stripe payloads with lengths.
  const unsigned char payload[] = {
      0x00, 0x04,
      0x00, 0x00, 0x00, 0x06, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
      0x00, 0x00, 0x00, 0x06, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
  ASSERT_EQ(sizeof(payload), bs->write(payload, sizeof(payload)));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  GP<MMRDecoder> dec = MMRDecoder::create(bs, 16, 8, true);
  ASSERT_TRUE(dec != 0);
  for (int i = 0; i < 8; ++i)
  {
    try
    {
      const unsigned short *endptr = 0;
      (void)dec->scanruns(&endptr);
      if (!endptr)
        break;
    }
    catch (...)
    {
      break;
    }
  }
}
