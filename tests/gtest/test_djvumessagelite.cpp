#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DjVuMessageLite.h"

TEST(DjVuMessageLiteTest, PlainTextPassesThrough)
{
  DjVuMessageLite::create = DjVuMessageLite::create_lite;
  const GUTF8String input("plain text message");
  const GUTF8String output = DjVuMessageLite::LookUpUTF8(input);
  EXPECT_STREQ((const char *)input, (const char *)output);
}

TEST(DjVuMessageLiteTest, CanLoadCustomMessageFromByteStream)
{
  DjVuMessageLite::create = DjVuMessageLite::create_lite;

  GP<ByteStream> xml = ByteStream::create();
  const char payload[] =
    "<DJVMESSAGES><BODY>"
    "<MESSAGE name=\"unit.test.message\" value=\"resolved text\"/>"
    "</BODY></DJVMESSAGES>";
  ASSERT_EQ(sizeof(payload) - 1, xml->write(payload, sizeof(payload) - 1));
  ASSERT_EQ(0, xml->seek(0, SEEK_SET));

  DjVuMessageLite::AddByteStreamLater(xml);
  const GUTF8String output = DjVuMessageLite::LookUpUTF8("\003unit.test.message");
  EXPECT_STREQ("resolved text", (const char *)output);
}
