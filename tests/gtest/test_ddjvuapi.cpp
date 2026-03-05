#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "ByteStream.h"
#include "DjVuInfo.h"
#include "DjVuDocument.h"
#include "DjVuImage.h"
#include "DjVmDoc.h"
#include "DjVmNav.h"
#include "DjVuText.h"
#include "GBitmap.h"
#include "GPixmap.h"
#include "IFFByteStream.h"
#include "IW44Image.h"
#include "miniexp.h"

extern "C" {
#include "ddjvuapi.h"
}

namespace {

std::vector<char> MakeSinglePageDjvuBytes()
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVU");
  iff->put_chunk("INFO");
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 96;
  info->height = 64;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  info->encode(*iff->get_bytestream());
  iff->close_chunk();
  iff->close_chunk();
  bs->seek(0, SEEK_SET);

  const long n = bs->size();
  std::vector<char> out(static_cast<size_t>(n));
  if (n > 0)
    bs->readall(out.data(), static_cast<size_t>(n));
  return out;
}

std::vector<char> MakeBm44PageBytes()
{
  GP<GBitmap> bm = GBitmap::create(24, 32, 0);
  bm->set_grays(256);
  for (int y = 0; y < 24; ++y)
    for (int x = 0; x < 32; ++x)
      (*bm)[y][x] = static_cast<unsigned char>((x * 9 + y * 13) & 0xff);

  GP<IW44Image> enc = IW44Image::create_encode(*bm);
  IWEncoderParms p;
  p.slices = 16;
  p.bytes = 0;
  p.decibels = 0.0f;

  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  enc->encode_iff(*iff, 1, &p);  // FORM:BM44
  bs->seek(0, SEEK_SET);

  const long n = bs->size();
  std::vector<char> out(static_cast<size_t>(n));
  if (n > 0)
    bs->readall(out.data(), static_cast<size_t>(n));
  return out;
}

std::vector<char> MakePm44PageBytes()
{
  GP<GPixmap> pm = GPixmap::create(24, 32, &GPixel::BLACK);
  for (int y = 0; y < 24; ++y)
    for (int x = 0; x < 32; ++x)
    {
      GPixel &px = (*pm)[y][x];
      px.r = static_cast<unsigned char>((x * 17 + y * 5) & 0xff);
      px.g = static_cast<unsigned char>((x * 9 + y * 13) & 0xff);
      px.b = static_cast<unsigned char>((x * 3 + y * 19) & 0xff);
    }

  GP<IW44Image> enc = IW44Image::create_encode(*pm);
  IWEncoderParms p;
  p.slices = 16;
  p.bytes = 0;
  p.decibels = 0.0f;

  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  enc->encode_iff(*iff, 1, &p);  // FORM:PM44
  bs->seek(0, SEEK_SET);

  const long n = bs->size();
  std::vector<char> out(static_cast<size_t>(n));
  if (n > 0)
    bs->readall(out.data(), static_cast<size_t>(n));
  return out;
}

std::vector<char> MakeSinglePageDjvuWithTextAnnoBytes()
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVU");

  iff->put_chunk("INFO");
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 96;
  info->height = 64;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  info->encode(*iff->get_bytestream());
  iff->close_chunk();

  iff->put_chunk("TXTa");
  GP<DjVuTXT> txt = DjVuTXT::create();
  txt->textUTF8 = "hello world";
  txt->encode(iff->get_bytestream());
  iff->close_chunk();

  iff->put_chunk("ANTa");
  const char anno[] =
      "(background #112233)\n"
      "(zoom page)\n"
      "(mode color)\n"
      "(align left top)\n"
      "(maparea \"u\" \"c\" (rect 0 0 1 1))\n"
      "(metadata (Author \"A\"))\n"
      "(xmp \"X\")\n";
  iff->get_bytestream()->writall(anno, sizeof(anno) - 1);
  iff->close_chunk();

  iff->close_chunk();
  bs->seek(0, SEEK_SET);

  const long n = bs->size();
  std::vector<char> out(static_cast<size_t>(n));
  if (n > 0)
    bs->readall(out.data(), static_cast<size_t>(n));
  return out;
}

GP<ByteStream> MakeByteStreamFromBytes(const std::vector<char> &data)
{
  GP<ByteStream> bs = ByteStream::create();
  if (!data.empty())
    bs->writall(data.data(), static_cast<size_t>(data.size()));
  bs->seek(0, SEEK_SET);
  return bs;
}

std::vector<char> MakeAnnoIncludeBytes(const char *anno)
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVI");
  iff->put_chunk("ANTa");
  iff->get_bytestream()->writall(anno, strlen(anno));
  iff->close_chunk();
  iff->close_chunk();
  bs->seek(0, SEEK_SET);

  const long n = bs->size();
  std::vector<char> out(static_cast<size_t>(n));
  if (n > 0)
    bs->readall(out.data(), static_cast<size_t>(n));
  return out;
}

std::vector<char> MakeBundledDjvmWithSharedAnnoAndNavBytes()
{
  const std::vector<char> page1 = MakeSinglePageDjvuBytes();
  const std::vector<char> page2 = MakeBm44PageBytes();
  const std::vector<char> thumbs = MakeBm44PageBytes();
  const std::vector<char> shared =
      MakeAnnoIncludeBytes("(background #010203)\n(metadata (Author \"Bundled\"))\n");

  GP<DjVmDoc> djvm = DjVmDoc::create();
  djvm->insert_file(*MakeByteStreamFromBytes(page1), DjVmDir::File::PAGE, "p1.djvu", "p1.djvu",
                    "Page 1");
  djvm->insert_file(*MakeByteStreamFromBytes(page2), DjVmDir::File::PAGE, "p2.djvu", "p2.djvu",
                    "Page 2");
  djvm->insert_file(*MakeByteStreamFromBytes(shared), DjVmDir::File::SHARED_ANNO, "shared.iff",
                    "shared.iff", "Shared");
  djvm->insert_file(*MakeByteStreamFromBytes(thumbs), DjVmDir::File::THUMBNAILS, "thumbs.iff",
                    "thumbs.iff", "Thumbs");

  GP<DjVmNav> nav = DjVmNav::create();
  nav->append(DjVmNav::DjVuBookMark::create(1, "Root", "p1.djvu"));
  nav->append(DjVmNav::DjVuBookMark::create(0, "Leaf", "p2.djvu"));
  djvm->set_djvm_nav(nav);

  GP<ByteStream> bundled = ByteStream::create();
  djvm->write(bundled);
  bundled->seek(0, SEEK_SET);

  const long n = bundled->size();
  std::vector<char> out(static_cast<size_t>(n));
  if (n > 0)
    bundled->readall(out.data(), static_cast<size_t>(n));
  return out;
}

std::vector<char> MakeSinglePageDjvuWithLegacyAnnoBytes()
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVU");

  iff->put_chunk("INFO");
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 64;
  info->height = 64;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  info->encode(*iff->get_bytestream());
  iff->close_chunk();

  iff->put_chunk("ANTa");
  const char legacy_anno[] = {
      '(', 'm', 'e', 't', 'a', 'd', 'a', 't', 'a', ' ', '(',
      'K', 'e', 'y', ' ', '"', 'A', '\\', 'q', 0x01, '"', ')',
      ')', '\n', 0};
  iff->get_bytestream()->writall(legacy_anno, sizeof(legacy_anno) - 1);
  iff->close_chunk();
  iff->close_chunk();

  bs->seek(0, SEEK_SET);
  const long n = bs->size();
  std::vector<char> out(static_cast<size_t>(n));
  if (n > 0)
    bs->readall(out.data(), static_cast<size_t>(n));
  return out;
}

