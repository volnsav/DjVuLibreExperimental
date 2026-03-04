#include <gtest/gtest.h>

#include "GScaler.h"

TEST(GScalerTest, BitmapScalerScalesAndKeepsCorners)
{
  GP<GBitmap> input = GBitmap::create(2, 2, 0);
  input->set_grays(256);
  (*input)[0][0] = 0;
  (*input)[0][1] = 64;
  (*input)[1][0] = 128;
  (*input)[1][1] = 255;

  GP<GBitmapScaler> scaler = GBitmapScaler::create(2, 2, 4, 4);
  GRect provided(0, 0, 2, 2);
  GRect desired(0, 0, 4, 4);
  GP<GBitmap> output = GBitmap::create();
  scaler->scale(provided, *input, desired, *output);

  EXPECT_EQ(4u, output->rows());
  EXPECT_EQ(4u, output->columns());
  EXPECT_EQ(0, (*output)[0][0]);
  EXPECT_EQ(255, (*output)[3][3]);
}

TEST(GScalerTest, GetInputRectForSubsetIsWithinBounds)
{
  GP<GBitmapScaler> scaler = GBitmapScaler::create(100, 80, 50, 40);
  GRect desired(10, 5, 20, 10);
  GRect required;
  scaler->get_input_rect(desired, required);

  EXPECT_GE(required.xmin, 0);
  EXPECT_GE(required.ymin, 0);
  EXPECT_LE(required.xmax, 100);
  EXPECT_LE(required.ymax, 80);
  EXPECT_GT(required.width(), 0);
  EXPECT_GT(required.height(), 0);
}

TEST(GScalerTest, PixmapScalerRejectsTooSmallProvidedInput)
{
  GP<GPixmap> input = GPixmap::create(4, 4);
  GP<GPixmapScaler> scaler = GPixmapScaler::create(4, 4, 8, 8);

  GRect provided(0, 0, 2, 2);
  GRect desired(0, 0, 8, 8);
  GP<GPixmap> output = GPixmap::create();

  EXPECT_THROW(scaler->scale(provided, *input, desired, *output), GException);
}
