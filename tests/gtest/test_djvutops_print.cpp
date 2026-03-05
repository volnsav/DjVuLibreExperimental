#include <gtest/gtest.h>

#include <string>

#include "ByteStream.h"
#include "DjVmDoc.h"
#include "DjVuDocument.h"
#include "DjVuFile.h"
#include "DjVuImage.h"
#include "DjVuInfo.h"
#include "DjVuPalette.h"
#include "DjVuToPS.h"
#include "GBitmap.h"
#include "IFFByteStream.h"
#include "JB2Image.h"

namespace {

GP<ByteStream> MakeSinglePageDjvu(int w = 80, int h = 60)
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVU");

  iff->put_chunk("INFO");
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = w;
  info->height = h;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  info->encode(*iff->get_bytestream());
  iff->close_chunk();

  iff->close_chunk();
  bs->seek(0, SEEK_SET);
  return bs;
}

GP<ByteStream> MakeTwoPageBundledDjvu()
{
  GP<DjVmDoc> doc = DjVmDoc::create();
  GP<ByteStream> p1 = MakeSinglePageDjvu(80, 60);
  GP<ByteStream> p2 = MakeSinglePageDjvu(120, 90);
  doc->insert_file(*p1, DjVmDir::File::PAGE, "p1.djvu", "p1.djvu", "Page 1");
  doc->insert_file(*p2, DjVmDir::File::PAGE, "p2.djvu", "p2.djvu", "Page 2");
  GP<ByteStream> out = ByteStream::create();
  doc->write(out);
  out->seek(0, SEEK_SET);
  return out;
}

GP<ByteStream> MakeFourPageBundledDjvu()
{
  GP<DjVmDoc> doc = DjVmDoc::create();
  for (int i = 0; i < 4; ++i)
  {
    const int w = 80 + i * 20;
    const int h = 60 + i * 15;
    GP<ByteStream> page = MakeSinglePageDjvu(w, h);
    const GUTF8String id = GUTF8String("p") + GUTF8String(i + 1) + ".djvu";
    const GUTF8String title = GUTF8String("Page ") + GUTF8String(i + 1);
    doc->insert_file(*page, DjVmDir::File::PAGE, id, id, title);
  }
  GP<ByteStream> out = ByteStream::create();
  doc->write(out);
  out->seek(0, SEEK_SET);
  return out;
}

GP<DjVuImage> MakeSyntheticCompoundImage()
{
  const int width = 64;
  const int height = 48;

  GP<DjVuFile> file = DjVuFile::create(MakeSinglePageDjvu(width, height));
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = width;
  info->height = height;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  file->info = info;

  file->bgpm = GPixmap::create(height, width);
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      GPixel p;
      p.r = static_cast<unsigned char>((x * 7 + y * 3) & 0xff);
      p.g = static_cast<unsigned char>((x * 5 + y * 9) & 0xff);
      p.b = static_cast<unsigned char>((x * 2 + y * 11) & 0xff);
      (*file->bgpm)[y][x] = p;
    }
  }

  file->fgpm = GPixmap::create(height, width);
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      GPixel p;
      p.r = static_cast<unsigned char>((x * 13 + y) & 0xff);
      p.g = static_cast<unsigned char>((x + y * 17) & 0xff);
      p.b = static_cast<unsigned char>((x * 3 + y * 19) & 0xff);
      (*file->fgpm)[y][x] = p;
    }
  }

  GP<JB2Image> jb2 = JB2Image::create();
  jb2->set_dimension(width, height);
  JB2Shape shape;
  shape.parent = -1;
  shape.userdata = 0;
  shape.bits = GBitmap::create(6, 6, 0);
  shape.bits->set_grays(2);
  for (int y = 0; y < 6; ++y)
    for (int x = 0; x < 6; ++x)
      (*shape.bits)[y][x] = static_cast<unsigned char>((x == y) || (x + y == 5));
  const int shape_no = jb2->add_shape(shape);

  JB2Blit b1;
  b1.left = 8;
  b1.bottom = 8;
  b1.shapeno = static_cast<unsigned int>(shape_no);
  jb2->add_blit(b1);

  JB2Blit b2;
  b2.left = 24;
  b2.bottom = 20;
  b2.shapeno = static_cast<unsigned int>(shape_no);
  jb2->add_blit(b2);
  file->fgjb = jb2;

  GP<DjVuPalette> pal = DjVuPalette::create();
  pal->histogram_add(GPixel::RED, 1);
  pal->histogram_add(GPixel::GREEN, 1);
  pal->histogram_add(GPixel::BLUE, 1);
  pal->compute_palette(3);
  pal->colordata.resize(0, jb2->get_blit_count() - 1);
  pal->colordata[0] = static_cast<short>(pal->color_to_index(GPixel::RED));
  pal->colordata[1] = static_cast<short>(pal->color_to_index(GPixel::BLUE));
  file->fgbc = pal;

  return DjVuImage::create(file);
}

