#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "ByteStream.h"
#include "DjVuInfo.h"
#include "DjVuPalette.h"
#include "GBitmap.h"
#include "GPixmap.h"
#include "GScaler.h"
#include "GString.h"

namespace
{

GP<ByteStream> MakeStreamFromBytes(const std::vector<unsigned char> &bytes)
{
  if (bytes.empty())
    return ByteStream::create();
  return ByteStream::create(reinterpret_cast<const char *>(&bytes[0]), bytes.size());
}

GP<GBitmap> MakeMaskBitmap(int rows, int cols, int grays)
{
  GP<GBitmap> bm = GBitmap::create(rows, cols, 0);
  bm->set_grays(grays);
  for (int y = 0; y < rows; ++y)
    for (int x = 0; x < cols; ++x)
      (*bm)[y][x] = static_cast<unsigned char>((x + 2 * y) % grays);
  return bm;
}

GP<GPixmap> MakeGradientPixmap(int rows, int cols)
{
  GP<GPixmap> pm = GPixmap::create(rows, cols);
  for (int y = 0; y < rows; ++y)
  {
    for (int x = 0; x < cols; ++x)
    {
      GPixel p;
      p.r = static_cast<unsigned char>((x * 31 + y * 7) & 0xff);
      p.g = static_cast<unsigned char>((x * 11 + y * 23) & 0xff);
      p.b = static_cast<unsigned char>((x * 3 + y * 41) & 0xff);
      (*pm)[y][x] = p;
    }
  }
  return pm;
}

} // namespace

TEST(GBitmapExtendedTest, FormatRoundtripsAndRleHelpers)
{
  GP<GBitmap> src = GBitmap::create(3, 8, 1);
  src->set_grays(2);
  for (int y = 0; y < 3; ++y)
    for (int x = 0; x < 8; ++x)
      (*src)[y][x] = static_cast<unsigned char>((x + y) & 1);

  GP<ByteStream> pbm_text = ByteStream::create();
  src->save_pbm(*pbm_text, 0);
  pbm_text->seek(0, SEEK_SET);
  GP<GBitmap> pbm_text_dec = GBitmap::create(*pbm_text, 0);
  EXPECT_EQ(3u, pbm_text_dec->rows());
  EXPECT_EQ(8u, pbm_text_dec->columns());

  GP<ByteStream> pbm_raw = ByteStream::create();
  src->save_pbm(*pbm_raw, 1);
  pbm_raw->seek(0, SEEK_SET);
  GP<GBitmap> pbm_raw_dec = GBitmap::create(*pbm_raw, 0);
  EXPECT_EQ(3u, pbm_raw_dec->rows());
  EXPECT_EQ(8u, pbm_raw_dec->columns());

  src->set_grays(16);
  GP<ByteStream> pgm_text = ByteStream::create();
  src->save_pgm(*pgm_text, 0);
  pgm_text->seek(0, SEEK_SET);
  GP<GBitmap> pgm_text_dec = GBitmap::create(*pgm_text, 0);
  EXPECT_EQ(3u, pgm_text_dec->rows());
  EXPECT_EQ(8u, pgm_text_dec->columns());

  GP<ByteStream> pgm_raw = ByteStream::create();
  src->save_pgm(*pgm_raw, 1);
  pgm_raw->seek(0, SEEK_SET);
  GP<GBitmap> pgm_raw_dec = GBitmap::create(*pgm_raw, 0);
  EXPECT_EQ(3u, pgm_raw_dec->rows());
  EXPECT_EQ(8u, pgm_raw_dec->columns());

  src->change_grays(2);
  src->compress();
  unsigned int rle_len = 0;
  const unsigned char *rle = src->get_rle(rle_len);
  ASSERT_NE(nullptr, rle);
  ASSERT_GT(rle_len, 0u);

  int runs[64] = {0};
  const int nruns = src->rle_get_runs(1, runs);
  EXPECT_GT(nruns, 0);

  unsigned char bits[64] = {0};
  const int nbits = src->rle_get_bits(1, bits);
  EXPECT_EQ(8, nbits);

  GRect rect;
  EXPECT_GE(src->rle_get_rect(rect), 0);

  GP<ByteStream> r4 = ByteStream::create();
  src->save_rle(*r4);
  r4->seek(0, SEEK_SET);
  GP<GBitmap> r4_dec = GBitmap::create(*r4, 0);
  EXPECT_EQ(3u, r4_dec->rows());
  EXPECT_EQ(8u, r4_dec->columns());
}

