#include <gtest/gtest.h>

#include "GURL.h"

TEST(GUrlEqualityArgsTest, EqualityComparesArgumentsAndNormalizesSingleSlash)
{
  GURL::UTF8 a("http://example.com/p?a=1#x");
  GURL::UTF8 b("http://example.com/p/?a=1#x");
  GURL::UTF8 c("http://example.com/p?a=2#x");

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
}

TEST(GUrlEqualityArgsTest, EncodeDecodeReservedHandlesPercentAndSpaces)
{
  const GUTF8String raw("a b%20c#d");
  const GUTF8String encoded = GURL::encode_reserved(raw);
  EXPECT_STREQ("a%20b%2520c%23d", encoded);
  EXPECT_STREQ(raw, GURL::decode_reserved(encoded));
}

TEST(GUrlEqualityArgsTest, ProtocolExtractionForInvalidAndValidUrls)
{
  GURL::UTF8 valid("https://example.com/a");
  EXPECT_STREQ("https", valid.protocol());

  GURL::UTF8 ftp("ftp://example.com/file.bin");
  EXPECT_STREQ("ftp", ftp.protocol());
}
