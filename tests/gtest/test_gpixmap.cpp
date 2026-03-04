#include <gtest/gtest.h>

#include "ByteStream.h"
#include "GPixmap.h"
#include "GRect.h"

TEST(GPixmapTest, InitWithFillerAndAccessPixels)
{
  GP<GPixmap> pm = GPixmap::create(2, 3, &GPixel::GREEN);
  ASSERT_TRUE(pm != 0);
  EXPECT_EQ(2u, pm->rows());
  EXPECT_EQ(3u, pm->columns());

  const GPixel px = (*pm)[1][2];
  EXPECT_EQ(GPixel::GREEN, px);
}

TEST(GPixmapTest, SaveAndLoadPpmRoundtrip)
{
  GP<GPixmap> src = GPixmap::create(2, 2, &GPixel::BLACK);
  (*src)[0][0] = GPixel::RED;
  (*src)[0][1] = GPixel::GREEN;
  (*src)[1][0] = GPixel::BLUE;
  (*src)[1][1] = GPixel::WHITE;

  GP<ByteStream> bs = ByteStream::create();
  src->save_ppm(*bs, 1);
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  GP<GPixmap> decoded = GPixmap::create(*bs);
  ASSERT_TRUE(decoded != 0);
  EXPECT_EQ(2u, decoded->rows());
  EXPECT_EQ(2u, decoded->columns());
  EXPECT_EQ(GPixel::RED, (*decoded)[0][0]);
  EXPECT_EQ(GPixel::GREEN, (*decoded)[0][1]);
  EXPECT_EQ(GPixel::BLUE, (*decoded)[1][0]);
  EXPECT_EQ(GPixel::WHITE, (*decoded)[1][1]);
}

TEST(GPixmapTest, RotateOneStepMatchesImplementation)
{
  GP<GPixmap> src = GPixmap::create(2, 3, &GPixel::BLACK);
  (*src)[0][0] = GPixel::RED;
  (*src)[0][1] = GPixel::GREEN;
  (*src)[0][2] = GPixel::BLUE;
  (*src)[1][0] = GPixel::WHITE;

  GP<GPixmap> rotated = src->rotate(1);
  ASSERT_TRUE(rotated != 0);
  EXPECT_EQ(3u, rotated->rows());
  EXPECT_EQ(2u, rotated->columns());
  EXPECT_EQ(GPixel::WHITE, (*rotated)[0][0]);
  EXPECT_EQ(GPixel::RED, (*rotated)[0][1]);
  EXPECT_EQ(GPixel::GREEN, (*rotated)[1][1]);
  EXPECT_EQ(GPixel::BLUE, (*rotated)[2][1]);
}
