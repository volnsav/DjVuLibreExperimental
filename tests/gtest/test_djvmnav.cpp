#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DjVmNav.h"

TEST(DjVmNavTest, AppendGetAndCount)
{
  GP<DjVmNav> nav = DjVmNav::create();
  ASSERT_TRUE(nav != 0);

  nav->append(DjVmNav::DjVuBookMark::create(0, "Intro", "#intro"));
  nav->append(DjVmNav::DjVuBookMark::create(0, "Chapter1", "#ch1"));
  EXPECT_EQ(2, nav->getBookMarkCount());

  GP<DjVmNav::DjVuBookMark> bm;
  ASSERT_TRUE(nav->getBookMark(bm, 1));
  EXPECT_STREQ("Chapter1", (const char *)bm->displayname);
  EXPECT_STREQ("#ch1", (const char *)bm->url);

  EXPECT_FALSE(nav->getBookMark(bm, 10));
}

TEST(DjVmNavTest, EncodeDecodeRoundtrip)
{
  GP<DjVmNav> src = DjVmNav::create();
  src->append(DjVmNav::DjVuBookMark::create(1, "Root", "root.djvu"));
  src->append(DjVmNav::DjVuBookMark::create(0, "Leaf", "leaf.djvu"));

  GP<ByteStream> bs = ByteStream::create();
  src->encode(bs);
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  GP<DjVmNav> dst = DjVmNav::create();
  dst->decode(bs);
  ASSERT_EQ(2, dst->getBookMarkCount());

  GP<DjVmNav::DjVuBookMark> b0;
  GP<DjVmNav::DjVuBookMark> b1;
  ASSERT_TRUE(dst->getBookMark(b0, 0));
  ASSERT_TRUE(dst->getBookMark(b1, 1));
  EXPECT_EQ(1, b0->count);
  EXPECT_STREQ("Root", (const char *)b0->displayname);
  EXPECT_STREQ("root.djvu", (const char *)b0->url);
  EXPECT_EQ(0, b1->count);
  EXPECT_STREQ("Leaf", (const char *)b1->displayname);
  EXPECT_STREQ("leaf.djvu", (const char *)b1->url);
}

TEST(DjVmNavTest, GetTreeReturnsExpectedSizeForSimpleTree)
{
  GP<DjVmNav> nav = DjVmNav::create();
  ASSERT_TRUE(nav != 0);

  int counts[] = {2, 0, 0};
  EXPECT_EQ(2, nav->get_tree(0, counts, 3));
}