TEST(GBitmapExtendedTest, ReadsTextAndRawFormatsIncludingPgm16Bit)
{
  const std::string p1 = "P1\n# test comment\n4 2\n0 1 0 1\n1 0 1 0\n";
  GP<ByteStream> p1s = ByteStream::create(p1.data(), p1.size());
  GP<GBitmap> bm1 = GBitmap::create(*p1s, 0);
  EXPECT_EQ(2u, bm1->rows());
  EXPECT_EQ(4u, bm1->columns());

  std::vector<unsigned char> p5_16 = {
      'P', '5', '\n', '2', ' ', '1', '\n', '3', '0', '0', '\n',
      0x00, 0x00, 0x01, 0x2c};
  GP<ByteStream> p5s = MakeStreamFromBytes(p5_16);
  GP<GBitmap> bm5 = GBitmap::create(*p5s, 0);
  EXPECT_EQ(1u, bm5->rows());
  EXPECT_EQ(2u, bm5->columns());
}

TEST(GBitmapExtendedTest, DonateRleAndBorderRotationPaths)
{
  GP<GBitmap> src = GBitmap::create(4, 4, 0);
  src->set_grays(2);
  (*src)[0][0] = 1;
  (*src)[1][1] = 1;
  (*src)[2][2] = 1;
  (*src)[3][3] = 1;
  src->compress();

  unsigned int rle_len = 0;
  const unsigned char *rle = src->get_rle(rle_len);
  ASSERT_NE(nullptr, rle);
  unsigned char *owned = new unsigned char[rle_len];
  memcpy(owned, rle, rle_len);

  GP<GBitmap> donated = GBitmap::create();
  donated->donate_rle(owned, rle_len, 4, 4);
  donated->minborder(2);
  donated->share();
  EXPECT_EQ(4u, donated->rows());
  EXPECT_EQ(4u, donated->columns());

  GP<GBitmap> rot1 = donated->rotate(1);
  GP<GBitmap> rot3 = donated->rotate(3);
  EXPECT_EQ(4u, rot1->rows());
  EXPECT_EQ(4u, rot3->columns());
}

TEST(GBitmapExtendedTest, ErrorPathsForInvalidInputAndUnsupportedSave)
{
  const std::string bad_p1 = "P1\n2 1\n0 X\n";
  GP<ByteStream> bad_stream = ByteStream::create(bad_p1.data(), bad_p1.size());
  EXPECT_THROW((void)GBitmap::create(*bad_stream, 0), GException);

  GP<GBitmap> gray = GBitmap::create(1, 1, 0);
  gray->set_grays(4);
  GP<ByteStream> out = ByteStream::create();
  EXPECT_THROW(gray->save_pbm(*out, 1), GException);

  GP<GBitmap> empty = GBitmap::create();
  EXPECT_THROW(empty->save_rle(*out), GException);
  EXPECT_THROW(empty->set_grays(1), GException);
  EXPECT_THROW(empty->set_grays(300), GException);
}

TEST(GBitmapExtendedTest, BlitSubsampleAndDataOwnershipPaths)
{
  GP<GBitmap> dst = GBitmap::create(6, 6, 0);
  dst->set_grays(16);
  dst->fill(0);

  GP<GBitmap> src = GBitmap::create(4, 4, 0);
  src->set_grays(16);
  for (int y = 0; y < 4; ++y)
    for (int x = 0; x < 4; ++x)
      (*src)[y][x] = static_cast<unsigned char>((x + y) & 1);

  dst->blit(src, 1, 1);
  dst->blit(src, -20, -20);
  dst->blit(src, 2, 2, 2);

  src->change_grays(2);
  src->compress();
  dst->change_grays(2);
  dst->blit(src, 0, 0);
  dst->blit(src, 3, 3, 2);

  unsigned char *raw = new unsigned char[6];
  memset(raw, 0, 6);
  raw[0] = 1;
  raw[5] = 1;
  GP<GBitmap> owned = GBitmap::create();
  owned->donate_data(raw, 3, 2);
  EXPECT_EQ(2u, owned->rows());
  EXPECT_EQ(3u, owned->columns());

  size_t off = 1234;
  unsigned char *taken = owned->take_data(off);
  ASSERT_NE(nullptr, taken);
  EXPECT_EQ(0u, off);
  delete[] taken;

  EXPECT_GT(dst->get_memory_usage(), 0u);
}

