#include <gtest/gtest.h>

#include <cstring>

#include "DjVuGlobal.h"

TEST(DjVuGlobalMemoryTest, FallbackAllocatorsAreUsable)
{
  void *p = _djvu_malloc(16);
  ASSERT_NE(nullptr, p);
  std::memset(p, 0xAB, 16);

  p = _djvu_realloc(p, 32);
  ASSERT_NE(nullptr, p);

  _djvu_free(p);

  void *z = _djvu_calloc(4, 8);
  ASSERT_NE(nullptr, z);
  const unsigned char *bytes = static_cast<const unsigned char *>(z);
  for (int i = 0; i < 32; ++i)
    EXPECT_EQ(0u, bytes[i]);
  _djvu_free(z);
}
