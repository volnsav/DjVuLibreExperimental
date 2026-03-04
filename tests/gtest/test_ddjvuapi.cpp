#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "ddjvuapi.h"
}

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
