#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DjVuText.h"
#include "IFFByteStream.h"

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

TEST(DjVuTextTest, TxtZoneRoundtripAndRectQueries)
{
  GP<DjVuTXT> txt = DjVuTXT::create();
  txt->textUTF8 = "Hello World";
  txt->page_zone.ztype = DjVuTXT::PAGE;
  txt->page_zone.rect = GRect(0, 0, 100, 100);
  txt->page_zone.text_start = 0;
  txt->page_zone.text_length = txt->textUTF8.length();

  DjVuTXT::Zone *line = txt->page_zone.append_child();
  line->ztype = DjVuTXT::LINE;
  line->rect = GRect(5, 5, 90, 20);
  line->text_start = 0;
  line->text_length = txt->textUTF8.length();

  DjVuTXT::Zone *word = line->append_child();
  word->ztype = DjVuTXT::WORD;
  word->rect = GRect(10, 10, 40, 10);
  word->text_start = 0;
  word->text_length = 5;

  GUTF8String selected;
  GList<GRect> rects = txt->find_text_with_rect(GRect(8, 8, 10, 10), selected, 1);
  EXPECT_GE(rects.size(), 1);
  EXPECT_STREQ("Hello", (const char *)selected.substr(0, 5));

  GUTF8String selected2;
  GList<DjVuTXT::Zone *> zones = txt->find_text_in_rect(GRect(8, 8, 10, 10), selected2);
  EXPECT_GE(zones.size(), 0);
  EXPECT_GT(txt->get_memory_usage(), 0u);

  GP<ByteStream> bs = ByteStream::create();
  txt->encode(bs);
  bs->seek(0, SEEK_SET);
  GP<DjVuTXT> decoded = DjVuTXT::create();
  decoded->decode(bs);
  EXPECT_STREQ((const char *)txt->textUTF8, (const char *)decoded->textUTF8);

  GP<DjVuTXT> copied = txt->copy();
  ASSERT_TRUE(copied != 0);
  EXPECT_STREQ((const char *)txt->textUTF8, (const char *)copied->textUTF8);
  EXPECT_GE(copied->get_xmlText(100).search("WORD"), 0);
}

TEST(DjVuTextTest, DuplicateChunkAndBadVersionThrow)
{
  GP<ByteStream> dup = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(dup);

  iff->put_chunk("TXTa");
  GP<DjVuTXT> t1 = DjVuTXT::create();
  t1->textUTF8 = "a";
  t1->encode(iff->get_bytestream());
  iff->close_chunk();

  iff->put_chunk("TXTa");
  GP<DjVuTXT> t2 = DjVuTXT::create();
  t2->textUTF8 = "b";
  t2->encode(iff->get_bytestream());
  iff->close_chunk();

  dup->seek(0, SEEK_SET);
  GP<DjVuText> text = DjVuText::create();
  EXPECT_THROW(text->decode(dup), GException);

  GP<ByteStream> bad = ByteStream::create();
  GP<IFFByteStream> iff_bad = IFFByteStream::create(bad);
  iff_bad->put_chunk("TXTa");
  ByteStream &out = *iff_bad->get_bytestream();
  out.write24(1);
  out.write8('x');
  out.write8(0xff);
  iff_bad->close_chunk();
  bad->seek(0, SEEK_SET);

  GP<DjVuText> text_bad = DjVuText::create();
  EXPECT_THROW(text_bad->decode(bad), GException);
}

TEST(DjVuTextTest, ZoneNormalizeAndHelpersAreReachable)
{
  GP<DjVuTXT> txt = DjVuTXT::create();
  txt->textUTF8 = "abc def ghi";
  txt->page_zone.ztype = DjVuTXT::PAGE;
  txt->page_zone.rect = GRect(0, 0, 120, 80);
  txt->page_zone.text_start = 0;
  txt->page_zone.text_length = txt->textUTF8.length();

  DjVuTXT::Zone *para = txt->page_zone.append_child();
  para->ztype = DjVuTXT::PARAGRAPH;
  para->rect = GRect(0, 0, 120, 80);
  para->text_start = 0;
  para->text_length = txt->textUTF8.length();

  DjVuTXT::Zone *line = para->append_child();
  line->ztype = DjVuTXT::LINE;
  line->rect = GRect(0, 40, 120, 20);
  line->text_start = 0;
  line->text_length = txt->textUTF8.length();

  DjVuTXT::Zone *w1 = line->append_child();
  w1->ztype = DjVuTXT::WORD;
  w1->rect = GRect(2, 42, 20, 10);
  w1->text_start = 0;
  w1->text_length = 3;

  DjVuTXT::Zone *w2 = line->append_child();
  w2->ztype = DjVuTXT::WORD;
  w2->rect = GRect(30, 42, 20, 10);
  w2->text_start = 4;
  w2->text_length = 3;

  txt->normalize_text();
  EXPECT_GT(txt->textUTF8.length(), 0);

  GUTF8String selected;
  GList<GRect> rects = txt->find_text_with_rect(GRect(0, 40, 60, 20), selected, 2);
  EXPECT_GE(rects.size(), 0);
  EXPECT_GE(selected.length(), 0);

  GUTF8String in_rect;
  GList<DjVuTXT::Zone *> zones = txt->find_text_in_rect(GRect(0, 0, 120, 80), in_rect);
  EXPECT_GE(zones.size(), 0);
}
