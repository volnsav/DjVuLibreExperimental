#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DjVuAnno.h"

TEST(DjVuAnnoTest, ColorConversionParsesRgbHex)
{
  const unsigned long rgb = DjVuANT::cvt_color("#112233", 0);
  EXPECT_EQ(0x00112233UL, rgb);
  EXPECT_EQ(0x00abcdefUL, DjVuANT::cvt_color("bad", 0x00abcdefUL));
}

TEST(DjVuAnnoTest, AntEncodeDecodeRoundtripBasicFields)
{
  GP<DjVuANT> src = DjVuANT::create();
  src->bg_color = 0x00112233UL;
  src->zoom = 125;
  src->mode = DjVuANT::MODE_COLOR;
  src->hor_align = DjVuANT::ALIGN_LEFT;
  src->ver_align = DjVuANT::ALIGN_TOP;

  GP<ByteStream> bs = ByteStream::create();
  src->encode(*bs);
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  GP<DjVuANT> dst = DjVuANT::create();
  dst->decode(*bs);
  EXPECT_EQ(0x00112233UL, dst->bg_color);
  EXPECT_EQ(125, dst->zoom);
  EXPECT_EQ(DjVuANT::MODE_COLOR, dst->mode);
}

TEST(DjVuAnnoTest, ContainerEncodeDecodeKeepsAntData)
{
  GP<DjVuAnno> src = DjVuAnno::create();
  src->ant = DjVuANT::create();
  src->ant->zoom = DjVuANT::ZOOM_WIDTH;
  src->ant->mode = DjVuANT::MODE_BW;

  GP<ByteStream> bs = ByteStream::create();
  src->encode(bs);
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  GP<DjVuAnno> dst = DjVuAnno::create();
  dst->decode(bs);
  ASSERT_TRUE(dst->ant != 0);
  EXPECT_EQ(DjVuANT::ZOOM_WIDTH, dst->ant->zoom);
  EXPECT_EQ(DjVuANT::MODE_BW, dst->ant->mode);
}
