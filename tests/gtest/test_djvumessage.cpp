#include <gtest/gtest.h>

#include "DjVuGlobal.h"
#include "DjVuMessage.h"

TEST(DjVuMessageTest, ProgramNameRoundtripViaCApi)
{
  const char *stored = djvu_programname("djvu_gtest_program");
  ASSERT_NE(nullptr, stored);
  EXPECT_STREQ("djvu_gtest_program", stored);
}

TEST(DjVuMessageTest, ProfilePathsListIsAvailable)
{
  DjVuMessage::use_language();
  GList<GURL> paths = DjVuMessage::GetProfilePaths();
  EXPECT_GT(paths.size(), 0);
}

TEST(DjVuMessageTest, LookupNativeForPlainTextReturnsValue)
{
  const GNativeString native = DjVuMessage::LookUpNative("plain text");
  EXPECT_STREQ("plain text", (const char *)native);
}

TEST(DjVuMessageTest, SetProgramNameAndCreateFullAreCallable)
{
  DjVuMessage::set_programname("djvu_gtest_binary");
  EXPECT_STREQ("djvu_gtest_binary", (const char *)DjVuMessage::programname());

  const DjVuMessageLite &full = DjVuMessage::create_full();
  const GUTF8String looked = full.LookUpUTF8("plain text");
  EXPECT_STREQ("plain text", (const char *)looked);
}
