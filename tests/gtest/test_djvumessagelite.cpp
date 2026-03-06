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

TEST(DjVuMessageLiteTest, ParameterFormattingAndNestedLookupsWork)
{
  DjVuMessageLite::create = DjVuMessageLite::create_lite;

  GP<ByteStream> xml = ByteStream::create();
  const char payload[] =
    "<DJVMESSAGES><BODY>"
    "<MESSAGE name=\"unit.inner\" value=\"inner-value\"/>"
    "<MESSAGE name=\"unit.format\" number=\"007\" value=\"n=%0!s! a=%1!04d! b=%2!s! c=%3!0.1f!\"/>"
    "<MESSAGE name=\"unit.outer\" value=\"outer[%1!s!]\"/>"
    "</BODY></DJVMESSAGES>";
  ASSERT_EQ(sizeof(payload) - 1, xml->write(payload, sizeof(payload) - 1));
  ASSERT_EQ(0, xml->seek(0, SEEK_SET));

  DjVuMessageLite::AddByteStreamLater(xml);

  const GUTF8String formatted =
      DjVuMessageLite::LookUpUTF8("\003unit.format\t17\ttext\t3.5");
  EXPECT_STREQ("n=007 a=0017 b=text c=3.5", (const char *)formatted);

  const GUTF8String nested =
      DjVuMessageLite::LookUpUTF8("\003unit.outer\v\003unit.inner");
  EXPECT_STREQ("outer[inner-value]", (const char *)nested);
}

TEST(DjVuMessageLiteTest, UnknownMessageFallsBackToDiagnosticText)
{
  DjVuMessageLite::create = DjVuMessageLite::create_lite;

  const GUTF8String output =
      DjVuMessageLite::LookUpUTF8("\003unit.unknown.message\tARG");
  EXPECT_NE(-1, output.search("Unrecognized"));
  EXPECT_NE(-1, output.search("unit.unknown.message"));
  EXPECT_NE(-1, output.search("ARG"));
}
