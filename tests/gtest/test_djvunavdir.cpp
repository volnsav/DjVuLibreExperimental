#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DjVuNavDir.h"
#include "GURL.h"

TEST(DjVuNavDirTest, InsertLookupAndUrlMapping)
{
  const GURL base = GURL::UTF8("file:///tmp/book/index.djvu");
  GP<DjVuNavDir> dir = DjVuNavDir::create(base);

  dir->insert_page(-1, "p1.djvu");
  dir->insert_page(-1, "p2.djvu");
  dir->insert_page(-1, "p3.djvu");

  EXPECT_EQ(3, dir->get_pages_num());
  EXPECT_EQ(0, dir->name_to_page("p1.djvu"));
  EXPECT_EQ(1, dir->name_to_page("p2.djvu"));
  EXPECT_EQ(2, dir->name_to_page("p3.djvu"));
  EXPECT_EQ(-1, dir->name_to_page("missing.djvu"));

  const GURL p2 = dir->page_to_url(1);
  EXPECT_EQ(1, dir->url_to_page(p2));
  EXPECT_STREQ("p2.djvu", dir->page_to_name(1));
}

TEST(DjVuNavDirTest, EncodeDecodeRoundtripKeepsOrder)
{
  const GURL base = GURL::UTF8("file:///tmp/book/index.djvu");
  GP<DjVuNavDir> src = DjVuNavDir::create(base);
  src->insert_page(-1, "a.djvu");
  src->insert_page(-1, "b.djvu");
  src->insert_page(-1, "c.djvu");

  GP<ByteStream> bs = ByteStream::create();
  src->encode(*bs);

  ASSERT_EQ(0, bs->seek(0));
  GP<DjVuNavDir> decoded = DjVuNavDir::create(*bs, base);

  EXPECT_EQ(3, decoded->get_pages_num());
  EXPECT_STREQ("a.djvu", decoded->page_to_name(0));
  EXPECT_STREQ("b.djvu", decoded->page_to_name(1));
  EXPECT_STREQ("c.djvu", decoded->page_to_name(2));
}

TEST(DjVuNavDirTest, DecodeRemovesDuplicateNames)
{
  const char payload[] = "x.djvu\nx.djvu\ny.djvu\n";
  const GURL base = GURL::UTF8("file:///tmp/book/index.djvu");
  GP<ByteStream> bs = ByteStream::create();

  bs->writall(payload, sizeof(payload) - 1);
  ASSERT_EQ(0, bs->seek(0));

  GP<DjVuNavDir> decoded = DjVuNavDir::create(*bs, base);
  EXPECT_EQ(2, decoded->get_pages_num());
  EXPECT_EQ(0, decoded->name_to_page("x.djvu"));
  EXPECT_EQ(1, decoded->name_to_page("y.djvu"));
}

TEST(DjVuNavDirTest, InvalidPageIndicesThrow)
{
  const GURL base = GURL::UTF8("file:///tmp/book/index.djvu");
  GP<DjVuNavDir> dir = DjVuNavDir::create(base);
  dir->insert_page(-1, "only.djvu");

  EXPECT_ANY_THROW(dir->page_to_name(-1));
  EXPECT_ANY_THROW(dir->page_to_name(1));
  EXPECT_ANY_THROW(dir->page_to_url(1));
}
