#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DjVuInfo.h"
#include "GException.h"

TEST(DjVuInfoTest, EncodeDecodeRoundtrip)
{
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 640;
  info->height = 480;
  info->version = 24;
  info->dpi = 300;
  info->gamma = 2.2;
  info->orientation = 3;

  GP<ByteStream> bs = ByteStream::create();
  info->encode(*bs);

  bs->seek(0, SEEK_SET);
  GP<DjVuInfo> decoded = DjVuInfo::create();
  decoded->decode(*bs);

  EXPECT_EQ(640, decoded->width);
  EXPECT_EQ(480, decoded->height);
  EXPECT_EQ(24, decoded->version);
  EXPECT_EQ(300, decoded->dpi);
  EXPECT_DOUBLE_EQ(2.2, decoded->gamma);
  EXPECT_EQ(3, decoded->orientation);
}

TEST(DjVuInfoTest, DecodeClampsInvalidGammaAndDpi)
{
  GP<ByteStream> bs = ByteStream::create();
  bs->write8(0x00); bs->write8(0x10); // width 16
  bs->write8(0x00); bs->write8(0x20); // height 32
  bs->write8(0x18);                   // version low
  bs->write8(0x00);                   // version high
  bs->write8(10);   bs->write8(0);    // dpi 10 -> invalid -> clamp to 300
  bs->write8(1);                      // gamma 0.1 -> clamp to 0.3
  bs->write8(6);                      // orientation flag => 1

  bs->seek(0, SEEK_SET);
  GP<DjVuInfo> decoded = DjVuInfo::create();
  decoded->decode(*bs);

  EXPECT_EQ(16, decoded->width);
  EXPECT_EQ(32, decoded->height);
  EXPECT_EQ(24, decoded->version);
  EXPECT_EQ(300, decoded->dpi);
  EXPECT_DOUBLE_EQ(0.3, decoded->gamma);
  EXPECT_EQ(1, decoded->orientation);
}

TEST(DjVuInfoTest, DecodeRejectsTooShortPayload)
{
  GP<ByteStream> bs = ByteStream::create();
  bs->write8(0x00);
  bs->write8(0x10);
  bs->write8(0x00);
  bs->write8(0x20);
  bs->seek(0, SEEK_SET);

  GP<DjVuInfo> decoded = DjVuInfo::create();
  EXPECT_THROW(decoded->decode(*bs), GException);
}

TEST(DjVuInfoTest, DecodeFiveBytePayloadKeepsDefaultDpiGammaAndOrientation)
{
  GP<ByteStream> bs = ByteStream::create();
  bs->write8(0x00); bs->write8(0x64); // width 100
  bs->write8(0x00); bs->write8(0x32); // height 50
  bs->write8(0x17);                   // version low only (legacy payload)
  bs->seek(0, SEEK_SET);

  GP<DjVuInfo> decoded = DjVuInfo::create();
  decoded->decode(*bs);

  EXPECT_EQ(100, decoded->width);
  EXPECT_EQ(50, decoded->height);
  EXPECT_EQ(23, decoded->version);
  EXPECT_EQ(300, decoded->dpi);
  EXPECT_DOUBLE_EQ(2.2, decoded->gamma);
  EXPECT_EQ(0, decoded->orientation);
}

TEST(DjVuInfoTest, DecodeClampsTooLargeGamma)
{
  GP<ByteStream> bs = ByteStream::create();
  bs->write8(0x00); bs->write8(0x20); // width
  bs->write8(0x00); bs->write8(0x10); // height
  bs->write8(0x18);                   // version low
  bs->write8(0x00);                   // version high
  bs->write8(0x2c); bs->write8(0x01); // dpi 300
  bs->write8(60);                     // gamma 6.0 -> clamp to 5.0
  bs->write8(0);                      // orientation flags
  bs->seek(0, SEEK_SET);

  GP<DjVuInfo> decoded = DjVuInfo::create();
  decoded->decode(*bs);
  EXPECT_DOUBLE_EQ(5.0, decoded->gamma);
}
