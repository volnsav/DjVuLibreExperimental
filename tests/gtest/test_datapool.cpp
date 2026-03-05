#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "ByteStream.h"
#include "DataPool.h"
#include "GException.h"
#include "GURL.h"

namespace {

std::filesystem::path MakeTempDataPoolPath(const char *suffix)
{
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("djvu_gtest_datapool_" + std::to_string(now) + "_" + suffix + ".bin");
}

std::filesystem::path WriteTempDataFile(const std::string &payload, const char *suffix)
{
  const std::filesystem::path path = MakeTempDataPoolPath(suffix);
  std::ofstream os(path, std::ios::binary | std::ios::trunc);
  os.write(payload.data(), static_cast<std::streamsize>(payload.size()));
  os.close();
  return path;
}

}  // namespace

TEST(DataPoolTest, SequentialAddGetAndLength)
{
  GP<DataPool> pool = DataPool::create();
  ASSERT_TRUE(pool != 0);

  const char payload[] = "abcdef";
  pool->add_data(payload, 6);

  EXPECT_TRUE(pool->has_data(0, 6));
  EXPECT_FALSE(pool->is_eof());

  char out[8] = {0};
  EXPECT_EQ(6, pool->get_data(out, 0, 6));
  EXPECT_STREQ("abcdef", out);

  pool->set_eof();
  EXPECT_TRUE(pool->is_eof());
  EXPECT_EQ(6, pool->get_length());
}

TEST(DataPoolTest, SlaveRangeReadsSubset)
{
  GP<DataPool> master = DataPool::create();
  const char payload[] = "0123456789";
  master->add_data(payload, 10);
  master->set_eof();

  GP<DataPool> slave = DataPool::create(master, 3, 4);
  EXPECT_EQ(4, slave->get_length());

  char out[8] = {0};
  EXPECT_EQ(4, slave->get_data(out, 0, 10));
  EXPECT_STREQ("3456", out);
}

TEST(DataPoolTest, StopMakesFurtherReadsFail)
{
  GP<DataPool> pool = DataPool::create();
  const char payload[] = "xyz";
  pool->add_data(payload, 3);
  pool->set_eof();
  pool->stop(false);

  char out[4] = {0};
  try
  {
    (void)pool->get_data(out, 0, 1);
    FAIL() << "Expected GException";
  }
  catch (const GException &ex)
  {
    EXPECT_EQ(0, ex.cmp_cause(DataPool::Stop));
  }
}

TEST(DataPoolTest, FileBackedPoolClampsRequestedRange)
{
  const std::string payload = "0123456789";
  const std::filesystem::path path = WriteTempDataFile(payload, "range");
  const GURL url = GURL::Filename::UTF8(path.string().c_str());

  GP<DataPool> tail = DataPool::create(url, 8, 100);
  ASSERT_EQ(2, tail->get_length());
  char out[8] = {0};
  EXPECT_EQ(2, tail->get_data(out, 0, 8));
  EXPECT_EQ("89", std::string(out, 2));

  GP<DataPool> empty = DataPool::create(url, 100, 1);
  EXPECT_EQ(0, empty->get_length());
  EXPECT_EQ(0, empty->get_data(out, 0, 1));

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(DataPoolTest, StaticLoadFileDetachesPoolFromFilesystem)
{
  const std::string payload = "payload-from-file";
  const std::filesystem::path path = WriteTempDataFile(payload, "load");
  const GURL url = GURL::Filename::UTF8(path.string().c_str());

  GP<DataPool> pool = DataPool::create(url, 0, -1);
  ASSERT_TRUE(pool != 0);

  DataPool::load_file(url);

  std::error_code ec;
  std::filesystem::remove(path, ec);

  char out[64] = {0};
  ASSERT_EQ((int)payload.size(), pool->get_data(out, 0, (int)payload.size()));
  EXPECT_EQ(payload, std::string(out, payload.size()));
}

TEST(DataPoolTest, RandomOffsetAddDataLeavesGapUnavailable)
{
  GP<DataPool> pool = DataPool::create();
  ASSERT_TRUE(pool != 0);

  const char value = 'Z';
  pool->add_data(&value, 3, 1);
  pool->set_eof();

  char out[8] = {0};
  EXPECT_TRUE(pool->has_data(3, 1));
  ASSERT_EQ(0, pool->get_data(out, 3, 1));
  EXPECT_THROW(pool->get_data(out, 0, 1), GException);
}

TEST(DataPoolTest, NegativeReadSizeThrows)
{
  GP<DataPool> pool = DataPool::create();
  const char payload[] = "abc";
  pool->add_data(payload, 3);
  pool->set_eof();

  char out[8] = {0};
  EXPECT_THROW(pool->get_data(out, 0, -1), GException);
}

TEST(DataPoolTest, ConnectRejectsNegativeStartAndSecondConnect)
{
  GP<DataPool> master = DataPool::create();
  GP<DataPool> slave = DataPool::create();
  ASSERT_TRUE(master != 0);
  ASSERT_TRUE(slave != 0);

  EXPECT_THROW(slave->connect(master, -1, 1), GException);
  EXPECT_NO_THROW(slave->connect(master, 0, 1));
  EXPECT_THROW(slave->connect(master, 0, 1), GException);
}

TEST(DataPoolTest, StopOnlyBlockedStillAllowsAvailableReads)
{
  GP<DataPool> pool = DataPool::create();
  ASSERT_TRUE(pool != 0);
  const char payload[] = "a";
  pool->add_data(payload, 1);
  pool->stop(true);

  char out[4] = {0};
  EXPECT_EQ(1, pool->get_data(out, 0, 1));
  EXPECT_EQ('a', out[0]);
  EXPECT_THROW(pool->get_data(out, 1, 1), GException);
}
