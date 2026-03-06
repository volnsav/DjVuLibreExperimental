#include <gtest/gtest.h>

#include "GException.h"

TEST(GExceptionTest, DefaultConstructorProvidesFallbackCause)
{
  GException ex;
  ASSERT_NE(nullptr, ex.get_cause());
  EXPECT_STREQ("Invalid exception", ex.get_cause());
  EXPECT_EQ(nullptr, ex.get_file());
  EXPECT_EQ(nullptr, ex.get_function());
  EXPECT_EQ(0, ex.get_line());
}

TEST(GExceptionTest, CauseComparisonIgnoresTextAfterTabOrNewline)
{
  GException ex("ByteStream::EndOfFile\twhile reading");
  EXPECT_EQ(0, ex.cmp_cause("ByteStream::EndOfFile"));
  EXPECT_NE(0, ex.cmp_cause("DataPool::Stop"));

  EXPECT_EQ(0, GException::cmp_cause("A\nextra", "A\tsuffix"));
  EXPECT_NE(0, GException::cmp_cause("A", "B"));
}

TEST(GExceptionTest, CopyAndAssignPreserveFields)
{
  GException src("custom-cause", "some_file.cpp", 123, "fn_name", GException::GAPPLICATION);
  GException copy(src);
  GException assigned;
  assigned = src;

  EXPECT_STREQ(src.get_cause(), copy.get_cause());
  EXPECT_STREQ(src.get_file(), copy.get_file());
  EXPECT_STREQ(src.get_function(), copy.get_function());
  EXPECT_EQ(src.get_line(), copy.get_line());
  EXPECT_EQ(src.get_source(), copy.get_source());

  EXPECT_STREQ(src.get_cause(), assigned.get_cause());
  EXPECT_STREQ(src.get_file(), assigned.get_file());
  EXPECT_STREQ(src.get_function(), assigned.get_function());
  EXPECT_EQ(src.get_line(), assigned.get_line());
  EXPECT_EQ(src.get_source(), assigned.get_source());
}

TEST(GExceptionTest, ThrowAndCatchMacrosWork)
{
  bool caught = false;
  G_TRY
  {
    G_THROW("gtest-throw");
  }
  G_CATCH(ex)
  {
    caught = true;
    EXPECT_EQ(0, ex.cmp_cause("gtest-throw"));
    EXPECT_NE(nullptr, ex.get_file());
    EXPECT_GT(ex.get_line(), 0);
  }
  G_ENDCATCH;

  EXPECT_TRUE(caught);
}
