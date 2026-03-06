#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>

#include "ByteStream.h"
#include "DjVuDocument.h"
#include "DjVuFile.h"
#include "DjVuText.h"
#include "IFFByteStream.h"

namespace {

std::optional<std::filesystem::path> FindReferenceFixtureFile(const char *name)
{
  const std::filesystem::path p1 = std::filesystem::path("tests/fixtures/reference") / name;
  if (std::filesystem::exists(p1))
    return p1;
  const std::filesystem::path p2 = std::filesystem::path("fixtures/reference") / name;
  if (std::filesystem::exists(p2))
    return p2;
  return std::nullopt;
}

bool ContainsNonAscii(const GUTF8String &s)
{
  const char *p = (const char *)s;
  for (; p && *p; ++p)
    if (static_cast<unsigned char>(*p) & 0x80)
      return true;
  return false;
}

GUTF8String DecodeFixtureText(const std::filesystem::path &path)
{
  const GURL url = GURL::Filename::UTF8(path.string().c_str());
  GP<DjVuDocument> doc = DjVuDocument::create_wait(url);
  EXPECT_TRUE(doc != 0);
  if (!doc)
    return GUTF8String();
  EXPECT_TRUE(doc->is_init_complete());

  GP<DjVuFile> file = doc->get_djvu_file(0, false);
  EXPECT_TRUE(file != 0);
  if (!file)
    return GUTF8String();

  EXPECT_NO_THROW(file->resume_decode(true));
  GP<ByteStream> text_bs = file->get_text();
  EXPECT_TRUE(text_bs != 0);
  if (!text_bs)
    return GUTF8String();

  GP<DjVuText> text = DjVuText::create();
  EXPECT_NO_THROW(text->decode(text_bs));
  if (!text || !text->txt)
    return GUTF8String();
  return text->txt->textUTF8;
}

}  // namespace

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

TEST(DjVuTextTest, ReferenceUnicodeFixtureDecodesRealOcrTextAndXml)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("sp_utf8_multilang_hidden.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> doc = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->is_init_complete());

  GP<DjVuFile> file = doc->get_djvu_file(0, false);
  ASSERT_TRUE(file != 0);
  EXPECT_NO_THROW(file->resume_decode(true));
  ASSERT_TRUE(file->contains_text());

  GP<ByteStream> text_bs = file->get_text();
  ASSERT_TRUE(text_bs != 0);

  GP<DjVuText> text = DjVuText::create();
  ASSERT_TRUE(text != 0);
  EXPECT_NO_THROW(text->decode(text_bs));
  ASSERT_TRUE(text->txt != 0);
  EXPECT_GT(text->txt->textUTF8.length(), 0);
  EXPECT_TRUE(ContainsNonAscii(text->txt->textUTF8));

  const GUTF8String xml = text->get_xmlText(800);
  EXPECT_NE(-1, xml.search("HIDDENTEXT"));
  EXPECT_TRUE(ContainsNonAscii(xml));

  GUTF8String selected;
  GList<DjVuTXT::Zone *> zones = text->txt->find_text_in_rect(GRect(0, 0, 2000, 2000), selected);
  EXPECT_GE(zones.size(), 0);
  EXPECT_GE(selected.length(), 0);
}

TEST(DjVuTextTest, ReferenceHiddenOnlyFixtureRoundtripPreservesDecodedText)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("sp_hidden_only_no_visible.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GUTF8String original = DecodeFixtureText(*fixture);
  EXPECT_NE(-1, original.search("HIDDEN_ONLY_TEXT_123"));

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> src = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(src != 0);
  ASSERT_TRUE(src->is_init_complete());

  const std::filesystem::path bundled_path =
      std::filesystem::temp_directory_path() /
      ("djvu_gtest_text_hidden_only_" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_path.string().c_str());
  EXPECT_NO_THROW(src->save_as(bundled_url, true));

  const GUTF8String roundtrip = DecodeFixtureText(bundled_path);
  EXPECT_STREQ((const char *)original, (const char *)roundtrip);

  std::error_code ec;
  std::filesystem::remove(bundled_path, ec);
}

TEST(DjVuTextTest, ReferenceUnicodeFixtureIndirectRoundtripPreservesMultilingualText)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("sp_utf8_multilang_hidden.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GUTF8String original = DecodeFixtureText(*fixture);
  EXPECT_TRUE(ContainsNonAscii(original));

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> src = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(src != 0);
  ASSERT_TRUE(src->is_init_complete());

  const std::filesystem::path indirect_dir =
      std::filesystem::temp_directory_path() /
      ("djvu_gtest_text_unicode_" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directories(indirect_dir);
  const std::filesystem::path indirect_idx = indirect_dir / "index.djvu";
  const GURL indirect_url = GURL::Filename::UTF8(indirect_idx.string().c_str());
  EXPECT_NO_THROW(src->save_as(indirect_url, false));

  const GUTF8String roundtrip = DecodeFixtureText(indirect_idx);
  EXPECT_STREQ((const char *)original, (const char *)roundtrip);
  EXPECT_TRUE(ContainsNonAscii(roundtrip));

  std::error_code ec;
  std::filesystem::remove_all(indirect_dir, ec);
}
