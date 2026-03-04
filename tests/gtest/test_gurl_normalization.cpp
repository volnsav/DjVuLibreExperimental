#include <gtest/gtest.h>

#include "GURL.h"

TEST(GUrlNormalizationTest, BeautifyCollapsesDotAndDoubleSlashes)
{
  GURL::UTF8 url("http://example.com/a//b/./c/../d.djvu");
  EXPECT_STREQ("http://example.com/a/b/d.djvu", url.get_string());
}

TEST(GUrlNormalizationTest, HashArgumentIsDecoded)
{
  GURL::UTF8 url("http://example.com/p#file%201%23x");
  EXPECT_STREQ("file 1#x", url.hash_argument());
}

TEST(GUrlNormalizationTest, NameFnameAndExtensionForNoExtensionFile)
{
  GURL::UTF8 url("https://example.com/path/readme");
  EXPECT_STREQ("readme", url.name());
  EXPECT_STREQ("readme", url.fname());
  EXPECT_STREQ("", url.extension());
}
