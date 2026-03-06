#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "GURL.h"

TEST(GUrlRelativeTest, RelativeConstructorUsesCodebaseDirectory)
{
  GURL::UTF8 base("http://example.com/dir/");
  GURL relative(GUTF8String("file 1.djvu"), base);
  EXPECT_STREQ("http://example.com/dir/file%201.djvu", relative.get_string());
}

TEST(GUrlRelativeTest, AbsoluteConstructorArgumentBypassesCodebase)
{
  GURL::UTF8 base("http://example.com/dir/");
  GURL absolute(GUTF8String("https://other.net/a.djvu"), base);
  EXPECT_STREQ("https://other.net/a.djvu", absolute.get_string());
}

TEST(GUrlRelativeTest, ClearHashAndCgiArgumentsBehaveAsImplemented)
{
  GURL::UTF8 with_hash_then_query("http://example.com/p#frag?x=1");
  with_hash_then_query.clear_hash_argument();
  EXPECT_STREQ("http://example.com/p?x=1", with_hash_then_query.get_string());

  GURL::UTF8 with_query_and_hash("http://example.com/p?a=1#frag");
  with_query_and_hash.clear_cgi_arguments();
  EXPECT_STREQ("http://example.com/p", with_query_and_hash.get_string());
}

TEST(GUrlRelativeTest, FilenameNativeProducesLocalFileUrl)
{
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "djvu_gurl_filename_test";
  std::filesystem::create_directories(root);
  const std::filesystem::path file = root / "name with space.txt";
  {
    std::ofstream out(file.string().c_str(), std::ios::binary);
    out << "x";
  }

  GURL::Filename::Native file_url(file.string().c_str());
  EXPECT_TRUE(file_url.is_local_file_url());
  EXPECT_NE(-1, file_url.get_string().search("%20"));
  EXPECT_NE(-1, file_url.UTF8Filename().rsearch("name with space.txt"));

  std::filesystem::remove_all(root);
}
