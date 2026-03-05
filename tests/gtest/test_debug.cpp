#include <gtest/gtest.h>

#include "debug.h"

TEST(DebugTest, MacrosAreCallable)
{
#if DEBUGLVL > 0
  DEBUG_SET_LEVEL(DEBUGLVL);
  DEBUG_MSG("debug gtest line\n");
  DEBUG_MSGN("debug gtest line without indent\n");
#else
  DEBUG_SET_LEVEL(1);
  DEBUG_MSG("noop in DEBUGLVL=0\n");
#endif
  SUCCEED();
}
