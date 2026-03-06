#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "BSByteStream.h"
#include "ByteStream.h"
#include "DjVuAnno.h"
#include "DjVuDocument.h"
#include "DjVuFile.h"
#include "DjVuImage.h"
#include "DjVuInfo.h"
#include "DjVuPalette.h"
#include "DjVuText.h"
#include "IFFByteStream.h"
#include "JB2Image.h"

namespace {

GP<ByteStream> MakeDjvuWithInfo()
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVU");
  iff->put_chunk("INFO");

  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 40;
  info->height = 20;
  info->dpi = 400;
  info->gamma = 1.8;
  info->version = 24;
  info->encode(*iff->get_bytestream());

  iff->close_chunk();
  iff->close_chunk();
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

struct XmlSignals
{
  bool has_object = false;
  bool has_map = false;
  bool has_hidden = false;
  bool has_meta = false;
  size_t area_count = 0;
  size_t word_count = 0;

  bool operator==(const XmlSignals &other) const
  {
    return has_object == other.has_object &&
           has_map == other.has_map &&
           has_hidden == other.has_hidden &&
           has_meta == other.has_meta &&
           area_count == other.area_count &&
           word_count == other.word_count;
  }
};

size_t CountOccurrences(const std::string &text, const std::string &needle)
{
  size_t count = 0;
  size_t pos = 0;
  while ((pos = text.find(needle, pos)) != std::string::npos)
  {
    ++count;
    pos += needle.size();
  }
  return count;
}

XmlSignals CollectXmlSignals(const GP<DjVuImage> &image, const GURL &doc_url)
{
  GP<ByteStream> xml = ByteStream::create();
  EXPECT_NO_THROW(image->writeXML(*xml, doc_url));
  const std::string text = ReadAll(xml);
  XmlSignals out;
  out.has_object = text.find("<OBJECT") != std::string::npos;
  out.has_map = text.find("<MAP") != std::string::npos;
  out.has_hidden = text.find("HIDDENTEXT") != std::string::npos;
  out.has_meta = text.find("<METADATA") != std::string::npos;
  out.area_count = CountOccurrences(text, "<AREA");
  out.word_count = CountOccurrences(text, "<WORD");
  return out;
}

}  // namespace

TEST(DjVuImageTest, EmptyImageReturnsDefaults)
{
  GP<DjVuImage> image = DjVuImage::create();
  ASSERT_TRUE(image != 0);

  EXPECT_EQ(0, image->get_width());
  EXPECT_EQ(0, image->get_height());
  EXPECT_EQ(300, image->get_dpi());
  EXPECT_DOUBLE_EQ(2.2, image->get_gamma());
  EXPECT_TRUE(image->get_mimetype().length() == 0);
  EXPECT_FALSE(image->wait_for_complete_decode());
}

TEST(DjVuImageTest, ConnectedImageUsesFileInfoAndRotation)
{
  GP<DjVuFile> file = DjVuFile::create(MakeDjvuWithInfo());
  ASSERT_TRUE(file != 0);

  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 40;
  info->height = 20;
  info->dpi = 400;
  info->gamma = 1.8;
  file->info = info;
  file->mimetype = "image/djvu";
  file->file_size = 4096;
  file->description = "desc";

  GP<DjVuImage> image = DjVuImage::create(file);
  ASSERT_TRUE(image != 0);

  EXPECT_EQ(40, image->get_width());
  EXPECT_EQ(20, image->get_height());
  EXPECT_EQ(40, image->get_real_width());
  EXPECT_EQ(20, image->get_real_height());
  EXPECT_EQ(400, image->get_dpi());
  EXPECT_DOUBLE_EQ(1.8, image->get_gamma());
  EXPECT_STREQ("image/djvu", (const char *)image->get_mimetype());
  EXPECT_EQ(24, image->get_version());

  image->set_rotate(1);
  EXPECT_EQ(20, image->get_width());
  EXPECT_EQ(40, image->get_height());

  EXPECT_TRUE(image->get_short_description().length() > 0);
  EXPECT_STREQ("desc", (const char *)image->get_long_description());
}