TEST(GBitmapExtendedTest, LongRunRleAndMalformedRlePaths)
{
  const int kCols = 20000;
  GP<GBitmap> long_run = GBitmap::create(1, kCols, 0);
  long_run->set_grays(2);
  for (int x = 0; x < kCols; ++x)
    (*long_run)[0][x] = 1;

  GP<ByteStream> rle = ByteStream::create();
  long_run->save_rle(*rle);
  rle->seek(0, SEEK_SET);
  GP<GBitmap> decoded = GBitmap::create(*rle, 0);
  EXPECT_EQ(1u, decoded->rows());
  EXPECT_EQ(static_cast<unsigned int>(kCols), decoded->columns());

  std::vector<unsigned char> bad_rle = {'R', '4', '\n', '4', ' ', '1', '\n', 0x05};
  GP<ByteStream> bad_stream = MakeStreamFromBytes(bad_rle);
  EXPECT_THROW((void)GBitmap::create(*bad_stream, 0), GException);
}

TEST(GPixmapExtendedTest, InitAndResamplePaths)
{
  GP<GPixmap> src = MakeGradientPixmap(6, 7);
  GP<GPixmap> copy = GPixmap::create(*src);
  EXPECT_EQ(6u, copy->rows());
  EXPECT_EQ(7u, copy->columns());

  GRect rect(1, 1, 5, 4);
  GP<GPixmap> sub = GPixmap::create(*src, rect);
  EXPECT_EQ(static_cast<unsigned int>(rect.height()), sub->rows());
  EXPECT_EQ(static_cast<unsigned int>(rect.width()), sub->columns());

  GP<GBitmap> mask = MakeMaskBitmap(6, 7, 8);
  GP<GPixmap> from_mask = GPixmap::create(*mask);
  EXPECT_EQ(6u, from_mask->rows());
  EXPECT_EQ(7u, from_mask->columns());
  GP<GPixmap> from_mask_sub = GPixmap::create(*mask, rect);
  EXPECT_EQ(static_cast<unsigned int>(rect.height()), from_mask_sub->rows());

  GP<ByteStream> ppm_text = ByteStream::create();
  src->save_ppm(*ppm_text, 0);
  ppm_text->seek(0, SEEK_SET);
  GP<GPixmap> dec_text = GPixmap::create(*ppm_text);
  EXPECT_EQ(6u, dec_text->rows());
  EXPECT_EQ(7u, dec_text->columns());

  GP<ByteStream> ppm_raw = ByteStream::create();
  src->save_ppm(*ppm_raw, 1);
  ppm_raw->seek(0, SEEK_SET);
  GP<GPixmap> dec_raw = GPixmap::create(*ppm_raw);
  EXPECT_EQ(6u, dec_raw->rows());
  EXPECT_EQ(7u, dec_raw->columns());

  GP<GPixmap> down = GPixmap::create();
  down->downsample(src, 2, 0);
  EXPECT_EQ(3u, down->rows());
  EXPECT_EQ(4u, down->columns());

  GP<GPixmap> up = GPixmap::create();
  up->upsample(down, 2, 0);
  EXPECT_EQ(6u, up->rows());

  GP<GPixmap> d43 = GPixmap::create();
  d43->downsample43(src, 0);
  EXPECT_GT(d43->rows(), 0u);

  GP<GPixmap> u23 = GPixmap::create();
  u23->upsample23(d43, 0);
  EXPECT_GT(u23->columns(), 0u);

  GP<GPixmap> rot0 = src->rotate(0);
  GP<GPixmap> rot1 = src->rotate(1);
  GP<GPixmap> rot2 = src->rotate(2);
  GP<GPixmap> rot3 = src->rotate(3);
  EXPECT_EQ(6u, rot0->rows());
  EXPECT_EQ(7u, rot2->columns());
  EXPECT_EQ(7u, rot1->rows());
  EXPECT_EQ(7u, rot3->rows());
}

