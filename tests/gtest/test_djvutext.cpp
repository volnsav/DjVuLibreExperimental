#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DjVuText.h"

TEST(DjVuTextTest, DefaultXmlWithoutTextIsHiddenTextTag)
{
  GP<DjVuText> text = DjVuText::create();
  ASSERT_TRUE(text != 0);

  const GUTF8String xml = text->get_xmlText(100);
  EXPECT_GE(xml.search("HIDDENTEXT"), 0);
  EXPECT_EQ(0u, text->get_memory_usage());
}

TEST(DjVuTextTest, EncodeDecodeRoundtripKeepsUtf8Text)
{
  GP<DjVuText> src = DjVuText::create();
  src->txt = DjVuTXT::create();
  src->txt->textUTF8 = "hello world";

  GP<ByteStream> bs = ByteStream::create();
  src->encode(bs);
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  GP<DjVuText> dst = DjVuText::create();
  dst->decode(bs);
  ASSERT_TRUE(dst->txt != 0);
  EXPECT_STREQ("hello world", (const char *)dst->txt->textUTF8);
}

TEST(DjVuTextTest, ZoneAppendChildSetsParentAndType)
{
  DjVuTXT::Zone zone;
  zone.ztype = DjVuTXT::PARAGRAPH;
  DjVuTXT::Zone *child = zone.append_child();
  ASSERT_NE(nullptr, child);
  EXPECT_EQ(DjVuTXT::PARAGRAPH, child->ztype);
  EXPECT_EQ(&zone, child->get_parent());
}
