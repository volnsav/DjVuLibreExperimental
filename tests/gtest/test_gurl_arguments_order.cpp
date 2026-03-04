#include <gtest/gtest.h>

#include "GURL.h"

TEST(GUrlArgumentsOrderTest, CgiOrderIsPreserved)
{
  GURL::UTF8 url("http://example.com/p?b=2&a=1&c=3");
  ASSERT_EQ(3, url.cgi_arguments());
  EXPECT_STREQ("b", url.cgi_name(0));
  EXPECT_STREQ("2", url.cgi_value(0));
  EXPECT_STREQ("a", url.cgi_name(1));
  EXPECT_STREQ("1", url.cgi_value(1));
  EXPECT_STREQ("c", url.cgi_name(2));
  EXPECT_STREQ("3", url.cgi_value(2));
}

TEST(GUrlArgumentsOrderTest, SetHashArgumentKeepsCgiTail)
{
  GURL::UTF8 url("http://example.com/p?x=1&y=2");
  url.set_hash_argument("frag");
  EXPECT_STREQ("http://example.com/p#frag?x=1&y=2", url.get_string());
}

TEST(GUrlArgumentsOrderTest, ClearAllArgumentsRemovesHashAndCgi)
{
  GURL::UTF8 url("http://example.com/p#frag?x=1");
  url.clear_all_arguments();
  EXPECT_STREQ("http://example.com/p", url.get_string());
}
