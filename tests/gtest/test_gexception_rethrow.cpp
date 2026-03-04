#include <gtest/gtest.h>

#include "GException.h"

TEST(GExceptionRethrowTest, RethrowPreservesCause)
{
  bool outer_caught = false;

  G_TRY
  {
    G_TRY
    {
      G_THROW("inner-cause");
    }
    G_CATCH(ex)
    {
      EXPECT_EQ(0, ex.cmp_cause("inner-cause"));
      G_RETHROW;
    }
    G_ENDCATCH;
  }
  G_CATCH(outer)
  {
    outer_caught = true;
    EXPECT_EQ(0, outer.cmp_cause("inner-cause"));
  }
  G_ENDCATCH;

  EXPECT_TRUE(outer_caught);
}