std::string ReadAll(GP<ByteStream> bs)
{
  bs->seek(0, SEEK_SET);
  std::string out;
  char chunk[512];
  while (true)
  {
    const size_t got = bs->read(chunk, sizeof(chunk));
    if (!got)
      break;
    out.append(chunk, got);
  }
  return out;
}

struct PrintCounters
{
  int refresh = 0;
  int info = 0;
  int prn = 0;
  int dec = 0;
};

void RefreshCb(void *data)
{
  PrintCounters *c = static_cast<PrintCounters *>(data);
  ++c->refresh;
}

void PrnCb(double, void *data)
{
  PrintCounters *c = static_cast<PrintCounters *>(data);
  ++c->prn;
}

void DecCb(double, void *data)
{
  PrintCounters *c = static_cast<PrintCounters *>(data);
  ++c->dec;
}

void InfoCb(int, int, int, DjVuToPS::Stage, void *data)
{
  PrintCounters *c = static_cast<PrintCounters *>(data);
  ++c->info;
}

}  // namespace

TEST(DjVuToPSPrintTest, PrintImageProducesPostScriptMarkers)
{
  GP<DjVuFile> file = DjVuFile::create(MakeSinglePageDjvu(40, 20));
  ASSERT_TRUE(file != 0);

  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 40;
  info->height = 20;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  file->info = info;

  GP<DjVuImage> image = DjVuImage::create(file);
  ASSERT_TRUE(image != 0);

  DjVuToPS printer;
  GP<ByteStream> out = ByteStream::create();

  const GRect img_rect(0, 0, image->get_width(), image->get_height());
  printer.print(*out, image, img_rect, img_rect);

  const std::string ps = ReadAll(out);
  EXPECT_NE(std::string::npos, ps.find("%!PS-Adobe-3.0"));
  EXPECT_NE(std::string::npos, ps.find("%%Pages: 1"));
  EXPECT_NE(std::string::npos, ps.find("%%Page: 1 1"));
  EXPECT_NE(std::string::npos, ps.find("showpage"));
}

TEST(DjVuToPSPrintTest, PrintDocumentSinglePageRangeProducesOutput)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());

  DjVuToPS printer;
  GP<ByteStream> out = ByteStream::create();

  printer.print(*out, doc, "1");

  const std::string ps = ReadAll(out);
  EXPECT_NE(std::string::npos, ps.find("%!PS-Adobe-3.0"));
  EXPECT_NE(std::string::npos, ps.find("%%Pages: 1"));
  EXPECT_NE(std::string::npos, ps.find("%%Page: 1 1"));
}

TEST(DjVuToPSPrintTest, PrintDocumentBadRangeThrows)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());

  DjVuToPS printer;
  GP<ByteStream> out = ByteStream::create();

  EXPECT_THROW(printer.print(*out, doc, "bad-range"), GException);
}

TEST(DjVuToPSPrintTest, PrintDocumentTwoPagesWithCallbacksAndRange)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeTwoPageBundledDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());
  ASSERT_EQ(2, doc->get_pages_num());

  DjVuToPS printer;
  printer.options.set_level(2);
  printer.options.set_mode(DjVuToPS::Options::COLOR);
  printer.options.set_color(true);
  printer.options.set_text(false);

  PrintCounters counters;
  printer.set_refresh_cb(&RefreshCb, &counters);
  printer.set_prn_progress_cb(&PrnCb, &counters);
  printer.set_dec_progress_cb(&DecCb, &counters);
  printer.set_info_cb(&InfoCb, &counters);

  GP<ByteStream> out = ByteStream::create();
  printer.print(*out, doc, "1,2");

  const std::string ps = ReadAll(out);
  EXPECT_NE(std::string::npos, ps.find("%!PS-Adobe-3.0"));
  EXPECT_NE(std::string::npos, ps.find("%%Pages: 2"));
  EXPECT_NE(std::string::npos, ps.find("%%Page: 1 1"));
  EXPECT_NE(std::string::npos, ps.find("%%Page: 2 2"));
  EXPECT_GT(counters.prn, 0);
  EXPECT_GT(counters.info, 0);
}

