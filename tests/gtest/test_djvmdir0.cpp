#include <gtest/gtest.h>

#include <string>

#include "ByteStream.h"
#include "DjVmDir0.h"
#include "GException.h"

TEST(DjVmDir0Test, EncodeDecodeRoundtripPreservesRecords)
{
  GP<DjVmDir0> dir = DjVmDir0::create();
  dir->add_file("p0001.djvu", true, 100, 200);
  dir->add_file("meta.txt", false, 350, 20);

  GP<ByteStream> bs = ByteStream::create();
  dir->encode(*bs);
  EXPECT_EQ(dir->get_size(), bs->tell());

  ASSERT_EQ(0, bs->seek(0, SEEK_SET));
  GP<DjVmDir0> decoded = DjVmDir0::create();
  decoded->decode(*bs);

  ASSERT_EQ(2, decoded->get_files_num());
  GP<DjVmDir0::FileRec> f0 = decoded->get_file(0);
  GP<DjVmDir0::FileRec> f1 = decoded->get_file(1);
  ASSERT_TRUE(f0 != 0);
  ASSERT_TRUE(f1 != 0);

  EXPECT_EQ("p0001.djvu", std::string((const char *)f0->name));
  EXPECT_TRUE(f0->iff_file);
  EXPECT_EQ(100, f0->offset);
  EXPECT_EQ(200, f0->size);

  EXPECT_EQ("meta.txt", std::string((const char *)f1->name));
  EXPECT_FALSE(f1->iff_file);
  EXPECT_EQ(350, f1->offset);
  EXPECT_EQ(20, f1->size);
}

TEST(DjVmDir0Test, AddFileWithSlashThrows)
{
  GP<DjVmDir0> dir = DjVmDir0::create();
  EXPECT_THROW((void)dir->add_file("bad/name", true, 0, 1), GException);
}

TEST(DjVmDir0Test, DuplicateNameKeepsBothIndexesAndLatestNameLookup)
{
  GP<DjVmDir0> dir = DjVmDir0::create();
  dir->add_file("dup", true, 10, 11);
  dir->add_file("dup", false, 20, 21);

  EXPECT_EQ(2, dir->get_files_num());
  ASSERT_TRUE(dir->get_file(0) != 0);
  ASSERT_TRUE(dir->get_file(1) != 0);
  EXPECT_EQ(10, dir->get_file(0)->offset);
  EXPECT_EQ(20, dir->get_file(1)->offset);
  EXPECT_EQ(20, dir->get_file("dup")->offset);
  EXPECT_TRUE(dir->get_file(5) == 0);
}
