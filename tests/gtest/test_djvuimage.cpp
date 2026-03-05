#include <gtest/gtest.h>

#include "BSByteStream.h"
#include "ByteStream.h"
#include "DjVuAnno.h"
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
