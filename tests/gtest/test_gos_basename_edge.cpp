#include <gtest/gtest.h>

#include "GOS.h"

TEST(GOSBasenameEdgeTest, HandlesDriveLikeInputsAndEmptyStrings)
{
  EXPECT_STREQ("", GOS::basename(""));
  EXPECT_STREQ("C:", GOS::basename("C:"));
  EXPECT_STREQ("C:\\", GOS::basename("C:\\"));
}

TEST(GOSBasenameEdgeTest, KeepsNameWhenSuffixDoesNotMatchExtension)
{
  EXPECT_STREQ("archive.tar.gz", GOS::basename("archive.tar.gz", ".zip"));
  EXPECT_STREQ("archive", GOS::basename("archive.tar.gz", "tar.gz"));
}

TEST(GOSBasenameEdgeTest, RemovesSuffixCaseInsensitivelyWithOrWithoutDot)
{
  EXPECT_STREQ("archive.tar", GOS::basename("archive.tar.GZ", ".gz"));
  EXPECT_STREQ("archive.tar", GOS::basename("archive.tar.GZ", "gz"));
}
