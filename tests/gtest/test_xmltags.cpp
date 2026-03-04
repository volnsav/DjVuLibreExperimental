#include <gtest/gtest.h>

#include "ByteStream.h"
#include "XMLTags.h"

TEST(XmlTagsTest, ParseNestedTagsAndAttributes)
{
  GP<ByteStream> bs = ByteStream::create();
  const char xml[] =
    "<ROOT mode=\"test\">"
    "<CHILD key=\"A\">value</CHILD>"
    "<CHILD key=\"B\"/>"
    "</ROOT>";
  ASSERT_EQ(sizeof(xml) - 1, bs->write(xml, sizeof(xml) - 1));
  ASSERT_EQ(0, bs->seek(0, SEEK_SET));

  GP<lt_XMLTags> tags = lt_XMLTags::create(bs);
  ASSERT_TRUE(tags != 0);
  EXPECT_STREQ("ROOT", (const char *)tags->get_name());

  GPosition mode_pos = tags->get_args().contains("mode");
  ASSERT_TRUE((bool)mode_pos);
  EXPECT_STREQ("test", (const char *)tags->get_args()[mode_pos]);

  GPList<lt_XMLTags> children = tags->get_Tags("CHILD");
  EXPECT_EQ(2, children.size());
  ASSERT_GT(children.size(), 0);
  EXPECT_STREQ("CHILD", (const char *)children[children]->get_name());
}

TEST(XmlTagsTest, ParseValuesDecodesEscapedEntities)
{
  GMap<GUTF8String, GUTF8String> args;
  lt_XMLTags::ParseValues("text=\"A&amp;B\" value=\"10\"", args, true);

  GPosition text_pos = args.contains("text");
  GPosition value_pos = args.contains("value");
  ASSERT_TRUE((bool)text_pos);
  ASSERT_TRUE((bool)value_pos);
  EXPECT_STREQ("A&B", (const char *)args[text_pos]);
  EXPECT_STREQ("10", (const char *)args[value_pos]);
}

TEST(XmlTagsTest, WriteProducesTagOutput)
{
  GP<lt_XMLTags> root = lt_XMLTags::create("ROOT mode=\"x\"");
  GP<lt_XMLTags> child = lt_XMLTags::create("CHILD id=\"1\"");
  child->addraw("payload");
  root->addtag(child);

  GP<ByteStream> out = ByteStream::create();
  root->write(*out, true);
  ASSERT_EQ(0, out->seek(0, SEEK_SET));

  char buffer[256] = {0};
  size_t n = out->read(buffer, sizeof(buffer) - 1);
  buffer[n] = 0;

  GUTF8String text(buffer);
  EXPECT_GE(text.search("ROOT"), 0);
  EXPECT_GE(text.search("CHILD"), 0);
  EXPECT_GE(text.search("payload"), 0);
}
