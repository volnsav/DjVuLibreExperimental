#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DjVuPalette.h"

TEST(DjVuPaletteTest, ComputePaletteMapsDistinctColors)
{
  GP<DjVuPalette> pal = DjVuPalette::create();
  pal->histogram_clear();
  pal->histogram_add(GPixel::RED, 10);
  pal->histogram_add(GPixel::BLUE, 5);

  (void)pal->compute_palette(2);
  EXPECT_EQ(2, pal->size());

  const int red_index = pal->color_to_index(GPixel::RED);
  const int blue_index = pal->color_to_index(GPixel::BLUE);
  EXPECT_NE(red_index, blue_index);

  GPixel red_back;
  GPixel blue_back;
  pal->index_to_color(red_index, red_back);
  pal->index_to_color(blue_index, blue_back);
  EXPECT_TRUE((red_back == GPixel::RED) || (red_back == GPixel::BLUE));
  EXPECT_TRUE((blue_back == GPixel::RED) || (blue_back == GPixel::BLUE));
}

TEST(DjVuPaletteTest, EncodeDecodePreservesPaletteAndColorData)
{
  GP<DjVuPalette> src = DjVuPalette::create();
  src->histogram_add(GPixel::RED, 7);
  src->histogram_add(GPixel::GREEN, 3);
  src->compute_palette(2);

  src->colordata.resize(0, 2);
  src->colordata[0] = (short)src->color_to_index(GPixel::RED);
  src->colordata[1] = (short)src->color_to_index(GPixel::GREEN);
  src->colordata[2] = src->colordata[0];

  GP<ByteStream> bs = ByteStream::create();
  src->encode(bs);

  bs->seek(0, SEEK_SET);
  GP<DjVuPalette> dst = DjVuPalette::create();
  dst->decode(bs);

  EXPECT_EQ(src->size(), dst->size());
  ASSERT_EQ(src->colordata.size(), dst->colordata.size());
  EXPECT_EQ(src->colordata[0], dst->colordata[0]);
  EXPECT_EQ(src->colordata[1], dst->colordata[1]);
  EXPECT_EQ(src->colordata[2], dst->colordata[2]);

  GPixel c0;
  dst->get_color(0, c0);
  EXPECT_TRUE((c0 == GPixel::RED) || (c0 == GPixel::GREEN));
}