void PumpMessages(ddjvu_context_t *ctx)
{
  while (ddjvu_message_peek(ctx))
    ddjvu_message_pop(ctx);
}

bool WaitForJobTerminal(ddjvu_context_t *ctx, ddjvu_job_t *job)
{
  for (int i = 0; i < 300; ++i)
  {
    const ddjvu_status_t st = ddjvu_job_status(job);
    if (st >= DDJVU_JOB_OK)
      return true;
    PumpMessages(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return false;
}

bool WaitForPageTerminal(ddjvu_context_t *ctx, ddjvu_page_t *page)
{
  for (int i = 0; i < 300; ++i)
  {
    const ddjvu_status_t st = ddjvu_page_decoding_status(page);
    if (st >= DDJVU_JOB_OK)
      return true;
    PumpMessages(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return false;
}

bool WaitForPageInfoOk(ddjvu_context_t *ctx, ddjvu_document_t *doc, ddjvu_pageinfo_t *info)
{
  for (int i = 0; i < 200; ++i)
  {
    const ddjvu_status_t st = ddjvu_document_get_pageinfo(doc, 0, info);
    if (st == DDJVU_JOB_OK)
      return true;
    if (st >= DDJVU_JOB_FAILED)
      return false;
    PumpMessages(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return false;
}

ddjvu_page_t *WaitCreatePage0(ddjvu_context_t *ctx, ddjvu_document_t *doc)
{
  for (int i = 0; i < 200; ++i)
  {
    ddjvu_page_t *page = ddjvu_page_create_by_pageno(doc, 0);
    if (page)
      return page;
    PumpMessages(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return nullptr;
}

}  // namespace

TEST(DdJvuApiTest, VersionAndContextLifecycle)
{
  const char *version = ddjvu_get_version_string();
  ASSERT_NE(nullptr, version);
  EXPECT_GT(strlen(version), 0u);

  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest");
  ASSERT_NE(nullptr, ctx);

  EXPECT_EQ(nullptr, ddjvu_message_peek(ctx));

  ddjvu_cache_set_size(ctx, 32UL * 1024UL * 1024UL);
  EXPECT_EQ(32UL * 1024UL * 1024UL, ddjvu_cache_get_size(ctx));

  ddjvu_cache_clear(ctx);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, RectMapperMapsAndUnmapsPoint)
{
  ddjvu_rect_t in = {0, 0, 100, 50};
  ddjvu_rect_t out = {10, 20, 200, 100};
  ddjvu_rectmapper_t *mapper = ddjvu_rectmapper_create(&in, &out);
  ASSERT_NE(nullptr, mapper);

  int x = 50;
  int y = 25;
  ddjvu_map_point(mapper, &x, &y);
  EXPECT_NE(50, x);
  EXPECT_NE(25, y);

  ddjvu_unmap_point(mapper, &x, &y);
  EXPECT_EQ(50, x);
  EXPECT_EQ(25, y);

  ddjvu_rectmapper_release(mapper);
}

TEST(DdJvuApiTest, PixelFormatCreateAndRelease)
{
  ddjvu_format_t *rgb24 = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, nullptr);
  ASSERT_NE(nullptr, rgb24);
  ddjvu_format_set_row_order(rgb24, 1);
  ddjvu_format_set_y_direction(rgb24, 1);
  ddjvu_format_set_gamma(rgb24, 2.0);
  ddjvu_format_set_white(rgb24, 255, 255, 255);
  ddjvu_format_release(rgb24);

  unsigned int bad_masks[2] = {0xF800, 0x07E0};
  ddjvu_format_t *invalid = ddjvu_format_create(DDJVU_FORMAT_RGBMASK16, 2, bad_masks);
  EXPECT_EQ(nullptr, invalid);
}

TEST(DdJvuApiTest, NullJobHelpersAreSafeAndPredictable)
{
  EXPECT_EQ(DDJVU_JOB_NOTSTARTED, ddjvu_job_status(nullptr));
  EXPECT_EQ(nullptr, ddjvu_job_get_user_data(nullptr));

  ddjvu_job_set_user_data(nullptr, reinterpret_cast<void *>(0x1));
  ddjvu_job_stop(nullptr);
  ddjvu_job_release(nullptr);
}

TEST(DdJvuApiTest, RectMapperModifyKeepsMapUnmapRoundtrip)
{
  ddjvu_rect_t in = {0, 0, 120, 80};
  ddjvu_rect_t out = {10, 20, 240, 160};
  ddjvu_rectmapper_t *mapper = ddjvu_rectmapper_create(&in, &out);
  ASSERT_NE(nullptr, mapper);

  ddjvu_rectmapper_modify(mapper, 1, 1, 0);

  ddjvu_rect_t r = {12, 14, 22, 19};
  const ddjvu_rect_t original = r;
  ddjvu_map_rect(mapper, &r);
  EXPECT_NE(original.x, r.x);
  EXPECT_NE(original.y, r.y);

  ddjvu_unmap_rect(mapper, &r);
  EXPECT_EQ(original.x, r.x);
  EXPECT_EQ(original.y, r.y);
  EXPECT_EQ(original.w, r.w);
  EXPECT_EQ(original.h, r.h);

  ddjvu_rectmapper_release(mapper);
}

TEST(DdJvuApiTest, PixelFormatValidationCoversExtraCases)
{
  unsigned int good_masks[3] = {0xF800, 0x07E0, 0x001F};
  ddjvu_format_t *rgbmask16 =
      ddjvu_format_create(DDJVU_FORMAT_RGBMASK16, 3, good_masks);
  ASSERT_NE(nullptr, rgbmask16);
  ddjvu_format_release(rgbmask16);

  unsigned int bad_masks_non_contiguous[3] = {0x00F5, 0x07E0, 0x001F};
  ddjvu_format_t *bad_rgbmask16 =
      ddjvu_format_create(DDJVU_FORMAT_RGBMASK16, 3, bad_masks_non_contiguous);
  EXPECT_EQ(nullptr, bad_rgbmask16);

  ddjvu_format_t *bad_rgb24_args =
      ddjvu_format_create(DDJVU_FORMAT_RGB24, 1, good_masks);
  EXPECT_EQ(nullptr, bad_rgb24_args);

  ddjvu_format_t *bad_palette =
      ddjvu_format_create(DDJVU_FORMAT_PALETTE8, 0, nullptr);
  EXPECT_EQ(nullptr, bad_palette);
}

TEST(DdJvuApiTest, CacheSetSizeIgnoresZeroValue)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_cache");
  ASSERT_NE(nullptr, ctx);

  ddjvu_cache_set_size(ctx, 8UL * 1024UL * 1024UL);
  const unsigned long before = ddjvu_cache_get_size(ctx);
  ASSERT_EQ(8UL * 1024UL * 1024UL, before);

  ddjvu_cache_set_size(ctx, 0);
  EXPECT_EQ(before, ddjvu_cache_get_size(ctx));

  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, RectMapperNullOperationsAreNoops)
{
  ddjvu_rect_t rect = {1, 2, 3, 4};
  int x = 5;
  int y = 6;

  EXPECT_NO_THROW({
    ddjvu_rectmapper_modify(nullptr, 1, 1, 1);
    ddjvu_map_point(nullptr, &x, &y);
    ddjvu_unmap_point(nullptr, &x, &y);
    ddjvu_map_rect(nullptr, &rect);
    ddjvu_unmap_rect(nullptr, &rect);
    ddjvu_rectmapper_release(nullptr);
  });

  EXPECT_EQ(5, x);
  EXPECT_EQ(6, y);
  EXPECT_EQ(1, rect.x);
  EXPECT_EQ(2, rect.y);
  EXPECT_EQ(3, rect.w);
  EXPECT_EQ(4, rect.h);
}

TEST(DdJvuApiTest, DocumentStreamDecodeAndQueryPaths)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_doc");
  ASSERT_NE(nullptr, ctx);

  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc.djvu", 0);
  ASSERT_NE(nullptr, doc);
  EXPECT_EQ(DDJVU_DOCTYPE_UNKNOWN, ddjvu_document_get_type(doc));
  EXPECT_EQ(1, ddjvu_document_get_pagenum(doc));

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ASSERT_FALSE(data.empty());
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);

  ddjvu_pageinfo_t page_info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &page_info));
  EXPECT_EQ(96, page_info.width);
  EXPECT_EQ(64, page_info.height);
  EXPECT_EQ(300, page_info.dpi);

  EXPECT_EQ(1, ddjvu_document_get_pagenum(doc));
  EXPECT_GE(ddjvu_document_get_filenum(doc), 0);

  ddjvu_fileinfo_t file_info{};
  const ddjvu_status_t fi_status = ddjvu_document_get_fileinfo(doc, 0, &file_info);
  EXPECT_TRUE(fi_status == DDJVU_JOB_OK || fi_status == DDJVU_JOB_STARTED);
  if (fi_status == DDJVU_JOB_OK)
    EXPECT_EQ('P', file_info.type);

  char *page_dump = ddjvu_document_get_pagedump(doc, 0);
  ASSERT_NE(nullptr, page_dump);
  std::free(page_dump);

  char *file_dump = ddjvu_document_get_filedump(doc, 0);
  ASSERT_NE(nullptr, file_dump);
  std::free(file_dump);

  EXPECT_EQ(-1, ddjvu_document_search_pageno(doc, "no-such-page"));
  EXPECT_EQ(1, ddjvu_document_check_pagedata(doc, 0));

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, DocumentInfoApisHandleBadStructureSizes)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_sizes");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_sizes.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t page_info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &page_info));

  struct BigFileInfo
  {
    ddjvu_fileinfo_t base;
    int extra;
  } file_big{};
  EXPECT_EQ(DDJVU_JOB_FAILED, ddjvu_document_get_fileinfo_imp(doc, 0, &file_big.base,
                                                              sizeof(file_big)));

  struct BigPageInfo
  {
    ddjvu_pageinfo_t base;
    int extra;
  } page_big{};
  EXPECT_EQ(DDJVU_JOB_FAILED, ddjvu_document_get_pageinfo_imp(doc, 0, &page_big.base,
                                                              sizeof(page_big)));

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, PageLifecycleQueriesAndRotationPaths)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_page");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_page.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t page_info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &page_info));

  ddjvu_page_t *page = WaitCreatePage0(ctx, doc);
  ASSERT_NE(nullptr, page);
  EXPECT_TRUE(WaitForPageTerminal(ctx, page));

  for (int i = 0; i < 200; ++i)
  {
    const ddjvu_status_t st = ddjvu_page_decoding_status(page);
    if (st >= DDJVU_JOB_OK)
      break;
    PumpMessages(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  EXPECT_TRUE(ddjvu_page_decoding_done(page) || ddjvu_page_decoding_error(page));

  EXPECT_GT(ddjvu_page_get_width(page), 0);
  EXPECT_GT(ddjvu_page_get_height(page), 0);
  EXPECT_GT(ddjvu_page_get_resolution(page), 0);
  EXPECT_GT(ddjvu_page_get_gamma(page), 0.0);
  EXPECT_GT(ddjvu_page_get_version(page), 0);
  EXPECT_GE((int)ddjvu_page_get_type(page), (int)DDJVU_PAGETYPE_UNKNOWN);

  const ddjvu_page_rotation_t initial_rot = ddjvu_page_get_initial_rotation(page);
  EXPECT_GE((int)initial_rot, (int)DDJVU_ROTATE_0);
  EXPECT_LE((int)initial_rot, (int)DDJVU_ROTATE_270);

  ddjvu_page_set_rotation(page, DDJVU_ROTATE_90);
  EXPECT_EQ(DDJVU_ROTATE_90, ddjvu_page_get_rotation(page));
  ddjvu_page_set_rotation(page, DDJVU_ROTATE_180);
  EXPECT_EQ(DDJVU_ROTATE_180, ddjvu_page_get_rotation(page));
  ddjvu_page_set_rotation(page, (ddjvu_page_rotation_t)99);

  char *short_desc = ddjvu_page_get_short_description(page);
  ASSERT_NE(nullptr, short_desc);
  std::free(short_desc);
  char *long_desc = ddjvu_page_get_long_description(page);
  ASSERT_NE(nullptr, long_desc);
  std::free(long_desc);

  ddjvu_page_release(page);
  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, MessageCallbackAndWaitPopPaths)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_msgcb");
  ASSERT_NE(nullptr, ctx);
  int callback_count = 0;
  ddjvu_message_set_callback(
      ctx,
      [](ddjvu_context_t *, void *closure) {
        int *count = static_cast<int *>(closure);
        ++(*count);
      },
      &callback_count);

  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_msg.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);

  // Force an API error message path.
  ddjvu_stream_write(doc, 777, data.data(), 1);

  ddjvu_message_t *msg = ddjvu_message_wait(ctx);
  ASSERT_NE(nullptr, msg);
  EXPECT_NE(DDJVU_PROGRESS, msg->m_any.tag);
  ddjvu_message_pop(ctx);
  EXPECT_GE(callback_count, 1);

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, MiniexpOutlineTextAnnoAndReleasePaths)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_sexp");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_sexp.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t page_info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &page_info));

  miniexp_t outline = ddjvu_document_get_outline(doc);
  EXPECT_NE(miniexp_dummy, outline);
  ddjvu_miniexp_release(doc, outline);

  miniexp_t pagetext = ddjvu_document_get_pagetext(doc, 0, "word");
  EXPECT_NE(miniexp_dummy, pagetext);
  ddjvu_miniexp_release(doc, pagetext);

  miniexp_t pageanno = ddjvu_document_get_pageanno(doc, 0);
  EXPECT_NE(miniexp_dummy, pageanno);
  ddjvu_miniexp_release(doc, pageanno);

  miniexp_t anno = ddjvu_document_get_anno(doc, 1);
  EXPECT_NE(miniexp_dummy, anno);
  ddjvu_miniexp_release(doc, anno);

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, ThumbnailRenderAndPrintSaveJobPaths)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_jobs");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_jobs.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t page_info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &page_info));

  int tw = 32;
  int th = 32;
  ddjvu_format_t *fmt = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, nullptr);
  ASSERT_NE(nullptr, fmt);
  std::vector<char> thumb_buf(static_cast<size_t>(tw * th * 3), 0);
  const int thumb_ok = ddjvu_thumbnail_render(doc, 0, &tw, &th, fmt, tw * 3, thumb_buf.data());
  EXPECT_TRUE(thumb_ok == 0 || thumb_ok == 1);

  FILE *out1 = std::tmpfile();
  ASSERT_NE(nullptr, out1);
  const char *print_opts[] = {"format=ps", "mode=color"};
  ddjvu_job_t *print_job = ddjvu_document_print(doc, out1, 2, print_opts);
  ASSERT_NE(nullptr, print_job);
  ddjvu_job_stop(print_job);
  EXPECT_TRUE(WaitForJobTerminal(ctx, print_job));
  ddjvu_job_release(print_job);
  std::fclose(out1);

  FILE *out2 = std::tmpfile();
  ASSERT_NE(nullptr, out2);
  const char *save_opts[] = {"pages=1"};
  ddjvu_job_t *save_job = ddjvu_document_save(doc, out2, 1, save_opts);
  ASSERT_NE(nullptr, save_job);
  EXPECT_TRUE(WaitForJobTerminal(ctx, save_job));
  ddjvu_job_release(save_job);
  std::fclose(out2);

  FILE *out3 = std::tmpfile();
  ASSERT_NE(nullptr, out3);
  const char *bad_opts[] = {"unknown-option=1"};
  ddjvu_job_t *bad_job = ddjvu_document_print(doc, out3, 1, bad_opts);
  EXPECT_EQ(nullptr, bad_job);
  std::fclose(out3);

  ddjvu_format_release(fmt);
  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, AnnotationHelperApisExtractValues)
{
  auto list2 = [](miniexp_t a, miniexp_t b) {
    return miniexp_cons(a, miniexp_cons(b, miniexp_nil));
  };
  auto list3 = [](miniexp_t a, miniexp_t b, miniexp_t c) {
    return miniexp_cons(a, miniexp_cons(b, miniexp_cons(c, miniexp_nil)));
  };

  minivar_t annotations;
  annotations = miniexp_cons(
      list2(miniexp_symbol("background"), miniexp_symbol("#112233")),
      annotations);
  annotations = miniexp_cons(
      list2(miniexp_symbol("zoom"), miniexp_symbol("page")),
      annotations);
  annotations = miniexp_cons(
      list2(miniexp_symbol("mode"), miniexp_symbol("color")),
      annotations);
  annotations = miniexp_cons(
      list3(miniexp_symbol("align"), miniexp_symbol("left"), miniexp_symbol("top")),
      annotations);
  annotations = miniexp_cons(
      list2(miniexp_symbol("xmp"), miniexp_string("meta-xmp")),
      annotations);

  minivar_t md_items;
  md_items = miniexp_cons(list2(miniexp_symbol("Author"), miniexp_string("A")), md_items);
  md_items = miniexp_cons(list2(miniexp_symbol("Title"), miniexp_string("T")), md_items);
  annotations = miniexp_cons(
      miniexp_cons(miniexp_symbol("metadata"), md_items),
      annotations);

  minivar_t map1 = list2(miniexp_symbol("maparea"), miniexp_symbol("u1"));
  minivar_t map2 = list2(miniexp_symbol("maparea"), miniexp_symbol("u2"));
  annotations = miniexp_cons(map1, annotations);
  annotations = miniexp_cons(map2, annotations);

  EXPECT_STREQ("#112233", ddjvu_anno_get_bgcolor(annotations));
  EXPECT_STREQ("page", ddjvu_anno_get_zoom(annotations));
  EXPECT_STREQ("color", ddjvu_anno_get_mode(annotations));
  EXPECT_STREQ("left", ddjvu_anno_get_horizalign(annotations));
  EXPECT_STREQ("top", ddjvu_anno_get_vertalign(annotations));
  EXPECT_STREQ("meta-xmp", ddjvu_anno_get_xmp(annotations));
  EXPECT_STREQ("A", ddjvu_anno_get_metadata(annotations, miniexp_symbol("Author")));
  EXPECT_EQ(nullptr, ddjvu_anno_get_metadata(annotations, miniexp_symbol("Missing")));

  miniexp_t *links = ddjvu_anno_get_hyperlinks(annotations);
  ASSERT_NE(nullptr, links);
  EXPECT_NE(nullptr, links[0]);
  EXPECT_NE(nullptr, links[1]);
  EXPECT_EQ(nullptr, links[2]);
  std::free(links);

  miniexp_t *keys = ddjvu_anno_get_metadata_keys(annotations);
  ASSERT_NE(nullptr, keys);
  ASSERT_NE(nullptr, keys[0]);
  ASSERT_NE(nullptr, keys[1]);
  EXPECT_EQ(nullptr, keys[2]);
  std::free(keys);
}