TEST(DjVuImageTest, GetDjVuFileReturnsConnectedFile)
{
  GP<DjVuFile> file = DjVuFile::create(MakeDjvuWithInfo());
  ASSERT_TRUE(file != 0);

  GP<DjVuImage> image = DjVuImage::create(file);
  ASSERT_TRUE(image != 0);

  GP<DjVuFile> returned = image->get_djvu_file();
  EXPECT_TRUE(returned == file);
}

TEST(DjVuImageTest, RotateNormalizationAffectsReportedGeometry)
{
  GP<DjVuFile> file = DjVuFile::create(MakeDjvuWithInfo());
  ASSERT_TRUE(file != 0);
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 40;
  info->height = 20;
  info->dpi = 400;
  info->gamma = 1.8;
  file->info = info;

  GP<DjVuImage> image = DjVuImage::create(file);
  ASSERT_TRUE(image != 0);

  image->set_rotate(5);
  EXPECT_EQ(1, image->get_rotate());
  EXPECT_EQ(20, image->get_width());
  EXPECT_EQ(40, image->get_height());

  image->set_rotate(8);
  EXPECT_EQ(0, image->get_rotate());
  EXPECT_EQ(40, image->get_width());
  EXPECT_EQ(20, image->get_height());
}

TEST(DjVuImageTest, RoundedDpiUsesNearestTens)
{
  GP<DjVuFile> file = DjVuFile::create(MakeDjvuWithInfo());
  ASSERT_TRUE(file != 0);
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 40;
  info->height = 20;
  info->dpi = 307;
  info->gamma = 2.2;
  file->info = info;

  GP<DjVuImage> image = DjVuImage::create(file);
  ASSERT_TRUE(image != 0);
  EXPECT_EQ(307, image->get_dpi());
  EXPECT_EQ(310, image->get_rounded_dpi());
}

namespace {
class TestNotifier : public DjVuInterface
{
public:
  int chunks = 0;
  int relayout = 0;
  int redisplay = 0;

  void notify_chunk(const char *, const char *) override { ++chunks; }
  void notify_relayout() override { ++relayout; }
  void notify_redisplay() override { ++redisplay; }
};
}  // namespace

TEST(DjVuImageTest, LegacyDecodeAndSecondDecodeThrows)
{
  GP<DjVuImage> image = DjVuImage::create();
  TestNotifier notifier;
  GP<ByteStream> bs = MakeDjvuWithInfo();
  EXPECT_NO_THROW(image->decode(*bs, &notifier));
  EXPECT_TRUE(image->wait_for_complete_decode());
  EXPECT_GT(image->get_width(), 0);

  bs->seek(0, SEEK_SET);
  EXPECT_THROW(image->decode(*bs, &notifier), GException);
}

TEST(DjVuImageTest, LegalTypeChecksAndXmlMapPaths)
{
  GP<DjVuFile> file = DjVuFile::create(MakeDjvuWithInfo());
  ASSERT_TRUE(file != 0);

  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 40;
  info->height = 20;
  info->dpi = 300;
  info->gamma = 2.2;
  file->info = info;
  file->mimetype = "image/djvu";

  GP<JB2Image> mask = JB2Image::create();
  mask->set_dimension(40, 20);
  file->fgjb = mask;

  GP<DjVuImage> image = DjVuImage::create(file);
  ASSERT_TRUE(image != 0);
  EXPECT_EQ(1, image->is_legal_bilevel());
  EXPECT_EQ(0, image->is_legal_photo());

  file->bgpm = GPixmap::create(20, 40);
  file->fgbc = DjVuPalette::create();
  EXPECT_NO_THROW(EXPECT_EQ(1, image->is_legal_compound()));

  file->fgjb = 0;
  file->fgpm = 0;
  EXPECT_NO_THROW(EXPECT_EQ(1, image->is_legal_photo()));

  GP<ByteStream> anno_bs = ByteStream::create();
  GP<DjVuAnno> anno = DjVuAnno::create();
  anno->ant = DjVuANT::create();
  anno->encode(anno_bs);
  file->anno = anno_bs;

  GP<DjVuText> text = DjVuText::create();
  text->txt = DjVuTXT::create();
  text->txt->textUTF8 = "hello";
  GP<ByteStream> text_bs = ByteStream::create();
  text->encode(text_bs);
  file->text = text_bs;

  GP<ByteStream> meta = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(meta);
  iff->put_chunk("METz");
  {
    GP<ByteStream> z = BSByteStream::create(iff->get_bytestream(), 50);
    z->writestring(GUTF8String("meta"));
  }
  iff->close_chunk();
  file->meta = meta;

  EXPECT_NO_THROW(([&]() {
    image->set_rotate(1);
    GRect rect(1, 1, 10, 5);
    image->map(rect);
    image->unmap(rect);
    int x = 3, y = 2;
    image->map(x, y);
    image->unmap(x, y);
  })());

  GP<ByteStream> xml = ByteStream::create();
  EXPECT_THROW(
    image->writeXML(*xml, GURL(), DjVuImage::NOMAP | DjVuImage::NOTEXT | DjVuImage::NOMETA),
    GException);
}

