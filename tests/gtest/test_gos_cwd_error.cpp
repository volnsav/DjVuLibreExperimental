#include <gtest/gtest.h>

#include <filesystem>

#include "GException.h"
#include "GOS.h"

TEST(GOSCwdErrorTest, CwdThrowsForNonExistingDirectory)
{
  const GUTF8String old_cwd = GOS::cwd();

  const std::filesystem::path missing =
      std::filesystem::temp_directory_path() / "djvu_gtest_missing_cwd_7329";
  std::filesystem::remove_all(missing);

  EXPECT_THROW(
      {
        GOS::cwd(missing.u8string().c_str());
      },
      GException);

  EXPECT_STREQ(old_cwd, GOS::cwd());
}

TEST(GOSCwdErrorTest, EmptyEnvNameReturnsEmpty)
{
  const GUTF8String v = GOS::getenv("");
  EXPECT_EQ(0, v.length());
}
