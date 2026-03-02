#include <gtest/gtest.h>

#include "GURL.h"

TEST(GUrlTest, LocalAndRemoteProtocols)
{
  GURL::UTF8 local_url("file:///tmp/sample.djvu");
  EXPECT_TRUE(local_url.is_local_file_url());
  EXPECT_TRUE(local_url.protocol() == "file");

  GURL::UTF8 remote_url("https://example.com/file%201.djvu");
  EXPECT_FALSE(remote_url.is_local_file_url());
  EXPECT_TRUE(remote_url.protocol() == "https");
  EXPECT_TRUE(remote_url.name() == "file%201.djvu");
  EXPECT_TRUE(remote_url.fname() == "file 1.djvu");
  EXPECT_TRUE(remote_url.extension() == "djvu");
}
