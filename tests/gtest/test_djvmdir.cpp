#include <gtest/gtest.h>

#include "DjVmDir.h"

TEST(DjVmDirTest, InsertLookupAndCounts)
{
  GP<DjVmDir> dir = DjVmDir::create();
  ASSERT_TRUE(dir != 0);

  GP<DjVmDir::File> include_file = DjVmDir::File::create(
    "shared.inc", "shared.inc", "shared", DjVmDir::File::INCLUDE);
  GP<DjVmDir::File> page_file = DjVmDir::File::create(
    "page01.djvu", "page01.djvu", "Page 1", DjVmDir::File::PAGE);

  EXPECT_EQ(0, dir->insert_file(include_file));
  EXPECT_EQ(1, dir->insert_file(page_file));
  EXPECT_EQ(2, dir->get_files_num());
  EXPECT_EQ(1, dir->get_pages_num());

  GP<DjVmDir::File> by_id = dir->id_to_file("page01.djvu");
  ASSERT_TRUE(by_id != 0);
  EXPECT_TRUE(by_id->is_page());
  EXPECT_STREQ("Page 1", (const char *)by_id->get_title());

  GP<DjVmDir::File> by_name = dir->name_to_file("page01.djvu");
  ASSERT_TRUE(by_name != 0);
  EXPECT_STREQ("page01.djvu", (const char *)by_name->get_load_name());

  GP<DjVmDir::File> page0 = dir->page_to_file(0);
  ASSERT_TRUE(page0 != 0);
  EXPECT_STREQ("page01.djvu", (const char *)page0->get_load_name());
}

TEST(DjVmDirTest, DuplicateIdOrNameThrows)
{
  GP<DjVmDir> dir = DjVmDir::create();
  ASSERT_TRUE(dir != 0);

  GP<DjVmDir::File> f1 = DjVmDir::File::create(
    "same.djvu", "name1.djvu", "A", DjVmDir::File::PAGE);
  GP<DjVmDir::File> f2 = DjVmDir::File::create(
    "same.djvu", "name2.djvu", "B", DjVmDir::File::INCLUDE);
  GP<DjVmDir::File> f3 = DjVmDir::File::create(
    "id3.djvu", "name1.djvu", "C", DjVmDir::File::INCLUDE);

  dir->insert_file(f1);
  EXPECT_THROW(dir->insert_file(f2), GException);
  EXPECT_THROW(dir->insert_file(f3), GException);
}

TEST(DjVmDirTest, RenameTitleAndDeleteUpdateMappings)
{
  GP<DjVmDir> dir = DjVmDir::create();
  ASSERT_TRUE(dir != 0);

  GP<DjVmDir::File> page_file = DjVmDir::File::create(
    "pageA.djvu", "pageA.djvu", "TitleA", DjVmDir::File::PAGE);
  dir->insert_file(page_file);

  dir->set_file_name("pageA.djvu", "pageRenamed.djvu");
  dir->set_file_title("pageA.djvu", "RenamedTitle");

  EXPECT_TRUE(dir->name_to_file("pageA.djvu") == 0);
  GP<DjVmDir::File> renamed = dir->name_to_file("pageRenamed.djvu");
  ASSERT_TRUE(renamed != 0);
  EXPECT_STREQ("RenamedTitle", (const char *)renamed->get_title());

  dir->delete_file("pageA.djvu");
  EXPECT_EQ(0, dir->get_files_num());
  EXPECT_EQ(0, dir->get_pages_num());
}
