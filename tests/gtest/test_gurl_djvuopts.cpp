#include <gtest/gtest.h>

#include "GURL.h"

TEST(GUrlDjvuOptsTest, AddDjvuArgumentsInsertsSentinelAndValues)
{
  GURL::UTF8 url("http://example.com/doc.djvu?foo=bar");
  url.add_djvu_cgi_argument("page", "4");
  url.add_djvu_cgi_argument("mode", "color");

  EXPECT_EQ(2, url.djvu_cgi_arguments());
  EXPECT_STREQ("page", url.djvu_cgi_name(0));
  EXPECT_STREQ("4", url.djvu_cgi_value(0));
  EXPECT_STREQ("mode", url.djvu_cgi_name(1));
  EXPECT_STREQ("color", url.djvu_cgi_value(1));

  EXPECT_NE(-1, url.get_string().search("DJVUOPTS"));
}

TEST(GUrlDjvuOptsTest, AddDjvuArgumentWithoutValueCreatesEmptyValue)
{
  GURL::UTF8 url("http://example.com/doc.djvu");
  url.add_djvu_cgi_argument("cache");

  EXPECT_EQ(1, url.djvu_cgi_arguments());
  EXPECT_STREQ("cache", url.djvu_cgi_name(0));
  EXPECT_STREQ("", url.djvu_cgi_value(0));
}

TEST(GUrlDjvuOptsTest, ClearDjvuArgumentsKeepsRegularArgumentsAndHash)
{
  GURL::UTF8 url("http://example.com/doc.djvu?foo=bar#frag");
  url.add_djvu_cgi_argument("page", "2");
  url.add_djvu_cgi_argument("zoom", "150");

  url.clear_djvu_cgi_arguments();
  EXPECT_EQ(0, url.djvu_cgi_arguments());
  EXPECT_EQ(1, url.cgi_arguments());
  EXPECT_STREQ("foo", url.cgi_name(0));
  EXPECT_STREQ("bar#frag", url.cgi_value(0));
  EXPECT_EQ(-1, url.get_string().search("#frag"));
}
