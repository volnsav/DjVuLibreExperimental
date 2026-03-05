#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>

#include "DjVuDocument.h"
#include "DjVuFile.h"
#include "DjVuImage.h"
#include "DjVuText.h"

namespace {

std::optional<std::filesystem::path> FindFixtureRoot()
{
  const std::filesystem::path p1("tests/fixtures/reference");
  if (std::filesystem::exists(p1))
    return p1;
  const std::filesystem::path p2("fixtures/reference");
  if (std::filesystem::exists(p2))
    return p2;
  return std::nullopt;
}

std::optional<std::filesystem::path> FindFixtureFile(const char *name)
{
  const std::optional<std::filesystem::path> root = FindFixtureRoot();
  if (!root)
    return std::nullopt;
  const std::filesystem::path p = *root / name;
  if (!std::filesystem::exists(p))
    return std::nullopt;
  return p;
}

GP<DjVuDocument> OpenFixtureDocument(const std::filesystem::path &path)
{
  const GURL url = GURL::Filename::UTF8(path.string().c_str());
  return DjVuDocument::create_wait(url);
}

std::string TryExtractHiddenText(const GP<DjVuFile> &file)
{
  if (file == 0)
    return std::string();
  try
  {
    GP<ByteStream> text_bs = file->get_text();
    if (text_bs == 0)
      return std::string();
    GP<DjVuTXT> txt = DjVuTXT::create();
    txt->decode(text_bs);
    return std::string((const char *)txt->textUTF8);
  }
  catch (...)
  {
    return std::string();
  }
}

}  // namespace

TEST(ReferenceFixturesTest, SinglePageDocumentsLoadAndExposeExpectedGeometry)
{
  if (!FindFixtureRoot())
    GTEST_SKIP() << "tests/fixtures/reference not found";

  struct Sample
  {
    const char *name;
    int min_width;
    int min_height;
  };

  const Sample samples[] = {
    {"sp_bg_only.djvu", 700, 500},
    {"sp_bg_fgtext.djvu", 700, 500},
    {"sp_bg_fgtext_hidden.djvu", 700, 500},
    {"sp_bg_fgtext_hidden_links.djvu", 700, 500},
    {"sp_unicode_hidden.djvu", 700, 500},
    {"sp_bg_only_large.djvu", 1800, 1400},
  };

  for (const Sample &s : samples)
  {
    const std::optional<std::filesystem::path> fixture = FindFixtureFile(s.name);
    ASSERT_TRUE(fixture.has_value()) << "Missing fixture: " << s.name;

    GP<DjVuDocument> doc = OpenFixtureDocument(*fixture);
    ASSERT_TRUE(doc != 0) << s.name;
    ASSERT_TRUE(doc->is_init_ok()) << s.name;
    EXPECT_EQ(1, doc->get_pages_num()) << s.name;

    const int type = doc->get_doc_type();
    EXPECT_TRUE(type == DjVuDocument::SINGLE_PAGE || type == DjVuDocument::BUNDLED) << s.name;

    GP<DjVuImage> page = doc->get_page(0, true);
    ASSERT_TRUE(page != 0) << s.name;
    EXPECT_GE(page->get_width(), s.min_width) << s.name;
    EXPECT_GE(page->get_height(), s.min_height) << s.name;
  }
}