TEST(DjVuImageTest, ReferenceFixtureSaveAsRoundtripPreservesImageLevelExtraction)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("sp_bg_fgtext_hidden_links.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> src = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(src != 0);
  ASSERT_TRUE(src->is_init_complete());

  const std::filesystem::path bundled_path =
      std::filesystem::temp_directory_path() /
      ("djvu_gtest_image_roundtrip_bundle_" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
       ".djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_path.string().c_str());
  EXPECT_NO_THROW(src->save_as(bundled_url, true));

  GP<DjVuDocument> bundled = DjVuDocument::create_wait(bundled_url);
  ASSERT_TRUE(bundled != 0);
  ASSERT_TRUE(bundled->is_init_complete());
  GP<DjVuImage> image = bundled->get_page(0, true);
  ASSERT_TRUE(image != 0);

  GP<ByteStream> anno = image->get_anno();
  GP<ByteStream> text = image->get_text();
  GP<ByteStream> meta = image->get_meta();
  EXPECT_TRUE(anno != 0);
  EXPECT_TRUE(text != 0);
  EXPECT_TRUE(meta == 0 || meta->size() >= 0);

  GP<ByteStream> xml = ByteStream::create();
  EXPECT_NO_THROW(image->writeXML(*xml, bundled_url));
  const std::string xml_text = ReadAll(xml);
  EXPECT_NE(std::string::npos, xml_text.find("HIDDENTEXT"));
  EXPECT_NE(std::string::npos, xml_text.find("OBJECT"));
}

TEST(DjVuImageTest, ReferenceMultipageRoundtripPreservesPerPageXmlSignals)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("mp3_indirect_mixed_layers/index.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> src = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(src != 0);
  ASSERT_TRUE(src->is_init_complete());
  ASSERT_EQ(3, src->get_pages_num());

  std::vector<XmlSignals> source;
  for (int i = 0; i < src->get_pages_num(); ++i)
  {
    GP<DjVuImage> image = src->get_page(i, true);
    ASSERT_TRUE(image != 0);
    source.push_back(CollectXmlSignals(image, fixture_url));
  }
  ASSERT_EQ(3u, source.size());
  EXPECT_LT(source[0].area_count, source[2].area_count);
  EXPECT_LE(source[0].word_count, source[2].word_count);

  const std::filesystem::path bundled_path =
      std::filesystem::temp_directory_path() /
      ("djvu_gtest_image_multipage_bundle_" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
       ".djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_path.string().c_str());
  EXPECT_NO_THROW(src->save_as(bundled_url, true));

  GP<DjVuDocument> bundled = DjVuDocument::create_wait(bundled_url);
  ASSERT_TRUE(bundled != 0);
  ASSERT_TRUE(bundled->is_init_complete());
  for (int i = 0; i < bundled->get_pages_num(); ++i)
  {
    GP<DjVuImage> image = bundled->get_page(i, true);
    ASSERT_TRUE(image != 0);
    EXPECT_EQ(source[static_cast<size_t>(i)], CollectXmlSignals(image, bundled_url));
  }
}
