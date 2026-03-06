#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>

#include "ByteStream.h"
#include "DjVuDocument.h"
#include "DjVuFile.h"
#include "DjVuInfo.h"
#include "DjVuImage.h"
#include "DjVuText.h"
#include "IFFByteStream.h"
#include "GURL.h"
#include "XMLParser.h"
#include "XMLTags.h"

namespace {

std::filesystem::path MakeTempXmlParserPath(const char *suffix)
{
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("djvu_gtest_xmlparser_" + std::to_string(now) + "_" + suffix + ".djvu");
}

GURL WriteSinglePageDjvu(const std::filesystem::path &path)
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVU");

  iff->put_chunk("INFO");
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 80;
  info->height = 60;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  info->encode(*iff->get_bytestream());
  iff->close_chunk();

  iff->close_chunk();
  bs->seek(0, SEEK_SET);

  const GURL url = GURL::Filename::UTF8(path.string().c_str());
  GP<ByteStream> out = ByteStream::create(url, "wb");
  out->copy(*bs, 0);
  out->flush();
  return url;
}

GP<ByteStream> MakeXmlStream(const std::string &xml)
{
  GP<ByteStream> bs = ByteStream::create();
  bs->write(xml.data(), xml.size());
  bs->seek(0, SEEK_SET);
  return bs;
}

GP<ByteStream> OcrStub(void *, const GUTF8String &, const GP<DjVuImage> &)
{
  static const char kText[] =
      "<HIDDENTEXT><PAGECOLUMN coords=\"0,0,10,10\"><WORD coords=\"1,1,2,2\">"
      "ocr"
      "</WORD></PAGECOLUMN></HIDDENTEXT>";
  GP<ByteStream> bs = ByteStream::create();
  bs->write(kText, sizeof(kText) - 1);
  bs->seek(0, SEEK_SET);
  return bs;
}

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

std::string ReadAll(const GP<ByteStream> &bs)
{
  GP<ByteStream> in = bs;
  in->seek(0, SEEK_SET);
  std::string out;
  char chunk[512];
  while (const size_t got = in->read(chunk, sizeof(chunk)))
    out.append(chunk, got);
  return out;
}

}  // namespace

TEST(XMLParserTest, CreateAndLifecycleNoOpsDoNotThrow)
{
  GP<lt_XMLParser> parser = lt_XMLParser::create();
  ASSERT_TRUE(parser != 0);

  EXPECT_NO_THROW(parser->empty());
  EXPECT_NO_THROW(parser->save());
}

TEST(XMLParserTest, ParseWithoutSingleBodyThrows)
{
  GP<lt_XMLParser> parser = lt_XMLParser::create();
  ASSERT_TRUE(parser != 0);

  GP<ByteStream> bs = ByteStream::create();
  const char xml[] = "<ROOT/>";
  ASSERT_EQ(sizeof(xml) - 1, bs->write(xml, sizeof(xml) - 1));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  EXPECT_THROW(parser->parse(bs, nullptr), GException);
}

TEST(XMLParserTest, ParseWithMultipleBodyTagsIsAccepted)
{
  GP<lt_XMLParser> parser = lt_XMLParser::create();
  ASSERT_TRUE(parser != 0);

  GP<ByteStream> bs = ByteStream::create();
  const char xml[] = "<ROOT><BODY/><BODY/></ROOT>";
  ASSERT_EQ(sizeof(xml) - 1, bs->write(xml, sizeof(xml) - 1));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  GP<lt_XMLTags> tags = lt_XMLTags::create(bs);
  ASSERT_TRUE(tags != 0);

  EXPECT_NO_THROW(parser->parse(*tags, nullptr));
}

