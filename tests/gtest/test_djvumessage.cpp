#include <gtest/gtest.h>

#include "DjVuGlobal.h"
#include "DjVuMessage.h"

namespace DJVU {
void DjVuMessageLookUpUTF8(char *msg_buffer, unsigned int buffer_size, const char *message);
void DjVuMessageLookUpNative(char *msg_buffer, unsigned int buffer_size, const char *message);
}

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

TEST(DjVuMessageTest, CApiLookupFunctionsHandleSmallAndLargeBuffers)
{
  char utf8_small[4] = {'x', 'x', 'x', 0};
  DJVU::DjVuMessageLookUpUTF8(utf8_small, sizeof(utf8_small), "plain text");
  EXPECT_STREQ("", utf8_small);

  char utf8_large[32] = {0};
  DJVU::DjVuMessageLookUpUTF8(utf8_large, sizeof(utf8_large), "plain text");
  EXPECT_STREQ("plain text", utf8_large);

  char native_small[4] = {'x', 'x', 'x', 0};
  DJVU::DjVuMessageLookUpNative(native_small, sizeof(native_small), "plain text");
  EXPECT_STREQ("", native_small);

  char native_large[32] = {0};
  DJVU::DjVuMessageLookUpNative(native_large, sizeof(native_large), "plain text");
  EXPECT_STREQ("plain text", native_large);
}