TEST(GPixmapExtendedTest, ColorCorrectionDitherAndCompositingPaths)
{
  GP<GPixmap> dst = MakeGradientPixmap(4, 4);
  GP<GBitmap> alpha = MakeMaskBitmap(4, 4, 4);
  GP<GPixmap> fg = GPixmap::create(4, 4, &GPixel::RED);

  dst->color_correct(1.3);
  dst->color_correct(0.9, GPixel::GREEN);

  GPixel px[3] = {GPixel::RED, GPixel::GREEN, GPixel::BLUE};
  GPixmap::color_correct(1.2, px, 3);
  GPixmap::color_correct(1.1, GPixel::WHITE, px, 3);

  dst->ordered_666_dither(1, 2);
  dst->ordered_32k_dither(3, 4);

  dst->attenuate(alpha, 0, 0);
  dst->blit(alpha, 0, 0, &GPixel::BLUE);
  dst->blit(alpha, 0, 0, fg);
  dst->blend(alpha, 0, 0, fg);

  GP<GPixmap> tiny = GPixmap::create(2, 2, &GPixel::GREEN);
  GP<GBitmap> mask = MakeMaskBitmap(4, 4, 4);
  dst->stencil(mask, tiny, 2, 0, 1.25, GPixel::WHITE);
  dst->stencil(mask, tiny, 2, 0, 0.85);

  EXPECT_EQ(4u, dst->rows());
  EXPECT_EQ(4u, dst->columns());
}

TEST(GPixmapExtendedTest, ErrorPathsForInvalidArguments)
{
  GP<GPixmap> dst = MakeGradientPixmap(3, 3);
  GP<GBitmap> alpha = MakeMaskBitmap(3, 3, 4);
  GP<GPixmap> wrong = GPixmap::create(2, 2, &GPixel::RED);

  EXPECT_THROW(dst->blit(0, 0, 0, &GPixel::RED), GException);
  EXPECT_THROW(dst->blit(alpha, 0, 0, static_cast<const GPixmap *>(0)), GException);
  EXPECT_THROW(dst->blend(alpha, 0, 0, wrong), GException);

  GRect bad_down(0, 0, 100, 100);
  EXPECT_THROW(dst->downsample(dst, 2, &bad_down), GException);
  EXPECT_THROW(dst->upsample(dst, 2, &bad_down), GException);
  EXPECT_THROW(dst->downsample43(dst, &bad_down), GException);
  EXPECT_THROW(dst->upsample23(dst, &bad_down), GException);
}

TEST(GPixmapExtendedTest, DataOwnershipAndPpmErrorPaths)
{
  GPixel *raw = new GPixel[6];
  for (int i = 0; i < 6; ++i)
    raw[i] = GPixel::BLACK;
  raw[0] = GPixel::RED;
  raw[5] = GPixel::BLUE;

  GP<GPixmap> pm = GPixmap::create();
  pm->donate_data(raw, 3, 2);
  EXPECT_EQ(2u, pm->rows());
  EXPECT_EQ(3u, pm->columns());
  EXPECT_EQ(GPixel::RED.r, (*pm)[0][0].r);

  size_t off = 99;
  GPixel *taken = pm->take_data(off);
  ASSERT_NE(nullptr, taken);
  EXPECT_EQ(0u, off);
  delete[] taken;

  std::vector<unsigned char> bad_magic = {'P', '9', '\n', '1', ' ', '1', '\n',
                                          '2', '5', '5', '\n', 0x00, 0x00, 0x00};
  GP<ByteStream> bads = MakeStreamFromBytes(bad_magic);
  EXPECT_THROW((void)GPixmap::create(*bads), GException);

  const std::string too_deep = "P3\n1 1\n70000\n0 0 0\n";
  GP<ByteStream> deep = ByteStream::create(too_deep.data(), too_deep.size());
  EXPECT_THROW((void)GPixmap::create(*deep), GException);
}

TEST(GPixmapExtendedTest, Raw16BitAndTruncatedInputPaths)
{
  std::vector<unsigned char> p6_16 = {
      'P', '6', '\n', '1', ' ', '1', '\n', '3', '0', '0', '\n',
      0x00, 0x64, 0x00, 0x96, 0x01, 0x2c};
  GP<ByteStream> s_p6_16 = MakeStreamFromBytes(p6_16);
  GP<GPixmap> pm16 = GPixmap::create(*s_p6_16);
  EXPECT_EQ(1u, pm16->rows());
  EXPECT_EQ(1u, pm16->columns());

  std::vector<unsigned char> p5_16 = {
      'P', '5', '\n', '2', ' ', '1', '\n', '3', '0', '0', '\n',
      0x00, 0x00, 0x01, 0x2c};
  GP<ByteStream> s_p5_16 = MakeStreamFromBytes(p5_16);
  GP<GPixmap> gray16 = GPixmap::create(*s_p5_16);
  EXPECT_EQ(1u, gray16->rows());
  EXPECT_EQ(2u, gray16->columns());

  const std::vector<unsigned char> trunc_rgb = {
      'P', '6', '\n', '2', ' ', '1', '\n', '2', '5', '5', '\n', 0x01, 0x02, 0x03};
  GP<ByteStream> s_trunc_rgb = MakeStreamFromBytes(trunc_rgb);
  EXPECT_THROW((void)GPixmap::create(*s_trunc_rgb), GException);

  const std::vector<unsigned char> trunc_gray = {
      'P', '5', '\n', '2', ' ', '1', '\n', '2', '5', '5', '\n', 0x7f};
  GP<ByteStream> s_trunc_gray = MakeStreamFromBytes(trunc_gray);
  EXPECT_THROW((void)GPixmap::create(*s_trunc_gray), GException);
}