TEST(DjVuToPSPrintTest, PrintEpsLevel1GrayWithFrameAndCropmarks)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu(100, 100));
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());

  DjVuToPS printer;
  printer.options.set_format(DjVuToPS::Options::EPS);
  printer.options.set_level(1);
  printer.options.set_color(false);
  printer.options.set_mode(DjVuToPS::Options::BW);
  printer.options.set_orientation(DjVuToPS::Options::PORTRAIT);
  printer.options.set_zoom(0);
  printer.options.set_frame(true);
  printer.options.set_cropmarks(true);
  printer.options.set_text(false);
  printer.options.set_sRGB(false);
  printer.options.set_gamma(2.4);

  GP<ByteStream> out = ByteStream::create();
  printer.print(*out, doc, "1");
  const std::string ps = ReadAll(out);
  EXPECT_NE(std::string::npos, ps.find("%!PS-Adobe-3.0"));
  EXPECT_NE(std::string::npos, ps.find("%%BoundingBox:"));
  EXPECT_FALSE(ps.empty());
}

TEST(DjVuToPSPrintTest, PrintSyntheticCompoundImageUsesForegroundAndBackgroundLayers)
{
  GP<DjVuImage> image = MakeSyntheticCompoundImage();
  ASSERT_TRUE(image != 0);
  ASSERT_EQ(64, image->get_width());
  ASSERT_EQ(48, image->get_height());

  DjVuToPS printer;
  printer.options.set_level(2);
  printer.options.set_color(true);
  printer.options.set_mode(DjVuToPS::Options::COLOR);

  GP<ByteStream> out = ByteStream::create();
  const GRect rect(0, 0, image->get_width(), image->get_height());
  printer.print(*out, image, rect, rect);
  const std::string ps = ReadAll(out);
  EXPECT_NE(std::string::npos, ps.find("%!PS-Adobe-3.0"));
  EXPECT_NE(std::string::npos, ps.find("%%Pages: 1"));
  EXPECT_NE(std::string::npos, ps.find("showpage"));
}

TEST(DjVuToPSPrintTest, PrintSingleImageLevel1ColorRequirementsPath)
{
  GP<DjVuImage> image = MakeSyntheticCompoundImage();
  ASSERT_TRUE(image != 0);

  DjVuToPS printer;
  printer.options.set_level(1);
  printer.options.set_color(true);
  printer.options.set_mode(DjVuToPS::Options::BW);
  printer.options.set_copies(2);

  GP<ByteStream> out = ByteStream::create();
  const GRect rect(0, 0, image->get_width(), image->get_height());
  printer.print(*out, image, rect, rect);
  const std::string ps = ReadAll(out);
  EXPECT_NE(std::string::npos, ps.find("%%Extensions: CMYK"));
  EXPECT_NE(std::string::npos, ps.find("numcopies(2)"));
}

TEST(DjVuToPSPrintTest, PrintBookletRectoVersoWithDollarRangeAndCropmarks)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeFourPageBundledDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());
  ASSERT_EQ(4, doc->get_pages_num());

  DjVuToPS printer;
  printer.options.set_bookletmode(DjVuToPS::Options::RECTOVERSO);
  printer.options.set_bookletmax(4);
  printer.options.set_bookletalign(10);
  printer.options.set_bookletfold(16, 300);
  printer.options.set_cropmarks(true);
  printer.options.set_copies(2);

  GP<ByteStream> out = ByteStream::create();
  printer.print(*out, doc, "$-1");
  const std::string ps = ReadAll(out);
  EXPECT_NE(std::string::npos, ps.find("duplex(tumble)"));
  EXPECT_NE(std::string::npos, ps.find("%%Page: ("));
  EXPECT_NE(std::string::npos, ps.find("fold-dict"));
}

TEST(DjVuToPSPrintTest, PrintBookletRectoAndVersoModesWork)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeFourPageBundledDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());

  DjVuToPS recto;
  recto.options.set_bookletmode(DjVuToPS::Options::RECTO);
  GP<ByteStream> recto_out = ByteStream::create();
  recto.print(*recto_out, doc, "1-4");
  const std::string recto_ps = ReadAll(recto_out);
  EXPECT_NE(std::string::npos, recto_ps.find("%%Pages: 1"));

  DjVuToPS verso;
  verso.options.set_bookletmode(DjVuToPS::Options::VERSO);
  GP<ByteStream> verso_out = ByteStream::create();
  verso.print(*verso_out, doc, "1-4");
  const std::string verso_ps = ReadAll(verso_out);
  EXPECT_NE(std::string::npos, verso_ps.find("%%Pages: 1"));
}
