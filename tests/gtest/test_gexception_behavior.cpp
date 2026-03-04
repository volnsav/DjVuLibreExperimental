#include <gtest/gtest.h>

#include "GException.h"

TEST(GExceptionBehaviorTest, StaticCompareHandlesEmptyAndNullLikeValues)
{
  EXPECT_GT(GException::cmp_cause("A", ""), 0);
  EXPECT_LT(GException::cmp_cause("", "A"), 0);
  EXPECT_EQ(0, GException::cmp_cause("same", "same"));
}

TEST(GExceptionBehaviorTest, OutOfMemoryCausePointerIsKeptAcrossCopies)
{
  GException src(GException::outofmemory);
  GException copy(src);
  GException assigned;
  assigned = src;

  EXPECT_EQ(GException::outofmemory, src.get_cause());
  EXPECT_EQ(GException::outofmemory, copy.get_cause());
  EXPECT_EQ(GException::outofmemory, assigned.get_cause());
}

TEST(GExceptionBehaviorTest, ConstructorPreservesSourceType)
{
  GException ex("cause", "f.cpp", 77, "func", GException::GEXTERNAL);
  EXPECT_EQ(GException::GEXTERNAL, ex.get_source());
  EXPECT_STREQ("f.cpp", ex.get_file());
  EXPECT_STREQ("func", ex.get_function());
  EXPECT_EQ(77, ex.get_line());
}
