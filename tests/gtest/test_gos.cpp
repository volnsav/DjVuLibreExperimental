#include <gtest/gtest.h>

#include <filesystem>

#include "GException.h"
#include "GOS.h"

TEST(GOSTest, BasenameExtractsLastComponent)
{
  EXPECT_STREQ("file.djvu", GOS::basename("C:\\temp\\file.djvu"));
  EXPECT_STREQ("file", GOS::basename("C:\\temp\\file.djvu", ".djvu"));
  EXPECT_STREQ("file", GOS::basename("C:\\temp\\file.DJVU", ".djvu"));
  EXPECT_STREQ("file", GOS::basename("C:\\temp\\file.djvu", "djvu"));
}

TEST(GOSTest, CwdReturnsNonEmptyAndCanRestoreAfterChange)
{
  const GUTF8String old_cwd = GOS::cwd();
  ASSERT_GT(old_cwd.length(), 0u);

  const std::filesystem::path tmp = std::filesystem::temp_directory_path() / "djvu_gtest_cwd";
  std::filesystem::create_directories(tmp);

  GUTF8String changed;
  ASSERT_NO_THROW({
    changed = GOS::cwd(tmp.u8string().c_str());
  });
  EXPECT_GT(changed.length(), 0u);
  EXPECT_GE(changed.rcontains("djvu_gtest_cwd"), 0);

  ASSERT_NO_THROW({
    GOS::cwd(old_cwd);
  });
}

TEST(GOSTest, GetEnvReturnsValueForExistingVariables)
{
  const GUTF8String path = GOS::getenv("PATH");
  EXPECT_GT(path.length(), 0u);

  const GUTF8String unlikely = GOS::getenv("DJVU_GTEST_UNLIKELY_ENV_NAME_12345");
  EXPECT_EQ(0, unlikely.length());
}

TEST(GOSTest, TicksAndSleepAdvanceTime)
{
  const unsigned long start = GOS::ticks();
  GOS::sleep(15);
  const unsigned long end = GOS::ticks();
  EXPECT_GE(end, start);
}
