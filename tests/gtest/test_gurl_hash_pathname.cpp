#include <gtest/gtest.h>

#include "GURL.h"

TEST(GUrlHashPathnameTest, HashArgumentIncludesQuestionTailAfterHash)
{
  GURL::UTF8 url("http://example.com/p#frag?x=1");
  EXPECT_STREQ("frag?x=1", url.hash_argument());
}

TEST(GUrlHashPathnameTest, PathnameForRemoteUrlStripsHashAndQuery)
{
  GURL::UTF8 url("http://example.com/a/b/file.djvu?x=1#h");
  EXPECT_STREQ("/a/b/file.djvu", url.pathname());
}

TEST(GUrlHashPathnameTest, SetHashArgumentEncodesReservedCharacters)
{
  GURL::UTF8 url("http://example.com/p");
  url.set_hash_argument("a b#c");
  EXPECT_STREQ("http://example.com/p#a%20b%23c", url.get_string());
}