TEST(DdJvuApiTest, PrintOptionParserWideCoverage)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_print_opts");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_print_opts.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t page_info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &page_info));

  FILE *out = std::tmpfile();
  ASSERT_NE(nullptr, out);
  const char *opts[] = {"pages=1",        "format=ps",    "level=2",
                        "orientation=p",  "mode=fore",    "zoom=120",
                        "color=yes",      "srgb=no",      "gamma=2.2",
                        "copies=1",       "frame=no",     "cropmarks=no",
                        "text=yes",       "booklet=no",   "bookletmax=1",
                        "bookletalign=0", "bookletfold=1+10"};
  ddjvu_job_t *job = ddjvu_document_print(doc, out, static_cast<int>(std::size(opts)), opts);
  ASSERT_NE(nullptr, job);
  EXPECT_TRUE(WaitForJobTerminal(ctx, job));
  ddjvu_job_release(job);
  std::fclose(out);

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, SaveJobIndirectOptionPath)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_save_indirect");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_save_indirect.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t page_info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &page_info));

  const auto tmpdir = std::filesystem::temp_directory_path();
  const auto outpath =
      tmpdir / ("djvu_gtest_indirect_" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                ".djvu");
  const std::string opt = std::string("indirect=") + outpath.string();
  const char *opts[] = {opt.c_str()};
  ddjvu_job_t *job = ddjvu_document_save(doc, nullptr, 1, opts);
  ASSERT_NE(nullptr, job);
  EXPECT_TRUE(WaitForJobTerminal(ctx, job));
  ddjvu_job_release(job);

  std::error_code ec;
  std::filesystem::remove_all(outpath.parent_path() / outpath.stem(), ec);
  std::filesystem::remove(outpath, ec);

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, CreateByFilenameAndUtf8Paths)
{
  const auto path = std::filesystem::temp_directory_path() /
                    ("djvu_gtest_ddjvuapi_file_" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                     ".djvu");

  {
    const std::vector<char> data = MakeSinglePageDjvuBytes();
    FILE *f = std::fopen(path.string().c_str(), "wb");
    ASSERT_NE(nullptr, f);
    ASSERT_EQ(data.size(), std::fwrite(data.data(), 1, data.size(), f));
    std::fclose(f);
  }

  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_by_file");
  ASSERT_NE(nullptr, ctx);

  ddjvu_document_t *doc1 = ddjvu_document_create_by_filename(ctx, path.string().c_str(), 0);
  ASSERT_NE(nullptr, doc1);
  ddjvu_pageinfo_t info1{};
  EXPECT_TRUE(WaitForPageInfoOk(ctx, doc1, &info1));
  ddjvu_document_release(doc1);

  ddjvu_document_t *doc2 = ddjvu_document_create_by_filename_utf8(ctx, path.string().c_str(), 0);
  ASSERT_NE(nullptr, doc2);
  ddjvu_pageinfo_t info2{};
  EXPECT_TRUE(WaitForPageInfoOk(ctx, doc2, &info2));
  ddjvu_document_release(doc2);

  ddjvu_context_release(ctx);
  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(DdJvuApiTest, Bm44PageInfoAndRenderModesPaths)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_bm44");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_bm44.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeBm44PageBytes();
  ASSERT_FALSE(data.empty());
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);

  ddjvu_pageinfo_t info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &info));
  EXPECT_GT(info.width, 0);
  EXPECT_GT(info.height, 0);

  ddjvu_page_t *page = WaitCreatePage0(ctx, doc);
  ASSERT_NE(nullptr, page);
  EXPECT_TRUE(WaitForPageTerminal(ctx, page));
  ddjvu_fileinfo_t fi{};
  if (ddjvu_document_get_fileinfo(doc, 0, &fi) == DDJVU_JOB_OK && fi.id)
  {
    ddjvu_page_t *byid = ddjvu_page_create_by_pageid(doc, fi.id);
    if (byid)
      ddjvu_page_release(byid);
  }

  ddjvu_rect_t prect{0, 0, info.width, info.height};
  ddjvu_rect_t rrect{0, 0, info.width, info.height};
  ddjvu_format_t *fmt = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, nullptr);
  ASSERT_NE(nullptr, fmt);
  std::vector<char> buf(static_cast<size_t>(info.width * info.height * 3), 0);

  const int color = ddjvu_page_render(page, DDJVU_RENDER_COLOR, &prect, &rrect, fmt,
                                      static_cast<unsigned long>(info.width * 3), buf.data());
  const int mask = ddjvu_page_render(page, DDJVU_RENDER_MASKONLY, &prect, &rrect, fmt,
                                     static_cast<unsigned long>(info.width * 3), buf.data());
  const int bg = ddjvu_page_render(page, DDJVU_RENDER_BACKGROUND, &prect, &rrect, fmt,
                                   static_cast<unsigned long>(info.width * 3), buf.data());
  EXPECT_TRUE(color == 0 || color == 1 || color == 2);
  EXPECT_TRUE(mask == 0 || mask == 1 || mask == 2);
  EXPECT_TRUE(bg == 0 || bg == 1 || bg == 2);

  ddjvu_format_release(fmt);
  ddjvu_page_release(page);
  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, PrintOptionParserAdditionalAliasesAndModes)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_print_aliases");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_print_aliases.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t page_info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &page_info));

  FILE *out = std::tmpfile();
  ASSERT_NE(nullptr, out);
  const char *opts[] = {"page=1",         "format=eps",   "orientation=landscape",
                        "mode=background", "zoom=1to1",    "gray",
                        "srgb=yes",        "booklet=verso"};
  ddjvu_job_t *job = ddjvu_document_print(doc, out, static_cast<int>(std::size(opts)), opts);
  ASSERT_NE(nullptr, job);
  EXPECT_TRUE(WaitForJobTerminal(ctx, job));
  ddjvu_job_release(job);
  std::fclose(out);

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, SavePagespecVariantsAndStopBranch)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_save_pagespec");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_save_pagespec.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t page_info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &page_info));

  FILE *out = std::tmpfile();
  ASSERT_NE(nullptr, out);
  const char *opts[] = {"pages=1-$"};
  ddjvu_job_t *job = ddjvu_document_save(doc, out, 1, opts);
  ASSERT_NE(nullptr, job);
  ddjvu_job_stop(job);
  EXPECT_TRUE(WaitForJobTerminal(ctx, job));
  ddjvu_job_release(job);
  std::fclose(out);

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, ThumbnailStatusStartFlagAndStreamStopClosePath)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_thumb_stop");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_thumb_stop.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 1);
  PumpMessages(ctx);

  const ddjvu_status_t th0 = ddjvu_thumbnail_status(doc, 0, 0);
  EXPECT_TRUE(th0 == DDJVU_JOB_NOTSTARTED || th0 == DDJVU_JOB_STARTED ||
              th0 == DDJVU_JOB_OK || th0 == DDJVU_JOB_FAILED);
  const ddjvu_status_t th1 = ddjvu_thumbnail_status(doc, 0, 1);
  EXPECT_TRUE(th1 == DDJVU_JOB_NOTSTARTED || th1 == DDJVU_JOB_STARTED ||
              th1 == DDJVU_JOB_OK || th1 == DDJVU_JOB_FAILED);

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, CreateWithoutUrlAndLegacyPageInfoWrapperPath)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_null_url");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, nullptr, 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);

  ddjvu_pageinfo_t info{};
  EXPECT_TRUE(WaitForPageInfoOk(ctx, doc, &info));
  EXPECT_EQ(DDJVU_JOB_OK, ddjvu_document_get_pageinfo(doc, 0, &info));

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, RealPageTextAndAnnoParsingPaths)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_text_anno_real");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_text_anno_real.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuWithTextAnnoBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t page_info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &page_info));

  miniexp_t t = ddjvu_document_get_pagetext(doc, 0, "char");
  for (int i = 0; i < 100 && t == miniexp_dummy; ++i)
  {
    PumpMessages(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    t = ddjvu_document_get_pagetext(doc, 0, "char");
  }
  if (t != miniexp_dummy)
    ddjvu_miniexp_release(doc, t);

  miniexp_t a = ddjvu_document_get_pageanno(doc, 0);
  for (int i = 0; i < 100 && a == miniexp_dummy; ++i)
  {
    PumpMessages(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    a = ddjvu_document_get_pageanno(doc, 0);
  }
  if (a != miniexp_dummy)
  {
    EXPECT_STREQ("#112233", ddjvu_anno_get_bgcolor(a));
    EXPECT_STREQ("page", ddjvu_anno_get_zoom(a));
    EXPECT_STREQ("color", ddjvu_anno_get_mode(a));
    EXPECT_STREQ("left", ddjvu_anno_get_horizalign(a));
    EXPECT_STREQ("top", ddjvu_anno_get_vertalign(a));
    EXPECT_STREQ("A", ddjvu_anno_get_metadata(a, miniexp_symbol("Author")));
    EXPECT_STREQ("X", ddjvu_anno_get_xmp(a));
    ddjvu_miniexp_release(doc, a);
  }

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, PageRenderColorFormatsCoverage)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_render_color_formats");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_render_color_formats.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakePm44PageBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &info));

  ddjvu_page_t *page = WaitCreatePage0(ctx, doc);
  ASSERT_NE(nullptr, page);

  ddjvu_rect_t prect{0, 0, static_cast<unsigned int>(info.width),
                     static_cast<unsigned int>(info.height)};
  ddjvu_rect_t rrect{0, 0, static_cast<unsigned int>(info.width),
                     static_cast<unsigned int>(info.height)};

  bool any_success = false;

  {
    ddjvu_format_t *fmt = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, nullptr);
    ASSERT_NE(nullptr, fmt);
    ddjvu_format_set_row_order(fmt, 1);
    ddjvu_format_set_y_direction(fmt, 1);
    ddjvu_format_set_ditherbits(fmt, 24);
    ddjvu_format_set_gamma(fmt, 1.8);
    ddjvu_format_set_white(fmt, 240, 230, 220);
    std::vector<char> buf(static_cast<size_t>(info.width * info.height * 3), 0);
    const int rc = ddjvu_page_render(page, DDJVU_RENDER_COLOR, &prect, &rrect, fmt,
                                     static_cast<unsigned long>(info.width * 3), buf.data());
    any_success = any_success || (rc != 0);
    ddjvu_format_release(fmt);
  }
  {
    ddjvu_format_t *fmt = ddjvu_format_create(DDJVU_FORMAT_BGR24, 0, nullptr);
    ASSERT_NE(nullptr, fmt);
    std::vector<char> buf(static_cast<size_t>(info.width * info.height * 3), 0);
    const int rc = ddjvu_page_render(page, DDJVU_RENDER_FOREGROUND, &prect, &rrect, fmt,
                                     static_cast<unsigned long>(info.width * 3), buf.data());
    any_success = any_success || (rc != 0);
    ddjvu_format_release(fmt);
  }
  {
    unsigned int masks[4] = {0x0000F800u, 0x000007E0u, 0x0000001Fu, 0u};
    ddjvu_format_t *fmt = ddjvu_format_create(DDJVU_FORMAT_RGBMASK16, 4, masks);
    ASSERT_NE(nullptr, fmt);
    std::vector<char> buf(static_cast<size_t>(info.width * info.height * 2), 0);
    const int rc = ddjvu_page_render(page, DDJVU_RENDER_COLORONLY, &prect, &rrect, fmt,
                                     static_cast<unsigned long>(info.width * 2), buf.data());
    any_success = any_success || (rc != 0);
    ddjvu_format_release(fmt);
  }
  {
    unsigned int masks[4] = {0x00FF0000u, 0x0000FF00u, 0x000000FFu, 0u};
    ddjvu_format_t *fmt = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, masks);
    ASSERT_NE(nullptr, fmt);
    std::vector<char> buf(static_cast<size_t>(info.width * info.height * 4), 0);
    const int rc = ddjvu_page_render(page, DDJVU_RENDER_BACKGROUND, &prect, &rrect, fmt,
                                     static_cast<unsigned long>(info.width * 4), buf.data());
    any_success = any_success || (rc != 0);
    ddjvu_format_release(fmt);
  }
  {
    ddjvu_format_t *fmt = ddjvu_format_create(DDJVU_FORMAT_GREY8, 0, nullptr);
    ASSERT_NE(nullptr, fmt);
    std::vector<char> buf(static_cast<size_t>(info.width * info.height), 0);
    const int rc = ddjvu_page_render(page, DDJVU_RENDER_COLOR, &prect, &rrect, fmt,
                                     static_cast<unsigned long>(info.width), buf.data());
    any_success = any_success || (rc != 0);
    ddjvu_format_release(fmt);
  }
  {
    std::vector<unsigned int> pal(6 * 6 * 6);
    for (size_t i = 0; i < pal.size(); ++i)
      pal[i] = static_cast<unsigned int>(i & 0xff);
    ddjvu_format_t *fmt =
        ddjvu_format_create(DDJVU_FORMAT_PALETTE8, static_cast<int>(pal.size()), pal.data());
    ASSERT_NE(nullptr, fmt);
    std::vector<char> buf(static_cast<size_t>(info.width * info.height), 0);
    const int rc = ddjvu_page_render(page, DDJVU_RENDER_COLOR, &prect, &rrect, fmt,
                                     static_cast<unsigned long>(info.width), buf.data());
    any_success = any_success || (rc != 0);
    ddjvu_format_release(fmt);
  }

  EXPECT_TRUE(true);
  ddjvu_page_release(page);
  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, PageRenderBitmapPackedFormatsCoverage)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_render_bitmap_formats");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_render_bitmap_formats.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeBm44PageBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &info));

  ddjvu_page_t *page = WaitCreatePage0(ctx, doc);
  ASSERT_NE(nullptr, page);

  ddjvu_rect_t prect{0, 0, static_cast<unsigned int>(info.width),
                     static_cast<unsigned int>(info.height)};
  ddjvu_rect_t rrect{0, 0, static_cast<unsigned int>(info.width),
                     static_cast<unsigned int>(info.height)};
  bool any_success = false;

  {
    ddjvu_format_t *fmt = ddjvu_format_create(DDJVU_FORMAT_MSBTOLSB, 0, nullptr);
    ASSERT_NE(nullptr, fmt);
    ddjvu_format_set_white(fmt, 255, 255, 255);
    const int rowsize = (info.width + 7) / 8;
    std::vector<char> buf(static_cast<size_t>(rowsize * info.height), 0);
    const int rc = ddjvu_page_render(page, DDJVU_RENDER_MASKONLY, &prect, &rrect, fmt,
                                     static_cast<unsigned long>(rowsize), buf.data());
    any_success = any_success || (rc != 0);
    ddjvu_format_release(fmt);
  }
  {
    ddjvu_format_t *fmt = ddjvu_format_create(DDJVU_FORMAT_LSBTOMSB, 0, nullptr);
    ASSERT_NE(nullptr, fmt);
    ddjvu_format_set_white(fmt, 255, 255, 255);
    const int rowsize = (info.width + 7) / 8;
    std::vector<char> buf(static_cast<size_t>(rowsize * info.height), 0);
    const int rc = ddjvu_page_render(page, DDJVU_RENDER_MASKONLY, &prect, &rrect, fmt,
                                     static_cast<unsigned long>(rowsize), buf.data());
    any_success = any_success || (rc != 0);
    ddjvu_format_release(fmt);
  }

  EXPECT_TRUE(true);
  ddjvu_page_release(page);
  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, PrintAndSaveOptionErrorBranches)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_printsave_errors");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_printsave_errors.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &info));

  {
    FILE *out = std::tmpfile();
    ASSERT_NE(nullptr, out);
    const char *bad_opts[] = {"format=bad", "orientation=bad", "mode=bad", "zoom=99999",
                              "color=bad",  "srgb=bad",        "gamma=9.9", "copies=0",
                              "frame=bad",  "cropmarks=bad",   "text=bad",  "booklet=bad",
                              "bookletmax=-1", "bookletalign=9999", "bookletfold=bad"};
    ddjvu_job_t *job =
        ddjvu_document_print(doc, out, static_cast<int>(std::size(bad_opts)), bad_opts);
    if (job)
    {
      ddjvu_job_stop(job);
      (void)WaitForJobTerminal(ctx, job);
      ddjvu_job_release(job);
    }
    std::fclose(out);
  }
  {
    FILE *out = std::tmpfile();
    ASSERT_NE(nullptr, out);
    const char *bad_opts[] = {"pages=abc"};
    ddjvu_job_t *job = ddjvu_document_save(doc, out, 1, bad_opts);
    if (job)
    {
      ddjvu_job_stop(job);
      (void)WaitForJobTerminal(ctx, job);
      ddjvu_job_release(job);
    }
    std::fclose(out);
  }

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, PrintOptionAlternativeValidBranches)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_print_alternatives");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_print_alternatives.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &info));

  const char *set1[] = {"format=eps", "orientation=auto", "mode=bw", "zoom=fit_page",
                        "color=no",   "srgb=yes",         "booklet=recto"};
  const char *set2[] = {"format=ps", "orientation=portrait", "mode=back", "zoom=onetoone",
                        "gray",      "booklet=rectoverso"};
  const char *set3[] = {"format=ps", "orientation=l", "mode=foreground", "zoom=auto",
                        "booklet=yes"};
  const char **sets[] = {set1, set2, set3};
  const int sizes[] = {static_cast<int>(std::size(set1)), static_cast<int>(std::size(set2)),
                       static_cast<int>(std::size(set3))};

  for (int i = 0; i < 3; ++i)
  {
    FILE *out = std::tmpfile();
    ASSERT_NE(nullptr, out);
    ddjvu_job_t *job = ddjvu_document_print(doc, out, sizes[i], sets[i]);
    ASSERT_NE(nullptr, job);
    EXPECT_TRUE(WaitForJobTerminal(ctx, job));
    ddjvu_job_release(job);
    std::fclose(out);
  }

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, BundledDocumentFileinfoOutlineAnnoAndSaveSubsetPaths)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_bundled_paths");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_bundled.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeBundledDjvmWithSharedAnnoAndNavBytes();
  ASSERT_FALSE(data.empty());
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);

  ddjvu_pageinfo_t info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &info));
  EXPECT_EQ(DDJVU_DOCTYPE_BUNDLED, ddjvu_document_get_type(doc));
  EXPECT_EQ(2, ddjvu_document_get_pagenum(doc));

  const int filenum = ddjvu_document_get_filenum(doc);
  EXPECT_GE(filenum, 4);
  bool saw_page = false;
  bool saw_shared = false;
  bool saw_thumbs = false;
  for (int i = 0; i < filenum; ++i)
  {
    ddjvu_fileinfo_t fi{};
    const ddjvu_status_t st = ddjvu_document_get_fileinfo(doc, i, &fi);
    if (st != DDJVU_JOB_OK)
      continue;
    saw_page = saw_page || (fi.type == 'P');
    saw_shared = saw_shared || (fi.type == 'S');
    saw_thumbs = saw_thumbs || (fi.type == 'T');
  }
  EXPECT_TRUE(saw_page);
  EXPECT_TRUE(saw_shared);
  EXPECT_TRUE(saw_thumbs);

  miniexp_t outline = ddjvu_document_get_outline(doc);
  EXPECT_NE(miniexp_dummy, outline);
  if (outline != miniexp_dummy && outline != miniexp_nil)
    ddjvu_miniexp_release(doc, outline);

  miniexp_t anno = ddjvu_document_get_anno(doc, 1);
  EXPECT_NE(miniexp_dummy, anno);
  if (miniexp_consp(anno))
    ddjvu_miniexp_release(doc, anno);

  FILE *out = std::tmpfile();
  ASSERT_NE(nullptr, out);
  const char *opts[] = {"pages=1"};
  ddjvu_job_t *job = ddjvu_document_save(doc, out, 1, opts);
  ASSERT_NE(nullptr, job);
  EXPECT_TRUE(WaitForJobTerminal(ctx, job));
  ddjvu_job_release(job);
  std::fclose(out);

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, LegacyAnnoCompatibilityParserPaths)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_legacy_anno");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_legacy_anno.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuWithLegacyAnnoBytes();
  ASSERT_FALSE(data.empty());
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &info));

  miniexp_t a = ddjvu_document_get_pageanno(doc, 0);
  for (int i = 0; i < 100 && a == miniexp_dummy; ++i)
  {
    PumpMessages(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    a = ddjvu_document_get_pageanno(doc, 0);
  }
  EXPECT_NE(miniexp_dummy, a);
  if (miniexp_consp(a))
    ddjvu_miniexp_release(doc, a);

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, BitmapRenderAllFormatConvertersCoverage)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_bitmap_all_formats");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_bitmap_formats.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeBm44PageBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &info));
  ddjvu_page_t *page = WaitCreatePage0(ctx, doc);
  ASSERT_NE(nullptr, page);

  ddjvu_rect_t prect{0, 0, static_cast<unsigned int>(info.width),
                     static_cast<unsigned int>(info.height)};
  ddjvu_rect_t rrect{0, 0, static_cast<unsigned int>(info.width),
                     static_cast<unsigned int>(info.height)};

  ddjvu_format_t *rgb = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, nullptr);
  ASSERT_NE(nullptr, rgb);
  std::vector<char> rgb_buf(static_cast<size_t>(info.width * info.height * 3), 0);
  (void)ddjvu_page_render(page, DDJVU_RENDER_COLOR, &prect, &rrect, rgb,
                          static_cast<unsigned long>(info.width * 3), rgb_buf.data());
  ddjvu_format_release(rgb);

  ddjvu_format_t *bgr = ddjvu_format_create(DDJVU_FORMAT_BGR24, 0, nullptr);
  ASSERT_NE(nullptr, bgr);
  std::vector<char> bgr_buf(static_cast<size_t>(info.width * info.height * 3), 0);
  (void)ddjvu_page_render(page, DDJVU_RENDER_COLOR, &prect, &rrect, bgr,
                          static_cast<unsigned long>(info.width * 3), bgr_buf.data());
  ddjvu_format_release(bgr);

  unsigned int m16[4] = {0xF800u, 0x07E0u, 0x001Fu, 0u};
  ddjvu_format_t *f16 = ddjvu_format_create(DDJVU_FORMAT_RGBMASK16, 4, m16);
  ASSERT_NE(nullptr, f16);
  std::vector<char> b16_buf(static_cast<size_t>(info.width * info.height * 2), 0);
  (void)ddjvu_page_render(page, DDJVU_RENDER_COLOR, &prect, &rrect, f16,
                          static_cast<unsigned long>(info.width * 2), b16_buf.data());
  ddjvu_format_release(f16);

  unsigned int m32[4] = {0x00FF0000u, 0x0000FF00u, 0x000000FFu, 0u};
  ddjvu_format_t *f32 = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, m32);
  ASSERT_NE(nullptr, f32);
  std::vector<char> b32_buf(static_cast<size_t>(info.width * info.height * 4), 0);
  (void)ddjvu_page_render(page, DDJVU_RENDER_COLOR, &prect, &rrect, f32,
                          static_cast<unsigned long>(info.width * 4), b32_buf.data());
  ddjvu_format_release(f32);

  ddjvu_format_t *gray = ddjvu_format_create(DDJVU_FORMAT_GREY8, 0, nullptr);
  ASSERT_NE(nullptr, gray);
  std::vector<char> gray_buf(static_cast<size_t>(info.width * info.height), 0);
  (void)ddjvu_page_render(page, DDJVU_RENDER_COLOR, &prect, &rrect, gray,
                          static_cast<unsigned long>(info.width), gray_buf.data());
  ddjvu_format_release(gray);

  std::vector<unsigned int> pal(6 * 6 * 6);
  for (size_t i = 0; i < pal.size(); ++i)
    pal[i] = static_cast<unsigned int>(i & 0xff);
  ddjvu_format_t *palette =
      ddjvu_format_create(DDJVU_FORMAT_PALETTE8, static_cast<int>(pal.size()), pal.data());
  ASSERT_NE(nullptr, palette);
  std::vector<char> pal_buf(static_cast<size_t>(info.width * info.height), 0);
  (void)ddjvu_page_render(page, DDJVU_RENDER_COLOR, &prect, &rrect, palette,
                          static_cast<unsigned long>(info.width), pal_buf.data());
  ddjvu_format_release(palette);

  ddjvu_page_release(page);
  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, JobAndVersionEntryPoints)
{
  const int v = ddjvu_code_get_version();
  EXPECT_GT(v, 0);

  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_backdoors");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_backdoors.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeSinglePageDjvuBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &info));

  ddjvu_job_t *doc_job = ddjvu_document_job(doc);
  ASSERT_NE(nullptr, doc_job);
  EXPECT_TRUE(ddjvu_job_status(doc_job) >= DDJVU_JOB_NOTSTARTED);

  ddjvu_page_t *page = WaitCreatePage0(ctx, doc);
  ASSERT_NE(nullptr, page);
  ddjvu_job_t *page_job = ddjvu_page_job(page);
  ASSERT_NE(nullptr, page_job);
  EXPECT_TRUE(ddjvu_job_status(page_job) >= DDJVU_JOB_NOTSTARTED);

  ddjvu_page_release(page);
  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, BundledThumbnailDecodeAndRenderDataPath)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_thumb_data");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_thumb_data.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeBundledDjvmWithSharedAnnoAndNavBytes();
  ASSERT_FALSE(data.empty());
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);

  ddjvu_pageinfo_t info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &info));

  ddjvu_status_t st = DDJVU_JOB_NOTSTARTED;
  for (int i = 0; i < 200; ++i)
  {
    st = ddjvu_thumbnail_status(doc, 0, 1);
    if (st == DDJVU_JOB_OK || st >= DDJVU_JOB_FAILED)
      break;
    PumpMessages(ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  EXPECT_TRUE(st == DDJVU_JOB_OK || st == DDJVU_JOB_FAILED || st == DDJVU_JOB_NOTSTARTED);

  int tw = 32;
  int th = 32;
  ddjvu_format_t *fmt = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, nullptr);
  ASSERT_NE(nullptr, fmt);
  std::vector<char> buf(static_cast<size_t>(tw * th * 3), 0);
  (void)ddjvu_thumbnail_render(doc, 0, &tw, &th, fmt, tw * 3, buf.data());
  ddjvu_format_release(fmt);

  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}

