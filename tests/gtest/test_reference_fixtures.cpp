#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>

#include "DjVuDocument.h"
#include "DjVuFile.h"
#include "DjVuImage.h"
#include "DjVmDir.h"
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
    {"sp_anno_only_links.djvu", 700, 500},
    {"sp_unicode_hidden.djvu", 700, 500},
    {"sp_hidden_only_no_visible.djvu", 700, 500},
    {"sp_rotation_90.djvu", 550, 750},
    {"sp_rotation_180.djvu", 700, 500},
    {"sp_rotation_270.djvu", 550, 750},
    {"sp_palette_indexed_bg.djvu", 700, 500},
    {"sp_bw_mask_heavy.djvu", 700, 500},
    {"sp_utf8_multilang_hidden.djvu", 700, 500},
    {"sp_links_edgecases.djvu", 700, 500},
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
  const std::optional<std::filesystem::path> anno_only =
    FindFixtureFile("sp_anno_only_links.djvu");
  const std::optional<std::filesystem::path> hidden = FindFixtureFile("sp_bg_fgtext_hidden.djvu");
  const std::optional<std::filesystem::path> hidden_only =
    FindFixtureFile("sp_hidden_only_no_visible.djvu");
  const std::optional<std::filesystem::path> links = FindFixtureFile("sp_bg_fgtext_hidden_links.djvu");
  const std::optional<std::filesystem::path> unicode = FindFixtureFile("sp_unicode_hidden.djvu");
  const std::optional<std::filesystem::path> utf8_multilang =
    FindFixtureFile("sp_utf8_multilang_hidden.djvu");
  const std::optional<std::filesystem::path> links_edgecases =
    FindFixtureFile("sp_links_edgecases.djvu");
  ASSERT_TRUE(bg_only.has_value());
  ASSERT_TRUE(anno_only.has_value());
  ASSERT_TRUE(hidden.has_value());
  ASSERT_TRUE(hidden_only.has_value());
  ASSERT_TRUE(links.has_value());
  ASSERT_TRUE(unicode.has_value());
  ASSERT_TRUE(utf8_multilang.has_value());
  ASSERT_TRUE(links_edgecases.has_value());

  GP<DjVuDocument> doc_bg_only = OpenFixtureDocument(*bg_only);
  GP<DjVuDocument> doc_anno_only = OpenFixtureDocument(*anno_only);
  GP<DjVuDocument> doc_hidden = OpenFixtureDocument(*hidden);
  GP<DjVuDocument> doc_hidden_only = OpenFixtureDocument(*hidden_only);
  GP<DjVuDocument> doc_links = OpenFixtureDocument(*links);
  GP<DjVuDocument> doc_unicode = OpenFixtureDocument(*unicode);
  GP<DjVuDocument> doc_utf8_multilang = OpenFixtureDocument(*utf8_multilang);
  GP<DjVuDocument> doc_links_edgecases = OpenFixtureDocument(*links_edgecases);
  ASSERT_TRUE(doc_bg_only != 0 && doc_bg_only->is_init_ok());
  ASSERT_TRUE(doc_anno_only != 0 && doc_anno_only->is_init_ok());
  ASSERT_TRUE(doc_hidden != 0 && doc_hidden->is_init_ok());
  ASSERT_TRUE(doc_hidden_only != 0 && doc_hidden_only->is_init_ok());
  ASSERT_TRUE(doc_links != 0 && doc_links->is_init_ok());
  ASSERT_TRUE(doc_unicode != 0 && doc_unicode->is_init_ok());
  ASSERT_TRUE(doc_utf8_multilang != 0 && doc_utf8_multilang->is_init_ok());
  ASSERT_TRUE(doc_links_edgecases != 0 && doc_links_edgecases->is_init_ok());

  GP<DjVuFile> file_bg_only = doc_bg_only->get_djvu_file(0, false);
  GP<DjVuFile> file_anno_only = doc_anno_only->get_djvu_file(0, false);
  GP<DjVuFile> file_hidden = doc_hidden->get_djvu_file(0, false);
  GP<DjVuFile> file_hidden_only = doc_hidden_only->get_djvu_file(0, false);
  GP<DjVuFile> file_links = doc_links->get_djvu_file(0, false);
  GP<DjVuFile> file_unicode = doc_unicode->get_djvu_file(0, false);
  GP<DjVuFile> file_utf8_multilang = doc_utf8_multilang->get_djvu_file(0, false);
  GP<DjVuFile> file_links_edgecases = doc_links_edgecases->get_djvu_file(0, false);
  ASSERT_TRUE(file_bg_only != 0);
  ASSERT_TRUE(file_anno_only != 0);
  ASSERT_TRUE(file_hidden != 0);
  ASSERT_TRUE(file_hidden_only != 0);
  ASSERT_TRUE(file_links != 0);
  ASSERT_TRUE(file_unicode != 0);
  ASSERT_TRUE(file_utf8_multilang != 0);
  ASSERT_TRUE(file_links_edgecases != 0);

  EXPECT_FALSE(file_bg_only->contains_text());
  EXPECT_FALSE(file_bg_only->contains_anno());

  EXPECT_FALSE(file_anno_only->contains_text());
  EXPECT_TRUE(file_anno_only->contains_anno());

  EXPECT_TRUE(file_hidden->contains_text());
  EXPECT_TRUE(file_hidden_only->contains_text());
  EXPECT_FALSE(file_hidden_only->contains_anno());
  EXPECT_TRUE(file_links->contains_text());
  EXPECT_TRUE(file_links->contains_anno());
  EXPECT_TRUE(file_unicode->contains_text());
  EXPECT_TRUE(file_utf8_multilang->contains_text());
  EXPECT_TRUE(file_links_edgecases->contains_anno());

  const std::string hidden_only_text = TryExtractHiddenText(file_hidden_only);
  if (!hidden_only_text.empty())
  {
    EXPECT_NE(std::string::npos, hidden_only_text.find("HIDDEN_ONLY_TEXT_123"));
  }

  const std::string unicode_hidden = TryExtractHiddenText(file_unicode);
  if (!unicode_hidden.empty())
  {
    const bool has_non_ascii = std::any_of(unicode_hidden.begin(), unicode_hidden.end(),
                                           [](unsigned char c) { return c >= 0x80; });
    EXPECT_TRUE(has_non_ascii);
    EXPECT_NE(std::string::npos, unicode_hidden.find("DjVu"));
  }

  const std::string utf8_multilang_hidden = TryExtractHiddenText(file_utf8_multilang);
  if (!utf8_multilang_hidden.empty())
  {
    const bool has_non_ascii = std::any_of(utf8_multilang_hidden.begin(),
                                           utf8_multilang_hidden.end(),
                                           [](unsigned char c) { return c >= 0x80; });
    EXPECT_TRUE(has_non_ascii);
    EXPECT_NE(std::string::npos, utf8_multilang_hidden.find("DjVu"));
    EXPECT_NE(std::string::npos, utf8_multilang_hidden.find("test 123"));
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

TEST(ReferenceFixturesTest, ExtendedMultiPageAndSharedResourceFixturesLoad)
{
  if (!FindFixtureRoot())
    GTEST_SKIP() << "tests/fixtures/reference not found";

  const std::optional<std::filesystem::path> bundled =
    FindFixtureFile("mp3_bundled_mixed_layers.djvu");
  const std::optional<std::filesystem::path> indirect =
    FindFixtureFile("mp3_indirect_mixed_layers/index.djvu");
  const std::optional<std::filesystem::path> shared =
    FindFixtureFile("mp2_bundled_shared_resources.djvu");
  ASSERT_TRUE(bundled.has_value());
  ASSERT_TRUE(indirect.has_value());
  ASSERT_TRUE(shared.has_value());

  GP<DjVuDocument> doc_bundled = OpenFixtureDocument(*bundled);
  GP<DjVuDocument> doc_indirect = OpenFixtureDocument(*indirect);
  GP<DjVuDocument> doc_shared = OpenFixtureDocument(*shared);
  ASSERT_TRUE(doc_bundled != 0 && doc_bundled->is_init_ok());
  ASSERT_TRUE(doc_indirect != 0 && doc_indirect->is_init_ok());
  ASSERT_TRUE(doc_shared != 0 && doc_shared->is_init_ok());

  EXPECT_EQ(DjVuDocument::BUNDLED, doc_bundled->get_doc_type());
  EXPECT_EQ(DjVuDocument::INDIRECT, doc_indirect->get_doc_type());
  EXPECT_EQ(DjVuDocument::BUNDLED, doc_shared->get_doc_type());
  EXPECT_EQ(3, doc_bundled->get_pages_num());
  EXPECT_EQ(3, doc_indirect->get_pages_num());
  EXPECT_EQ(2, doc_shared->get_pages_num());

  for (int i = 0; i < 3; ++i)
  {
    EXPECT_EQ(i, doc_bundled->id_to_page(doc_bundled->page_to_id(i)));
    EXPECT_EQ(i, doc_indirect->id_to_page(doc_indirect->page_to_id(i)));

    GP<DjVuImage> bundled_page = doc_bundled->get_page(i, true);
    GP<DjVuImage> indirect_page = doc_indirect->get_page(i, true);
    ASSERT_TRUE(bundled_page != 0);
    ASSERT_TRUE(indirect_page != 0);
    EXPECT_GT(bundled_page->get_width(), 0);
    EXPECT_GT(bundled_page->get_height(), 0);
    EXPECT_GT(indirect_page->get_width(), 0);
    EXPECT_GT(indirect_page->get_height(), 0);
  }

  GP<DjVuFile> bundled_page0 = doc_bundled->get_djvu_file(0, false);
  GP<DjVuFile> bundled_page1 = doc_bundled->get_djvu_file(1, false);
  GP<DjVuFile> bundled_page2 = doc_bundled->get_djvu_file(2, false);
  ASSERT_TRUE(bundled_page0 != 0);
  ASSERT_TRUE(bundled_page1 != 0);
  ASSERT_TRUE(bundled_page2 != 0);
  EXPECT_FALSE(bundled_page0->contains_text());
  EXPECT_FALSE(bundled_page0->contains_anno());
  EXPECT_FALSE(bundled_page1->contains_text());
  EXPECT_FALSE(bundled_page1->contains_anno());
  EXPECT_TRUE(bundled_page2->contains_text());
  EXPECT_TRUE(bundled_page2->contains_anno());
  const std::string bundled_page2_text = TryExtractHiddenText(bundled_page2);
  if (!bundled_page2_text.empty())
  {
    EXPECT_NE(std::string::npos, bundled_page2_text.find("HIDDEN_TEXT_PAGE3_XYZ"));
  }

  for (int i = 0; i < 2; ++i)
  {
    GP<DjVuImage> shared_page = doc_shared->get_page(i, true);
    GP<DjVuFile> shared_file = doc_shared->get_djvu_file(i, false);
    ASSERT_TRUE(shared_page != 0);
    ASSERT_TRUE(shared_file != 0);
    EXPECT_GT(shared_page->get_width(), 0);
    EXPECT_GT(shared_page->get_height(), 0);
    EXPECT_FALSE(shared_file->contains_text());
    EXPECT_FALSE(shared_file->contains_anno());
  }
}

TEST(ReferenceFixturesTest, RealBundledLegacyExamplesExposeNavmAndAntzSignals)
{
  if (!FindFixtureRoot())
    GTEST_SKIP() << "tests/fixtures/reference not found";

  const std::optional<std::filesystem::path> navm =
    FindFixtureFile("real_lizard2003_navm.djvu");
  const std::optional<std::filesystem::path> antz =
    FindFixtureFile("real_lizard2005_antz.djvu");
  ASSERT_TRUE(navm.has_value());
  ASSERT_TRUE(antz.has_value());

  GP<DjVuDocument> navm_doc = OpenFixtureDocument(*navm);
  GP<DjVuDocument> antz_doc = OpenFixtureDocument(*antz);
  ASSERT_TRUE(navm_doc != 0 && navm_doc->is_init_ok());
  ASSERT_TRUE(antz_doc != 0 && antz_doc->is_init_ok());

  EXPECT_EQ(DjVuDocument::BUNDLED, navm_doc->get_doc_type());
  EXPECT_EQ(DjVuDocument::BUNDLED, antz_doc->get_doc_type());
  EXPECT_EQ(6, navm_doc->get_pages_num());
  EXPECT_EQ(1, antz_doc->get_pages_num());

  EXPECT_TRUE(navm_doc->get_djvm_nav() != 0);

  GP<DjVuFile> navm_page0 = navm_doc->get_djvu_file(0, false);
  GP<DjVuFile> antz_page0 = antz_doc->get_djvu_file(0, false);
  ASSERT_TRUE(navm_page0 != 0);
  ASSERT_TRUE(antz_page0 != 0);

  EXPECT_TRUE(navm_page0->contains_anno());
  EXPECT_TRUE(antz_page0->contains_anno());
  EXPECT_FALSE(antz_page0->contains_text());

  GP<DjVuImage> navm_image0 = navm_doc->get_page(0, true);
  GP<DjVuImage> antz_image0 = antz_doc->get_page(0, true);
  ASSERT_TRUE(navm_image0 != 0);
  ASSERT_TRUE(antz_image0 != 0);
  EXPECT_GT(navm_image0->get_width(), 0);
  EXPECT_GT(navm_image0->get_height(), 0);
  EXPECT_GT(antz_image0->get_width(), 0);
  EXPECT_GT(antz_image0->get_height(), 0);
}

TEST(ReferenceFixturesTest, RealBundledRussianBookActsAsComplexReferenceDocument)
{
  if (!FindFixtureRoot())
    GTEST_SKIP() << "tests/fixtures/reference not found";

  const std::optional<std::filesystem::path> book =
    FindFixtureFile("real_djvulibre_book_ru.djvu");
  ASSERT_TRUE(book.has_value());

  GP<DjVuDocument> doc = OpenFixtureDocument(*book);
  ASSERT_TRUE(doc != 0 && doc->is_init_ok());

  EXPECT_EQ(DjVuDocument::BUNDLED, doc->get_doc_type());
  EXPECT_EQ(48, doc->get_pages_num());
  EXPECT_TRUE(doc->get_djvm_nav() != 0);

  GP<DjVmDir> dir = doc->get_djvm_dir();
  ASSERT_TRUE(dir != 0);
  EXPECT_TRUE(dir->get_shared_anno_file() != 0);
  EXPECT_GE(dir->get_files_num(), 49);

  const GList<GURL> names = doc->get_url_names();
  EXPECT_GE(names.size(), 48);

  for (int i = 0; i < 3; ++i)
  {
    GP<DjVuFile> file = doc->get_djvu_file(i, false);
    GP<DjVuImage> page = doc->get_page(i, true);
    ASSERT_TRUE(file != 0);
    ASSERT_TRUE(page != 0);
    EXPECT_TRUE(file->contains_text());
    EXPECT_TRUE(file->contains_anno());
    EXPECT_GT(page->get_width(), 3000);
    EXPECT_GT(page->get_height(), 4000);
  }

  const std::string hidden_text = TryExtractHiddenText(doc->get_djvu_file(0, false));
  if (!hidden_text.empty())
  {
    const bool has_non_ascii = std::any_of(hidden_text.begin(), hidden_text.end(),
                                           [](unsigned char c) { return c >= 0x80; });
    EXPECT_TRUE(has_non_ascii);
  }
}
