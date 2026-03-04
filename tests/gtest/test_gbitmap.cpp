#include <gtest/gtest.h>

#include "GBitmap.h"
#include "GRect.h"

TEST(GBitmapTest, InitFillAndBasicAccess)
{
  GP<GBitmap> bm = GBitmap::create(2, 3, 1);
  ASSERT_TRUE(bm != 0);
  EXPECT_EQ(2u, bm->rows());
  EXPECT_EQ(3u, bm->columns());
  EXPECT_EQ(2, bm->get_grays());

  bm->fill(1);
  EXPECT_EQ(1, (*bm)[0][0]);
  EXPECT_EQ(1, (*bm)[1][2]);
}

TEST(GBitmapTest, ChangeAndBinarizeGrays)
{
  GP<GBitmap> bm = GBitmap::create(1, 3, 0);
  bm->set_grays(4);

  (*bm)[0][0] = 0;
  (*bm)[0][1] = 1;
  (*bm)[0][2] = 3;

  bm->change_grays(2);
  EXPECT_EQ(2, bm->get_grays());
  EXPECT_EQ(0, (*bm)[0][0]);
  EXPECT_EQ(0, (*bm)[0][1]);
  EXPECT_EQ(1, (*bm)[0][2]);

  bm->set_grays(5);
  (*bm)[0][0] = 1;
  (*bm)[0][1] = 2;
  (*bm)[0][2] = 4;
  bm->binarize_grays(1);
  EXPECT_EQ(2, bm->get_grays());
  EXPECT_EQ(0, (*bm)[0][0]);
  EXPECT_EQ(1, (*bm)[0][1]);
  EXPECT_EQ(1, (*bm)[0][2]);
}

TEST(GBitmapTest, CompressRleRectAndRotate180)
{
  GP<GBitmap> bm = GBitmap::create(2, 3, 0);
  (*bm)[0][0] = 1;
  (*bm)[1][2] = 1;

  bm->compress();
  unsigned int rle_len = 0;
  const unsigned char *rle = bm->get_rle(rle_len);
  EXPECT_NE(nullptr, rle);
  EXPECT_GT(rle_len, 0u);

  GRect rect;
  const int area = bm->rle_get_rect(rect);
  EXPECT_EQ(2, area);
  EXPECT_EQ(0, rect.xmin);
  EXPECT_EQ(0, rect.ymin);
  EXPECT_EQ(2, rect.xmax);
  EXPECT_EQ(1, rect.ymax);

  GP<GBitmap> rot = bm->rotate(2);
  EXPECT_EQ(2u, rot->rows());
  EXPECT_EQ(3u, rot->columns());
  EXPECT_EQ(1, (*rot)[1][2]);
  EXPECT_EQ(1, (*rot)[0][0]);
}
