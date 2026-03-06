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

TEST(DjVuAnnoTest, AntMergeCopyAndParamSerializationPaths)
{
  GP<DjVuANT> src = DjVuANT::create();
  src->bg_color = 0x00010203UL;
  src->zoom = DjVuANT::ZOOM_PAGE;
  src->mode = DjVuANT::MODE_FORE;
  src->hor_align = DjVuANT::ALIGN_RIGHT;
  src->ver_align = DjVuANT::ALIGN_BOTTOM;
  src->metadata["Author"] = "tester";
  src->xmpmetadata = "<x:xmpmeta>t</x:xmpmeta>";
  EXPECT_FALSE(src->is_empty());

  GP<ByteStream> enc = ByteStream::create();
  src->encode(*enc);
  enc->seek(0, SEEK_SET);

  GP<DjVuANT> merged = DjVuANT::create();
  merged->merge(*enc);
  EXPECT_EQ(DjVuANT::ZOOM_PAGE, merged->zoom);
  EXPECT_EQ(DjVuANT::MODE_FORE, merged->mode);
  EXPECT_EQ(DjVuANT::ALIGN_RIGHT, merged->hor_align);
  EXPECT_EQ(DjVuANT::ALIGN_BOTTOM, merged->ver_align);

  const GUTF8String raw = merged->encode_raw();
  EXPECT_GT(raw.length(), 0u);
  const GUTF8String params = merged->get_paramtags();
  EXPECT_GT(params.length(), 0u);

  GP<ByteStream> out = ByteStream::create();
  merged->writeParam(*out);
  EXPECT_GT(out->size(), 0L);

  GP<DjVuANT> cpy = merged->copy();
  ASSERT_TRUE(cpy != 0);
  EXPECT_EQ(merged->mode, cpy->mode);
}

TEST(DjVuAnnoTest, ContainerMergeAndXmlHelpersAreReachable)
{
  GP<DjVuAnno> left = DjVuAnno::create();
  left->ant = DjVuANT::create();
  left->ant->mode = DjVuANT::MODE_BACK;

  GP<DjVuAnno> right = DjVuAnno::create();
  right->ant = DjVuANT::create();
  right->ant->zoom = 175;
  right->ant->metadata["Title"] = "Doc";

  left->merge(right);
  ASSERT_TRUE(left->ant != 0);
  EXPECT_EQ(175, left->ant->zoom);
  EXPECT_EQ(DjVuANT::MODE_BACK, left->ant->mode);

  const GUTF8String xml_map = left->get_xmlmap("page1.djvu", 100);
  const GUTF8String xml_param = left->get_paramtags();
  EXPECT_GE(xml_map.search("MAP"), 0);
  EXPECT_GT(xml_param.length(), 0u);

  GP<ByteStream> out = ByteStream::create();
  left->writeMap(*out, "page1.djvu", 100);
  left->writeParam(*out);
  EXPECT_GT(out->size(), 0L);
  EXPECT_GT(left->get_memory_usage(), 0u);
}