TEST(GPixmapExtendedTest, PbmInputDelegationPaths)
{
  const std::string p1 = "P1\n2 2\n0 1\n1 0\n";
  GP<ByteStream> s_p1 = ByteStream::create(p1.data(), p1.size());
  GP<GPixmap> from_p1 = GPixmap::create(*s_p1);
  EXPECT_EQ(2u, from_p1->rows());
  EXPECT_EQ(2u, from_p1->columns());

  std::vector<unsigned char> p4 = {'P', '4', '\n', '2', ' ', '2', '\n', 0x40, 0x80};
  GP<ByteStream> s_p4 = MakeStreamFromBytes(p4);
  GP<GPixmap> from_p4 = GPixmap::create(*s_p4);
  EXPECT_EQ(2u, from_p4->rows());
  EXPECT_EQ(2u, from_p4->columns());
}

TEST(GScalerExtendedTest, RatioValidationAndScalingSuccessPaths)
{
  GP<GBitmapScaler> bsc = GBitmapScaler::create();
  EXPECT_THROW(bsc->set_horz_ratio(1, 1), GException);
  EXPECT_THROW(bsc->set_vert_ratio(1, 1), GException);

  bsc->set_input_size(5, 4);
  bsc->set_output_size(9, 7);
  EXPECT_THROW(bsc->set_horz_ratio(1, 0), GException);
  EXPECT_THROW(bsc->set_vert_ratio(0, 1), GException);
  bsc->set_horz_ratio(3, 2);
  bsc->set_vert_ratio(2, 1);

  GRect desired(0, 0, 9, 7);
  GRect need;
  bsc->get_input_rect(desired, need);
  EXPECT_GT(need.width(), 0);
  EXPECT_GT(need.height(), 0);
  GRect too_big(0, 0, 10, 8);
  EXPECT_THROW(bsc->get_input_rect(too_big, need), GException);

  GP<GBitmap> in_bm = MakeMaskBitmap(4, 5, 64);
  GP<GBitmap> out_bm = GBitmap::create();
  GRect provided(0, 0, 5, 4);
  GRect out_rect(0, 0, 9, 7);
  bsc->set_input_size(5, 4);
  bsc->set_output_size(9, 7);
  EXPECT_NO_THROW(bsc->scale(provided, *in_bm, out_rect, *out_bm));
  EXPECT_EQ(7u, out_bm->rows());

  GP<GPixmapScaler> psc = GPixmapScaler::create(5, 4, 9, 7);
  GP<GPixmap> in_pm = MakeGradientPixmap(4, 5);
  GP<GPixmap> out_pm = GPixmap::create();
  EXPECT_NO_THROW(psc->scale(provided, *in_pm, out_rect, *out_pm));
  EXPECT_EQ(7u, out_pm->rows());
}