TEST(XMLParserTest, ParseAndSaveObjectWithMapTextAndMetadata)
{
  const std::filesystem::path path = MakeTempXmlParserPath("full");
  const GURL djvu_url = WriteSinglePageDjvu(path);

  const std::string xml =
      "<DjVuXML><BODY>"
      "<OBJECT data=\"" +
      std::string(djvu_url.get_string()) +
      "\" type=\"image/x.djvu\" width=\"80\" height=\"60\" usemap=\"main\">"
      "<PARAM name=\"dpi\" value=\"301\"/>"
      "<PARAM name=\"gamma\" value=\"2.5\"/>"
      "<MAP name=\"main\">"
      "<AREA shape=\"rect\" coords=\"1,2,10,20\" url=\"https://example.com\"/>"
      "</MAP>"
      "<HIDDENTEXT><PAGECOLUMN coords=\"0,0,20,20\"><WORD coords=\"1,2,5,6\">"
      "hello"
      "</WORD></PAGECOLUMN></HIDDENTEXT>"
      "<METADATA><meta name=\"k\" value=\"v\"/></METADATA>"
      "</OBJECT>"
      "</BODY></DjVuXML>";

  GP<lt_XMLParser> parser = lt_XMLParser::create();
  ASSERT_TRUE(parser != 0);
  EXPECT_NO_THROW(parser->parse(MakeXmlStream(xml), nullptr));
  EXPECT_NO_THROW(parser->save());

  GP<DjVuDocument> doc = DjVuDocument::create_wait(djvu_url);
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());
  const GUTF8String id = doc->page_to_id(0);
  GP<DjVuFile> file = doc->get_djvu_file(id, false);
  ASSERT_TRUE(file != 0);
  file->resume_decode(true);

  EXPECT_TRUE(file->contains_anno());
  EXPECT_TRUE(file->contains_text());
  EXPECT_TRUE(file->contains_meta());

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(XMLParserTest, UnknownUseMapThrows)
{
  const std::filesystem::path path = MakeTempXmlParserPath("bad_map");
  const GURL djvu_url = WriteSinglePageDjvu(path);

  const std::string xml =
      "<DjVuXML><BODY>"
      "<OBJECT data=\"" +
      std::string(djvu_url.get_string()) +
      "\" type=\"image/x.djvu\" usemap=\"missing\"/>"
      "</BODY></DjVuXML>";

  GP<lt_XMLParser> parser = lt_XMLParser::create();
  ASSERT_TRUE(parser != 0);
  EXPECT_THROW(parser->parse(MakeXmlStream(xml), nullptr), GException);

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(XMLParserTest, InvalidAreaColorThrows)
{
  const std::filesystem::path path = MakeTempXmlParserPath("bad_color");
  const GURL djvu_url = WriteSinglePageDjvu(path);

  const std::string xml =
      "<DjVuXML><BODY>"
      "<OBJECT data=\"" +
      std::string(djvu_url.get_string()) +
      "\" type=\"image/x.djvu\" usemap=\"m\">"
      "<MAP name=\"m\"><AREA shape=\"rect\" coords=\"1,1,2,2\" bordercolor=\"oops\"/>"
      "</MAP></OBJECT></BODY></DjVuXML>";

  GP<lt_XMLParser> parser = lt_XMLParser::create();
  ASSERT_TRUE(parser != 0);
  EXPECT_THROW(parser->parse(MakeXmlStream(xml), nullptr), GException);

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(XMLParserTest, OcrCallbackCanInjectHiddenText)
{
  const std::filesystem::path path = MakeTempXmlParserPath("ocr");
  const GURL djvu_url = WriteSinglePageDjvu(path);
  lt_XMLParser::setOCRcallback(nullptr, OcrStub);

  const std::string xml =
      "<DjVuXML><BODY>"
      "<OBJECT data=\"" +
      std::string(djvu_url.get_string()) +
      "\" type=\"image/x.djvu\">"
      "<PARAM name=\"ocr\" value=\"stub\"/>"
      "</OBJECT></BODY></DjVuXML>";

  GP<lt_XMLParser> parser = lt_XMLParser::create();
  ASSERT_TRUE(parser != 0);
  EXPECT_NO_THROW(parser->parse(MakeXmlStream(xml), nullptr));
  EXPECT_NO_THROW(parser->save());

  GP<DjVuDocument> doc = DjVuDocument::create_wait(djvu_url);
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());
  GP<DjVuFile> file = doc->get_djvu_file(doc->page_to_id(0), false);
  ASSERT_TRUE(file != 0);
  file->resume_decode(true);
  EXPECT_TRUE(file->contains_text());

  lt_XMLParser::setOCRcallback(nullptr, nullptr);

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(XMLParserTest, ReferenceOcrFixtureXmlRoundtripCanBeParsedAndSaved)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("sp_utf8_multilang_hidden.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const std::filesystem::path temp_path = MakeTempXmlParserPath("ref_ocr_roundtrip");
  std::filesystem::copy_file(*fixture, temp_path, std::filesystem::copy_options::overwrite_existing);
  const GURL temp_url = GURL::Filename::UTF8(temp_path.string().c_str());

  GP<DjVuDocument> src = DjVuDocument::create_wait(temp_url);
  ASSERT_TRUE(src != 0);
  ASSERT_TRUE(src->is_init_complete());
  GP<DjVuImage> page = src->get_page(0, true);
  ASSERT_TRUE(page != 0);
  GP<DjVuFile> file = page->get_djvu_file();
  ASSERT_TRUE(file != 0);
  EXPECT_NO_THROW(file->resume_decode(true));
  ASSERT_TRUE(file->contains_text());

  GP<DjVuText> hidden = DjVuText::create();
  ASSERT_TRUE(hidden != 0);
  GP<ByteStream> text_bs = file->get_text();
  ASSERT_TRUE(text_bs != 0);
  EXPECT_NO_THROW(hidden->decode(text_bs));
  ASSERT_TRUE(hidden->txt != 0);

  const GUTF8String hidden_xml = hidden->get_xmlText(page->get_height());
  const std::string xml =
      "<DjVuXML><BODY><OBJECT data=\"" + std::string(temp_url.get_string()) +
      "\" type=\"image/x.djvu\">" + std::string((const char *)hidden_xml) +
      "</OBJECT></BODY></DjVuXML>";

  GP<lt_XMLParser> parser = lt_XMLParser::create();
  ASSERT_TRUE(parser != 0);
  EXPECT_NO_THROW(parser->parse(MakeXmlStream(xml), nullptr));
  EXPECT_NO_THROW(parser->save());

  GP<DjVuDocument> reopened = DjVuDocument::create_wait(temp_url);
  ASSERT_TRUE(reopened != 0);
  ASSERT_TRUE(reopened->is_init_complete());
  GP<DjVuFile> reopened_file = reopened->get_djvu_file(reopened->page_to_id(0), false);
  ASSERT_TRUE(reopened_file != 0);
  EXPECT_NO_THROW(reopened_file->resume_decode(true));
  EXPECT_TRUE(reopened_file->contains_text());

  std::error_code ec;
  std::filesystem::remove(temp_path, ec);
}

TEST(XMLParserTest, ReferenceFixtureWriteXmlRoundtripPreservesMapAndHiddenText)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("sp_bg_fgtext_hidden_links.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const std::filesystem::path temp_path = MakeTempXmlParserPath("ref_map_text_roundtrip");
  std::filesystem::copy_file(*fixture, temp_path, std::filesystem::copy_options::overwrite_existing);
  const GURL temp_url = GURL::Filename::UTF8(temp_path.string().c_str());

  GP<DjVuDocument> src = DjVuDocument::create_wait(temp_url);
  ASSERT_TRUE(src != 0);
  ASSERT_TRUE(src->is_init_complete());
  GP<DjVuImage> page = src->get_page(0, true);
  ASSERT_TRUE(page != 0);

  GP<ByteStream> xml_bs = ByteStream::create();
  EXPECT_NO_THROW(page->writeXML(*xml_bs, temp_url));
  const std::string xml =
      "<DjVuXML><BODY>" + ReadAll(xml_bs) + "</BODY></DjVuXML>";

  GP<lt_XMLParser> parser = lt_XMLParser::create();
  ASSERT_TRUE(parser != 0);
  EXPECT_NO_THROW(parser->parse(MakeXmlStream(xml), nullptr));
  EXPECT_NO_THROW(parser->save());

  GP<DjVuDocument> reopened = DjVuDocument::create_wait(temp_url);
  ASSERT_TRUE(reopened != 0);
  ASSERT_TRUE(reopened->is_init_complete());
  GP<DjVuFile> file = reopened->get_djvu_file(reopened->page_to_id(0), false);
  ASSERT_TRUE(file != 0);
  EXPECT_NO_THROW(file->resume_decode(true));
  EXPECT_TRUE(file->contains_anno());
  EXPECT_TRUE(file->contains_text());

  GP<DjVuImage> reopened_page = reopened->get_page(0, true);
  ASSERT_TRUE(reopened_page != 0);
  GP<ByteStream> reopened_xml = ByteStream::create();
  EXPECT_NO_THROW(reopened_page->writeXML(*reopened_xml, temp_url));
  const std::string reopened_text = ReadAll(reopened_xml);
  EXPECT_NE(std::string::npos, reopened_text.find("<MAP"));
  EXPECT_NE(std::string::npos, reopened_text.find("HIDDENTEXT"));

  std::error_code ec;
  std::filesystem::remove(temp_path, ec);
}

TEST(XMLParserTest, ReferenceFixtureMetadataInjectionPersistsAfterSave)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("sp_hidden_only_no_visible.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const std::filesystem::path temp_path = MakeTempXmlParserPath("ref_meta_inject");
  std::filesystem::copy_file(*fixture, temp_path, std::filesystem::copy_options::overwrite_existing);
  const GURL temp_url = GURL::Filename::UTF8(temp_path.string().c_str());

  GP<DjVuDocument> src = DjVuDocument::create_wait(temp_url);
  ASSERT_TRUE(src != 0);
  ASSERT_TRUE(src->is_init_complete());
  GP<DjVuFile> file = src->get_djvu_file(src->page_to_id(0), false);
  ASSERT_TRUE(file != 0);
  EXPECT_NO_THROW(file->resume_decode(true));
  ASSERT_TRUE(file->contains_text());

  GP<DjVuText> hidden = DjVuText::create();
  ASSERT_TRUE(hidden != 0);
  GP<ByteStream> text_bs = file->get_text();
  ASSERT_TRUE(text_bs != 0);
  EXPECT_NO_THROW(hidden->decode(text_bs));
  ASSERT_TRUE(hidden->txt != 0);

  GP<DjVuImage> page = src->get_page(0, true);
  ASSERT_TRUE(page != 0);
  const GUTF8String hidden_xml = hidden->get_xmlText(page->get_height());
  const std::string xml =
      "<DjVuXML><BODY><OBJECT data=\"" + std::string(temp_url.get_string()) +
      "\" type=\"image/x.djvu\">" + std::string((const char *)hidden_xml) +
      "<METADATA><meta name=\"fixture\" value=\"reference\"/>"
      "<meta name=\"kind\" value=\"hidden-only\"/></METADATA>"
      "</OBJECT></BODY></DjVuXML>";

  GP<lt_XMLParser> parser = lt_XMLParser::create();
  ASSERT_TRUE(parser != 0);
  EXPECT_NO_THROW(parser->parse(MakeXmlStream(xml), nullptr));
  EXPECT_NO_THROW(parser->save());

  GP<DjVuDocument> reopened = DjVuDocument::create_wait(temp_url);
  ASSERT_TRUE(reopened != 0);
  ASSERT_TRUE(reopened->is_init_complete());
  GP<DjVuFile> reopened_file = reopened->get_djvu_file(reopened->page_to_id(0), false);
  ASSERT_TRUE(reopened_file != 0);
  EXPECT_NO_THROW(reopened_file->resume_decode(true));
  EXPECT_TRUE(reopened_file->contains_text());
  EXPECT_TRUE(reopened_file->contains_meta());

  GP<DjVuImage> reopened_page = reopened->get_page(0, true);
  ASSERT_TRUE(reopened_page != 0);
  GP<ByteStream> reopened_xml = ByteStream::create();
  EXPECT_NO_THROW(reopened_page->writeXML(*reopened_xml, temp_url));
  EXPECT_NE(std::string::npos, ReadAll(reopened_xml).find("<METADATA"));

  std::error_code ec;
  std::filesystem::remove(temp_path, ec);
}
