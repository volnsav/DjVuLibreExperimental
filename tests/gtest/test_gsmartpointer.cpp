#include <gtest/gtest.h>

#include <atomic>
#include <cstring>

#include "GSmartPointer.h"

namespace
{
class RefObj : public GPEnabled
{
public:
  explicit RefObj(int v) : value(v) {}
  ~RefObj() override { ++destroyed; }

  int value;
  static std::atomic<int> destroyed;
};

std::atomic<int> RefObj::destroyed(0);
} // namespace

TEST(GSmartPointerTest, ReferenceCountingAndDestruction)
{
  RefObj::destroyed.store(0, std::memory_order_release);

  GP<RefObj> p1 = new RefObj(7);
  ASSERT_TRUE((bool)(RefObj *)p1);
  EXPECT_EQ(1, p1->get_count());
  EXPECT_EQ(7, p1->value);

  {
    GP<RefObj> p2 = p1;
    EXPECT_EQ(2, p1->get_count());
    EXPECT_EQ(2, p2->get_count());

    GP<RefObj> p3;
    p3 = p2;
    EXPECT_EQ(3, p1->get_count());
  }

  EXPECT_EQ(1, p1->get_count());
  p1 = (RefObj *)0;
  EXPECT_EQ(1, RefObj::destroyed.load(std::memory_order_acquire));
}

TEST(GSmartPointerTest, ComparisonAndDereferenceOperators)
{
  GP<RefObj> a = new RefObj(10);
  GP<RefObj> b = a;
  GP<RefObj> c = new RefObj(20);

  EXPECT_TRUE(a == (RefObj *)b);
  EXPECT_TRUE(a != (RefObj *)c);
  EXPECT_EQ(10, (*a).value);
}

TEST(GSmartPointerTest, BufferResizeSetAndReplace)
{
  unsigned char *buf = nullptr;
  {
    GPBuffer<unsigned char> holder(buf, 4);
    ASSERT_NE(nullptr, buf);
    std::memset(buf, 0x11, 4);

    holder.resize(8);
    ASSERT_NE(nullptr, buf);
    EXPECT_EQ(0x11, buf[0]);
    EXPECT_EQ(0x11, buf[3]);

    holder.set(static_cast<char>(0x7F));
    EXPECT_EQ(0x7F, buf[0]);
    EXPECT_EQ(0x7F, buf[7]);
  }

  unsigned char *external = (unsigned char *)::operator new(3);
  external[0] = 1;
  external[1] = 2;
  external[2] = 3;
  unsigned char *adopted = nullptr;
  {
    GPBuffer<unsigned char> holder(adopted, 0);
    holder.replace(external, 3);
    EXPECT_EQ(1, adopted[0]);
    EXPECT_EQ(2, adopted[1]);
    EXPECT_EQ(3, adopted[2]);
  }
}
