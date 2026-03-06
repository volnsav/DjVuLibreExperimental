#include <gtest/gtest.h>

#include "GBitmap.h"
#include "JB2Image.h"

TEST(JB2ImageTest, AddShapeBlitAndRenderBitmap)
{
  GP<JB2Image> image = JB2Image::create();
  ASSERT_TRUE(image != 0);
  image->set_dimension(5, 5);

  JB2Shape shape;
  shape.parent = -1;
  shape.bits = GBitmap::create(1, 1, 0);
  shape.bits->set_grays(2);
  (*shape.bits)[0][0] = 1;
  const int shapeno = image->add_shape(shape);

  JB2Blit blit;
  blit.left = 2;
  blit.bottom = 3;
  blit.shapeno = shapeno;
  image->add_blit(blit);

  GP<GBitmap> rendered = image->get_bitmap(1, 1);
  ASSERT_TRUE(rendered != 0);
  EXPECT_EQ(5u, rendered->rows());
  EXPECT_EQ(5u, rendered->columns());
  EXPECT_GT((*rendered)[3][2], 0);
}

TEST(JB2ImageTest, InvalidShapeIndexesThrow)
{
  GP<JB2Image> image = JB2Image::create();
  image->set_dimension(10, 10);

  JB2Shape bad_shape;
  bad_shape.parent = 999;
  bad_shape.bits = GBitmap::create(1, 1, 0);
  EXPECT_THROW(image->add_shape(bad_shape), GException);

  JB2Blit bad_blit;
  bad_blit.left = 0;
  bad_blit.bottom = 0;
  bad_blit.shapeno = 123;
  EXPECT_THROW(image->add_blit(bad_blit), GException);
}

TEST(JB2ImageTest, DictionaryInheritanceIsCounted)
{
  GP<JB2Dict> dict = JB2Dict::create();
  JB2Shape base;
  base.parent = -1;
  base.bits = GBitmap::create(1, 1, 0);
  dict->add_shape(base);

  GP<JB2Image> image = JB2Image::create();
  image->set_inherited_dict(dict);
  EXPECT_EQ(1, image->get_inherited_shape_count());
  EXPECT_EQ(1, image->get_shape_count());
}