TEST(ReferenceFixturesTest, SinglePageTextAndAnnotationSignalsMatchManifest)
{
  if (!FindFixtureRoot())
    GTEST_SKIP() << "tests/fixtures/reference not found";

  const std::optional<std::filesystem::path> bg_only = FindFixtureFile("sp_bg_only.djvu");
  const std::optional<std::filesystem::path> hidden = FindFixtureFile("sp_bg_fgtext_hidden.djvu");
  const std::optional<std::filesystem::path> links = FindFixtureFile("sp_bg_fgtext_hidden_links.djvu");
  const std::optional<std::filesystem::path> unicode = FindFixtureFile("sp_unicode_hidden.djvu");
  ASSERT_TRUE(bg_only.has_value());
  ASSERT_TRUE(hidden.has_value());
  ASSERT_TRUE(links.has_value());
  ASSERT_TRUE(unicode.has_value());

  GP<DjVuDocument> doc_bg_only = OpenFixtureDocument(*bg_only);
  GP<DjVuDocument> doc_hidden = OpenFixtureDocument(*hidden);
  GP<DjVuDocument> doc_links = OpenFixtureDocument(*links);
  GP<DjVuDocument> doc_unicode = OpenFixtureDocument(*unicode);
  ASSERT_TRUE(doc_bg_only != 0 && doc_bg_only->is_init_ok());
  ASSERT_TRUE(doc_hidden != 0 && doc_hidden->is_init_ok());
  ASSERT_TRUE(doc_links != 0 && doc_links->is_init_ok());
  ASSERT_TRUE(doc_unicode != 0 && doc_unicode->is_init_ok());

  GP<DjVuFile> file_bg_only = doc_bg_only->get_djvu_file(0, false);
  GP<DjVuFile> file_hidden = doc_hidden->get_djvu_file(0, false);
  GP<DjVuFile> file_links = doc_links->get_djvu_file(0, false);
  GP<DjVuFile> file_unicode = doc_unicode->get_djvu_file(0, false);
  ASSERT_TRUE(file_bg_only != 0);
  ASSERT_TRUE(file_hidden != 0);
  ASSERT_TRUE(file_links != 0);
  ASSERT_TRUE(file_unicode != 0);

  EXPECT_FALSE(file_bg_only->contains_text());
  EXPECT_FALSE(file_bg_only->contains_anno());

  EXPECT_TRUE(file_hidden->contains_text());
  EXPECT_TRUE(file_links->contains_text());
  EXPECT_TRUE(file_links->contains_anno());
  EXPECT_TRUE(file_unicode->contains_text());

  const std::string unicode_hidden = TryExtractHiddenText(file_unicode);
  if (!unicode_hidden.empty())
  {
    const bool has_non_ascii = std::any_of(unicode_hidden.begin(), unicode_hidden.end(),
                                           [](unsigned char c) { return c >= 0x80; });
    EXPECT_TRUE(has_non_ascii);
    EXPECT_NE(std::string::npos, unicode_hidden.find("DjVu"));
  }
}

TEST(ReferenceFixturesTest, MultiPageBundledAndIndirectDocumentsLoadAndMapPages)
{
  if (!FindFixtureRoot())
    GTEST_SKIP() << "tests/fixtures/reference not found";

  const std::optional<std::filesystem::path> bundled = FindFixtureFile("mp2_bundled.djvu");
  const std::optional<std::filesystem::path> indirect =
    FindFixtureFile("mp2_indirect/index.djvu");
  ASSERT_TRUE(bundled.has_value());
  ASSERT_TRUE(indirect.has_value());

  GP<DjVuDocument> doc_bundled = OpenFixtureDocument(*bundled);
  GP<DjVuDocument> doc_indirect = OpenFixtureDocument(*indirect);
  ASSERT_TRUE(doc_bundled != 0 && doc_bundled->is_init_ok());
  ASSERT_TRUE(doc_indirect != 0 && doc_indirect->is_init_ok());

  EXPECT_EQ(DjVuDocument::BUNDLED, doc_bundled->get_doc_type());
  EXPECT_EQ(DjVuDocument::INDIRECT, doc_indirect->get_doc_type());
  EXPECT_EQ(2, doc_bundled->get_pages_num());
  EXPECT_EQ(2, doc_indirect->get_pages_num());

  for (int i = 0; i < 2; ++i)
  {
    const GURL b_url = doc_bundled->page_to_url(i);
    const GUTF8String b_id = doc_bundled->page_to_id(i);
    EXPECT_FALSE(b_url.is_empty());
    EXPECT_GT(b_id.length(), 0);
    EXPECT_EQ(i, doc_bundled->id_to_page(b_id));

    const GURL i_url = doc_indirect->page_to_url(i);
    const GUTF8String i_id = doc_indirect->page_to_id(i);
    EXPECT_FALSE(i_url.is_empty());
    EXPECT_GT(i_id.length(), 0);
    EXPECT_EQ(i, doc_indirect->id_to_page(i_id));

    GP<DjVuImage> b_page = doc_bundled->get_page(i, true);
    GP<DjVuImage> i_page = doc_indirect->get_page(i, true);
    ASSERT_TRUE(b_page != 0);
    ASSERT_TRUE(i_page != 0);
    EXPECT_GT(b_page->get_width(), 0);
    EXPECT_GT(b_page->get_height(), 0);
    EXPECT_GT(i_page->get_width(), 0);
    EXPECT_GT(i_page->get_height(), 0);
  }

  const GList<GURL> bundled_names = doc_bundled->get_url_names();
  const GList<GURL> indirect_names = doc_indirect->get_url_names();
  EXPECT_GE(bundled_names.size(), 2);
  EXPECT_GE(indirect_names.size(), 2);
}
