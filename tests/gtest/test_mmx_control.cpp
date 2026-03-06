#include <gtest/gtest.h>

#include "MMX.h"

#ifdef _WIN32
#include <cstdlib>
#else
#include <stdlib.h>
#endif

namespace
{

void SetDisableMmxEnv(const char *value)
{
#ifdef _WIN32
  _putenv_s("LIBDJVU_DISABLE_MMX", value ? value : "");
#else
  if (value && value[0])
    setenv("LIBDJVU_DISABLE_MMX", value, 1);
  else
    unsetenv("LIBDJVU_DISABLE_MMX");
#endif
}

} // namespace

TEST(MMXControlTest, DisableMmxForcesZero)
{
  SetDisableMmxEnv(nullptr);
  MMXControl::enable_mmx();
  EXPECT_EQ(0, MMXControl::disable_mmx());
  EXPECT_EQ(0, MMXControl::mmxflag);
}

TEST(MMXControlTest, EnableRespectsDisableEnvVar)
{
  SetDisableMmxEnv("1");
  EXPECT_EQ(0, MMXControl::enable_mmx());
  EXPECT_EQ(0, MMXControl::mmxflag);
  SetDisableMmxEnv(nullptr);
}

TEST(MMXControlTest, EnableWithZeroEnvVarReturnsStableFlag)
{
  SetDisableMmxEnv("0");
  const int enabled = MMXControl::enable_mmx();
  EXPECT_TRUE(enabled == 0 || enabled == 1);
  EXPECT_EQ(enabled, MMXControl::mmxflag);
  SetDisableMmxEnv(nullptr);
}

TEST(MMXControlTest, RepeatedEnableDisableIsConsistent)
{
  SetDisableMmxEnv(nullptr);
  const int first = MMXControl::enable_mmx();
  const int second = MMXControl::enable_mmx();
  EXPECT_EQ(first, second);
  EXPECT_EQ(second, MMXControl::mmxflag);
  EXPECT_EQ(0, MMXControl::disable_mmx());
  EXPECT_EQ(0, MMXControl::mmxflag);
}

