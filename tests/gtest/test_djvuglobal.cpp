#include <gtest/gtest.h>

#include "DjVuGlobal.h"

TEST(DjVuGlobalTest, ProgramNameGetterWithNullKeepsCurrentValue)
{
  const char *set_value = djvu_programname("djvu_gtest_global");
  ASSERT_NE(nullptr, set_value);
  EXPECT_STREQ("djvu_gtest_global", set_value);

  const char *current = djvu_programname(nullptr);
  ASSERT_NE(nullptr, current);
  EXPECT_STREQ("djvu_gtest_global", current);
}

TEST(DjVuGlobalTest, PrintAndFormatHelpersAreCallable)
{
  EXPECT_NO_THROW({
    DjVuPrintErrorUTF8("%s", "gtest-error");
    DjVuPrintErrorNative("%s", "gtest-error-native");
    DjVuPrintMessageUTF8("%s", "gtest-message");
    DjVuPrintMessageNative("%s", "gtest-message-native");

    DjVuFormatErrorUTF8("plain %d", 1);
    DjVuFormatErrorNative("plain %d", 2);

    DjVuWriteError("plain text");
    DjVuWriteMessage("plain text");
  });
}