TEST(GScalerExtendedTest, SlowPathNoMatchAndTooSmallErrors)
{
  GP<GBitmap> in_bm = MakeMaskBitmap(8, 8, 32);
  GP<GBitmapScaler> bsc = GBitmapScaler::create(8, 8, 2, 2);
  GRect provided(0, 0, 8, 8);
  GRect desired(0, 0, 2, 2);
  GP<GBitmap> out_bm = GBitmap::create();
  EXPECT_NO_THROW(bsc->scale(provided, *in_bm, desired, *out_bm));
  EXPECT_EQ(2u, out_bm->rows());
  EXPECT_EQ(2u, out_bm->columns());

  GRect wrong_shape(0, 0, 7, 8);
  EXPECT_THROW(bsc->scale(wrong_shape, *in_bm, desired, *out_bm), GException);

  GRect too_small(1, 1, 7, 7);
  EXPECT_THROW(bsc->scale(too_small, *in_bm, desired, *out_bm), GException);

  GP<GPixmap> in_pm = MakeGradientPixmap(8, 8);
  GP<GPixmapScaler> psc = GPixmapScaler::create(8, 8, 2, 2);
  GP<GPixmap> out_pm = GPixmap::create();
  EXPECT_NO_THROW(psc->scale(provided, *in_pm, desired, *out_pm));
  EXPECT_EQ(2u, out_pm->rows());
  EXPECT_EQ(2u, out_pm->columns());

  EXPECT_THROW(psc->scale(wrong_shape, *in_pm, desired, *out_pm), GException);
  EXPECT_THROW(psc->scale(too_small, *in_pm, desired, *out_pm), GException);
}

TEST(DjVuPaletteExtendedTest, ComputeQuantizeCopyAndColorCorrectPaths)
{
  GP<GPixmap> pm = MakeGradientPixmap(8, 8);
  GP<DjVuPalette> pal = DjVuPalette::create();
  pal->compute_pixmap_palette(*pm, 32, 1);
  EXPECT_GT(pal->size(), 0);

  pal->quantize(*pm);

  GP<GPixmap> pm2 = MakeGradientPixmap(6, 6);
  pal->compute_palette_and_quantize(*pm2, 16, 1);
  pal->color_correct(1.15);
  pal->colordata.resize(0, 2);
  pal->colordata[0] = 0;
  pal->colordata[1] = 1;
  pal->colordata[2] = 0;

  DjVuPalette copied(*pal);
  DjVuPalette assigned(copied);
  assigned = copied;
  EXPECT_EQ(copied.size(), assigned.size());

  GP<ByteStream> bs = ByteStream::create();
  pal->encode(bs);
  bs->seek(0, SEEK_SET);
  GP<DjVuPalette> decoded = DjVuPalette::create();
  decoded->decode(bs);
  EXPECT_EQ(pal->size(), decoded->size());
}

TEST(DjVuPaletteExtendedTest, EntryEncodingAndDecodeValidationPaths)
{
  GP<DjVuPalette> pal = DjVuPalette::create();
  GP<GPixmap> pm = MakeGradientPixmap(4, 4);
  pal->compute_pixmap_palette(*pm, 8, 0);

  GP<ByteStream> rgb = ByteStream::create();
  pal->encode_rgb_entries(*rgb);
  rgb->seek(0, SEEK_SET);
  GP<DjVuPalette> rgb_dec = DjVuPalette::create();
  rgb_dec->decode_rgb_entries(*rgb, pal->size());
  EXPECT_EQ(pal->size(), rgb_dec->size());

  std::vector<unsigned char> bad_version = {0x7f, 0x00, 0x00};
  GP<ByteStream> s_bad_ver = MakeStreamFromBytes(bad_version);
  GP<DjVuPalette> p_bad_ver = DjVuPalette::create();
  EXPECT_THROW(p_bad_ver->decode(s_bad_ver), GException);

  std::vector<unsigned char> bad_palette = {0x80, 0x00, 0x01, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x01, 0x00, 0x01};
  GP<ByteStream> s_bad_pal = MakeStreamFromBytes(bad_palette);
  GP<DjVuPalette> p_bad_pal = DjVuPalette::create();
  EXPECT_THROW(p_bad_pal->decode(s_bad_pal), GException);
}

TEST(DjVuInfoExtendedTest, ParamOutputAndMemoryUsagePaths)
{
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 123;
  info->height = 77;
  info->version = 25;
  info->dpi = 400;
  info->gamma = 1.8;
  info->orientation = 2;

  EXPECT_GT(info->get_memory_usage(), 0u);
  EXPECT_GT(info->get_paramtags().length(), 0u);

  GP<ByteStream> ps = ByteStream::create();
  info->writeParam(*ps);
  ps->seek(0, SEEK_END);
  EXPECT_GT(ps->tell(), 0);

  GP<ByteStream> encoded = ByteStream::create();
  info->encode(*encoded);
  encoded->seek(0, SEEK_SET);
  GP<DjVuInfo> round = DjVuInfo::create();
  round->decode(*encoded);
  EXPECT_EQ(123, round->width);
  EXPECT_EQ(77, round->height);
}
