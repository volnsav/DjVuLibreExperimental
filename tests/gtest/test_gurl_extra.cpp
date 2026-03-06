#include <gtest/gtest.h>

#include "GURL.h"

TEST(GUrlExtraTest, EncodeDecodeReservedRoundtrip)
{
  const GUTF8String raw("folder name/file#1?.djvu");
  const GUTF8String encoded = GURL::encode_reserved(raw);
  EXPECT_STREQ("folder%20name/file%231%3F.djvu", encoded);
  EXPECT_STREQ(raw, GURL::decode_reserved(encoded));
}

TEST(GUrlExtraTest, PathnameDropsQueryAndHash)
{
  GURL::UTF8 url("https://example.com/a/b/file.djvu?x=1#frag");
  EXPECT_STREQ("/a/b/file.djvu", url.pathname());
}

TEST(GUrlExtraTest, CgiParsingSupportsAmpAndSemicolonSeparators)
{
  GURL::UTF8 url("http://example.com/page?a=1;b=2&c");
  EXPECT_EQ(3, url.cgi_arguments());
  EXPECT_STREQ("a", url.cgi_name(0));
  EXPECT_STREQ("1", url.cgi_value(0));
  EXPECT_STREQ("b", url.cgi_name(1));
  EXPECT_STREQ("2", url.cgi_value(1));
  EXPECT_STREQ("c", url.cgi_name(2));
  EXPECT_STREQ("", url.cgi_value(2));
}

TEST(GUrlExtraTest, DjvuArgumentsAreTrackedSeparately)
{
  GURL::UTF8 url("http://example.com/doc.djvu?foo=bar");
  url.add_djvu_cgi_argument("page", "3");
  url.add_djvu_cgi_argument("zoom", "150");

  EXPECT_EQ(4, url.cgi_arguments());
  EXPECT_STREQ("DJVUOPTS", url.cgi_name(1));
  EXPECT_EQ(2, url.djvu_cgi_arguments());
  EXPECT_STREQ("page", url.djvu_cgi_name(0));
  EXPECT_STREQ("3", url.djvu_cgi_value(0));
  EXPECT_STREQ("zoom", url.djvu_cgi_name(1));
  EXPECT_STREQ("150", url.djvu_cgi_value(1));
}

TEST(GUrlExtraTest, ClearDjvuArgumentsKeepsRegularCgi)
{
  GURL::UTF8 url("http://example.com/doc.djvu?foo=bar");
  url.add_djvu_cgi_argument("page", "3");
  url.add_djvu_cgi_argument("zoom", "150");
  url.clear_djvu_cgi_arguments();

  EXPECT_EQ(1, url.cgi_arguments());
  EXPECT_STREQ("foo", url.cgi_name(0));
  EXPECT_STREQ("bar", url.cgi_value(0));
  EXPECT_STREQ("http://example.com/doc.djvu?foo=bar", url.get_string());
}

TEST(GUrlExtraTest, ClearCgiAndAllArguments)
{
  GURL::UTF8 with_cgi("http://example.com/doc.djvu?a=1&b=2");
  with_cgi.clear_cgi_arguments();
  EXPECT_STREQ("http://example.com/doc.djvu", with_cgi.get_string());

  GURL::UTF8 with_all("http://example.com/doc.djvu?a=1#frag");
  with_all.clear_all_arguments();
  EXPECT_STREQ("http://example.com/doc.djvu", with_all.get_string());
}

TEST(GUrlExtraTest, EqualityIgnoresSingleTrailingSlashBeforeArgs)
{
  GURL::UTF8 a("http://example.com/path");
  GURL::UTF8 b("http://example.com/path/");
  EXPECT_TRUE(a == b);
}

TEST(GUrlExtraTest, BaseForRootAndNestedPath)
{
  GURL::UTF8 root("http://example.com/file.djvu");
  EXPECT_STREQ("http://example.com/", root.base().get_string());

  GURL::UTF8 nested("http://example.com/dir/sub/file.djvu");
  EXPECT_STREQ("http://example.com/dir/sub/", nested.base().get_string());
}