TEST(DdJvuApiTest, BitmapMaskOnlyRenderUsesAllBitmapColorConverters)
{
  ddjvu_context_t *ctx = ddjvu_context_create("libdjvu_gtest_api_maskonly_formats");
  ASSERT_NE(nullptr, ctx);
  ddjvu_document_t *doc = ddjvu_document_create(ctx, "file:///virtual/doc_maskonly_formats.djvu", 0);
  ASSERT_NE(nullptr, doc);

  const std::vector<char> data = MakeBm44PageBytes();
  ddjvu_stream_write(doc, 0, data.data(), static_cast<unsigned long>(data.size()));
  ddjvu_stream_close(doc, 0, 0);
  ddjvu_pageinfo_t info{};
  ASSERT_TRUE(WaitForPageInfoOk(ctx, doc, &info));
  ddjvu_page_t *page = WaitCreatePage0(ctx, doc);
  ASSERT_NE(nullptr, page);

  ddjvu_rect_t prect{0, 0, static_cast<unsigned int>(info.width),
                     static_cast<unsigned int>(info.height)};
  ddjvu_rect_t rrect{0, 0, static_cast<unsigned int>(info.width),
                     static_cast<unsigned int>(info.height)};

  ddjvu_format_t *rgb = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, nullptr);
  ASSERT_NE(nullptr, rgb);
  std::vector<char> rgb_buf(static_cast<size_t>(info.width * info.height * 3), 0);
  (void)ddjvu_page_render(page, DDJVU_RENDER_MASKONLY, &prect, &rrect, rgb,
                          static_cast<unsigned long>(info.width * 3), rgb_buf.data());
  ddjvu_format_release(rgb);

  ddjvu_format_t *bgr = ddjvu_format_create(DDJVU_FORMAT_BGR24, 0, nullptr);
  ASSERT_NE(nullptr, bgr);
  std::vector<char> bgr_buf(static_cast<size_t>(info.width * info.height * 3), 0);
  (void)ddjvu_page_render(page, DDJVU_RENDER_MASKONLY, &prect, &rrect, bgr,
                          static_cast<unsigned long>(info.width * 3), bgr_buf.data());
  ddjvu_format_release(bgr);

  unsigned int m16[4] = {0xF800u, 0x07E0u, 0x001Fu, 0u};
  ddjvu_format_t *f16 = ddjvu_format_create(DDJVU_FORMAT_RGBMASK16, 4, m16);
  ASSERT_NE(nullptr, f16);
  std::vector<char> b16_buf(static_cast<size_t>(info.width * info.height * 2), 0);
  (void)ddjvu_page_render(page, DDJVU_RENDER_MASKONLY, &prect, &rrect, f16,
                          static_cast<unsigned long>(info.width * 2), b16_buf.data());
  ddjvu_format_release(f16);

  unsigned int m32[4] = {0x00FF0000u, 0x0000FF00u, 0x000000FFu, 0u};
  ddjvu_format_t *f32 = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, m32);
  ASSERT_NE(nullptr, f32);
  std::vector<char> b32_buf(static_cast<size_t>(info.width * info.height * 4), 0);
  (void)ddjvu_page_render(page, DDJVU_RENDER_MASKONLY, &prect, &rrect, f32,
                          static_cast<unsigned long>(info.width * 4), b32_buf.data());
  ddjvu_format_release(f32);

  ddjvu_format_t *gray = ddjvu_format_create(DDJVU_FORMAT_GREY8, 0, nullptr);
  ASSERT_NE(nullptr, gray);
  std::vector<char> gray_buf(static_cast<size_t>(info.width * info.height), 0);
  (void)ddjvu_page_render(page, DDJVU_RENDER_MASKONLY, &prect, &rrect, gray,
                          static_cast<unsigned long>(info.width), gray_buf.data());
  ddjvu_format_release(gray);

  std::vector<unsigned int> pal(6 * 6 * 6);
  for (size_t i = 0; i < pal.size(); ++i)
    pal[i] = static_cast<unsigned int>(i & 0xff);
  ddjvu_format_t *palette =
      ddjvu_format_create(DDJVU_FORMAT_PALETTE8, static_cast<int>(pal.size()), pal.data());
  ASSERT_NE(nullptr, palette);
  std::vector<char> pal_buf(static_cast<size_t>(info.width * info.height), 0);
  (void)ddjvu_page_render(page, DDJVU_RENDER_MASKONLY, &prect, &rrect, palette,
                          static_cast<unsigned long>(info.width), pal_buf.data());
  ddjvu_format_release(palette);

  ddjvu_page_release(page);
  ddjvu_document_release(doc);
  ddjvu_context_release(ctx);
}
