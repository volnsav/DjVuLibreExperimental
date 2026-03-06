#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DjVuFile.h"
#include "DjVuFileCache.h"
#include "DjVuInfo.h"
#include "IFFByteStream.h"

namespace {

GP<ByteStream> MakeDjvuWithInfo(int w, int h)
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVU");

  iff->put_chunk("INFO");
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = w;
  info->height = h;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  info->encode(*iff->get_bytestream());
  iff->close_chunk();

  iff->close_chunk();
  bs->seek(0, SEEK_SET);
  return bs;
}

class TestDjVuFileCache : public DjVuFileCache
{
public:
  static GP<TestDjVuFileCache> create(int max_size)
  {
    return new TestDjVuFileCache(max_size);
  }

  int item_count()
  {
    GPList<Item> items = get_items();
    return items.size();
  }

  int added_count = 0;
  int deleted_count = 0;
  int cleared_count = 0;

protected:
  explicit TestDjVuFileCache(int max_size) : DjVuFileCache(max_size) {}

  void file_added(const GP<DjVuFile> &) override { ++added_count; }
  void file_deleted(const GP<DjVuFile> &) override { ++deleted_count; }
  void file_cleared(const GP<DjVuFile> &) override { ++cleared_count; }
};

}  // namespace

TEST(DjVuFileCacheTest, AddDuplicateAndDeleteFlow)
{
  GP<TestDjVuFileCache> cache = TestDjVuFileCache::create(8 * 1024 * 1024);
  ASSERT_TRUE(cache != 0);

  GP<DjVuFile> file1 = DjVuFile::create(MakeDjvuWithInfo(64, 32));
  GP<DjVuFile> file2 = DjVuFile::create(MakeDjvuWithInfo(80, 60));
  ASSERT_TRUE(file1 != 0);
  ASSERT_TRUE(file2 != 0);

  cache->add_file(file1);
  EXPECT_EQ(1, cache->item_count());
  EXPECT_EQ(1, cache->added_count);

  cache->add_file(file1);
  EXPECT_EQ(1, cache->item_count());
  EXPECT_EQ(1, cache->added_count);

  cache->add_file(file2);
  EXPECT_EQ(2, cache->item_count());
  EXPECT_EQ(2, cache->added_count);

  cache->del_file(file1);
  EXPECT_EQ(1, cache->item_count());
  EXPECT_EQ(1, cache->deleted_count);

  cache->del_file(file1);
  EXPECT_EQ(1, cache->item_count());
  EXPECT_EQ(1, cache->deleted_count);
}

TEST(DjVuFileCacheTest, TooSmallMaxSizeRejectsFile)
{
  GP<TestDjVuFileCache> cache = TestDjVuFileCache::create(1);
  ASSERT_TRUE(cache != 0);

  GP<DjVuFile> file = DjVuFile::create(MakeDjvuWithInfo(64, 32));
  ASSERT_TRUE(file != 0);

  cache->add_file(file);
  EXPECT_EQ(0, cache->item_count());
  EXPECT_EQ(0, cache->added_count);
}

TEST(DjVuFileCacheTest, DisablePreventsAddUntilReenabled)
{
  GP<TestDjVuFileCache> cache = TestDjVuFileCache::create(8 * 1024 * 1024);
  ASSERT_TRUE(cache != 0);

  GP<DjVuFile> file = DjVuFile::create(MakeDjvuWithInfo(64, 32));
  ASSERT_TRUE(file != 0);

  cache->enable(false);
  EXPECT_FALSE(cache->is_enabled());
  cache->add_file(file);
  EXPECT_EQ(0, cache->item_count());

  cache->enable(true);
  EXPECT_TRUE(cache->is_enabled());
  cache->add_file(file);
  EXPECT_EQ(1, cache->item_count());
}

TEST(DjVuFileCacheTest, ShrinkMaxSizeClearsItems)
{
  GP<TestDjVuFileCache> cache = TestDjVuFileCache::create(8 * 1024 * 1024);
  ASSERT_TRUE(cache != 0);

  GP<DjVuFile> file1 = DjVuFile::create(MakeDjvuWithInfo(64, 32));
  GP<DjVuFile> file2 = DjVuFile::create(MakeDjvuWithInfo(80, 60));
  ASSERT_TRUE(file1 != 0);
  ASSERT_TRUE(file2 != 0);

  cache->add_file(file1);
  cache->add_file(file2);
  ASSERT_EQ(2, cache->item_count());

  cache->set_max_size(1);
  EXPECT_EQ(0, cache->item_count());
}

TEST(DjVuFileCacheTest, ClearDropsAllItems)
{
  GP<TestDjVuFileCache> cache = TestDjVuFileCache::create(8 * 1024 * 1024);
  ASSERT_TRUE(cache != 0);

  GP<DjVuFile> file1 = DjVuFile::create(MakeDjvuWithInfo(64, 32));
  GP<DjVuFile> file2 = DjVuFile::create(MakeDjvuWithInfo(80, 60));
  ASSERT_TRUE(file1 != 0);
  ASSERT_TRUE(file2 != 0);

  cache->add_file(file1);
  cache->add_file(file2);
  ASSERT_EQ(2, cache->item_count());

  cache->clear();
  EXPECT_EQ(0, cache->item_count());
  EXPECT_EQ(0, cache->cleared_count);
}

TEST(DjVuFileCacheTest, DisableAndEnablePreserveConfiguredMaxSize)
{
  GP<TestDjVuFileCache> cache = TestDjVuFileCache::create(123456);
  ASSERT_TRUE(cache != 0);
  EXPECT_EQ(123456, cache->get_max_size());

  cache->enable(false);
  EXPECT_FALSE(cache->is_enabled());
  EXPECT_EQ(123456, cache->get_max_size());

  cache->set_max_size(654321);
  EXPECT_EQ(654321, cache->get_max_size());
  cache->enable(true);
  EXPECT_TRUE(cache->is_enabled());
  EXPECT_EQ(654321, cache->get_max_size());
}
