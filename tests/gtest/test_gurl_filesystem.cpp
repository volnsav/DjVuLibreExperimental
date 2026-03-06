#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

#include "GURL.h"

namespace
{
std::filesystem::path MakeTempPath(const char *suffix)
{
  const auto ticks =
      std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("djvu_gurl_" + std::string(suffix) + "_" + std::to_string(ticks));
}
} // namespace

TEST(GUrlFilesystemTest, LocalPathClassificationAndDeleteFile)
{
  const std::filesystem::path root = MakeTempPath("classify");
  std::filesystem::create_directories(root);
  const std::filesystem::path file = root / "sample.txt";
  {
    std::ofstream out(file.string().c_str(), std::ios::binary);
    out << "djvu";
  }

  GURL::Filename::Native dir_url(root.string().c_str());
  GURL::Filename::Native file_url(file.string().c_str());

  EXPECT_TRUE(dir_url.is_local_file_url());
  EXPECT_TRUE(file_url.is_local_file_url());
  EXPECT_TRUE(dir_url.is_local_path());
  EXPECT_TRUE(file_url.is_local_path());
  EXPECT_TRUE(dir_url.is_dir());
  EXPECT_FALSE(dir_url.is_file());
  EXPECT_TRUE(file_url.is_file());
  EXPECT_FALSE(file_url.is_dir());

  EXPECT_GE(file_url.deletefile(), 0);
  EXPECT_FALSE(file_url.is_local_path());

  std::filesystem::remove_all(root);
}

TEST(GUrlFilesystemTest, RenameAndClearDirOnExistingDirectory)
{
  const std::filesystem::path root = MakeTempPath("ops");
  const std::filesystem::path nested = root / "a" / "b";
  std::filesystem::create_directories(nested);
  GURL::Filename::Native nested_url(nested.string().c_str());

  EXPECT_TRUE(nested_url.is_dir());

  const std::filesystem::path old_file = nested / "old.txt";
  {
    std::ofstream out(old_file.string().c_str(), std::ios::binary);
    out << "x";
  }
  const std::filesystem::path new_file = nested / "new.txt";
  GURL::Filename::Native old_url(old_file.string().c_str());
  GURL::Filename::Native new_url(new_file.string().c_str());

  EXPECT_EQ(0, old_url.renameto(new_url));
  EXPECT_FALSE(old_url.is_local_path());
  EXPECT_TRUE(new_url.is_file());

  const std::filesystem::path sub_dir = nested / "sub";
  std::filesystem::create_directories(sub_dir);
  {
    std::ofstream out((sub_dir / "k.txt").string().c_str(), std::ios::binary);
    out << "y";
  }

  EXPECT_EQ(0, nested_url.cleardir());
  ASSERT_TRUE(std::filesystem::exists(nested));
  ASSERT_TRUE(std::filesystem::is_directory(nested));
  EXPECT_TRUE(std::filesystem::is_empty(nested));

  std::filesystem::remove_all(root);
}
