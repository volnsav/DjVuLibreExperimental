#include <gtest/gtest.h>
#include "GURL.h"

TEST(GUrlTest, LocalAndRemoteProtocols)
{
  GURL::UTF8 local_url("file:///tmp/sample.djvu");
  EXPECT_TRUE(local_url.is_local_file_url());
  EXPECT_STREQ("file", local_url.protocol());

  GURL::UTF8 remote_url("https://example.com/file%201.djvu");
  EXPECT_FALSE(remote_url.is_local_file_url());
  EXPECT_STREQ("https", remote_url.protocol());
  EXPECT_STREQ("file%201.djvu", remote_url.name());
  EXPECT_STREQ("file 1.djvu", remote_url.fname());
  EXPECT_STREQ("djvu", remote_url.extension());
}

TEST(GUrlTest, Components)
{
    GURL::UTF8 url("http://user:pass@example.com:8080/path/to/file.html?query=1#fragment");
    EXPECT_STREQ("http", url.protocol());
    EXPECT_STREQ("/path/to/file.html", url.pathname());
    EXPECT_STREQ("file.html", url.name());
    EXPECT_STREQ("html", url.extension());
    EXPECT_STREQ("fragment", url.hash_argument());
}

TEST(GUrlTest, CgiArguments)
{
    GURL::UTF8 url("http://example.com/page?arg1=val1&arg2=val2");
    EXPECT_EQ(2, url.cgi_arguments());
    EXPECT_STREQ("arg1", url.cgi_name(0));
    EXPECT_STREQ("val1", url.cgi_value(0));
    EXPECT_STREQ("arg2", url.cgi_name(1));
    EXPECT_STREQ("val2", url.cgi_value(1));
}

TEST(GUrlTest, ClearArguments)
{
    GURL::UTF8 url("http://example.com/page?arg1=val1#hash");
    url.clear_all_arguments();
    EXPECT_STREQ("http://example.com/page", url.get_string());
}

TEST(GUrlTest, SetHashArgument)
{
    GURL::UTF8 url("http://example.com/page");
    url.set_hash_argument("new_hash");
    EXPECT_STREQ("http://example.com/page#new_hash", url.get_string());
}

TEST(GUrlTest, Base)
{
    GURL::UTF8 url("http://example.com/dir/file.html");
    GURL base = url.base();
    EXPECT_STREQ("http://example.com/dir/", base.get_string());
}

TEST(GUrlTest, EmptyUrl)
{
    GURL url;
    EXPECT_TRUE(url.is_empty());
}
