#include <gtest/gtest.h>

#include "ByteStream.h"
#include "XMLParser.h"
#include "XMLTags.h"

TEST(XMLParserTest, CreateAndLifecycleNoOpsDoNotThrow)
{
  GP<lt_XMLParser> parser = lt_XMLParser::create();
  ASSERT_TRUE(parser != 0);

  EXPECT_NO_THROW(parser->empty());
  EXPECT_NO_THROW(parser->save());
}

TEST(XMLParserTest, ParseWithoutSingleBodyThrows)
{
  GP<lt_XMLParser> parser = lt_XMLParser::create();
  ASSERT_TRUE(parser != 0);

  GP<ByteStream> bs = ByteStream::create();
  const char xml[] = "<ROOT/>";
  ASSERT_EQ(sizeof(xml) - 1, bs->write(xml, sizeof(xml) - 1));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  EXPECT_THROW(parser->parse(bs, nullptr), GException);
}

TEST(XMLParserTest, ParseWithMultipleBodyTagsIsAccepted)
{
  GP<lt_XMLParser> parser = lt_XMLParser::create();
  ASSERT_TRUE(parser != 0);

  GP<ByteStream> bs = ByteStream::create();
  const char xml[] = "<ROOT><BODY/><BODY/></ROOT>";
  ASSERT_EQ(sizeof(xml) - 1, bs->write(xml, sizeof(xml) - 1));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  GP<lt_XMLTags> tags = lt_XMLTags::create(bs);
  ASSERT_TRUE(tags != 0);

  EXPECT_NO_THROW(parser->parse(*tags, nullptr));
}
