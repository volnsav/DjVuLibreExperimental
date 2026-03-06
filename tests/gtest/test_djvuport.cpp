#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DataPool.h"
#include "DjVuPort.h"

namespace {

class RecordingPort : public DjVuPort
{
public:
  int error_count = 0;
  int status_count = 0;
  bool consume_error = false;

  bool notify_error(const DjVuPort *, const GUTF8String &) override
  {
    ++error_count;
    return consume_error;
  }

  bool notify_status(const DjVuPort *, const GUTF8String &) override
  {
    ++status_count;
    return true;
  }
};

}  // namespace

TEST(DjVuPortTest, AddAndDeleteRouteAffectsNotificationDelivery)
{
  GP<RecordingPort> src = new RecordingPort();
  GP<RecordingPort> dst = new RecordingPort();
  dst->consume_error = true;
  DjVuPortcaster *pc = DjVuPort::get_portcaster();

  pc->add_route(src, dst);
  EXPECT_TRUE(pc->notify_error(src, GUTF8String("boom")));
  EXPECT_EQ(1, dst->error_count);

  pc->del_route(src, dst);
  EXPECT_FALSE(pc->notify_error(src, GUTF8String("again")));
  EXPECT_EQ(1, dst->error_count);
}

TEST(DjVuPortTest, AliasLookupAndPrefixLookupReturnAlivePorts)
{
  GP<RecordingPort> port = new RecordingPort();
  DjVuPortcaster *pc = DjVuPort::get_portcaster();

  DjVuPortcaster::clear_all_aliases();
  pc->add_alias(port, "gtest.alias.one");

  GP<DjVuPort> alias_hit = pc->alias_to_port("gtest.alias.one");
  ASSERT_TRUE(alias_hit != 0);

  GPList<DjVuPort> prefixed = pc->prefix_to_ports("gtest.alias");
  EXPECT_EQ(1, prefixed.size());

  pc->clear_aliases(port);
  EXPECT_TRUE(pc->alias_to_port("gtest.alias.one") == 0);
}

TEST(DjVuPortTest, MemoryPortReturnsMappedDataPool)
{
  GP<DjVuMemoryPort> mem_port = new DjVuMemoryPort();
  GP<DataPool> pool = DataPool::create();
  const char payload[] = "memory-data";
  pool->add_data(payload, static_cast<int>(sizeof(payload) - 1));
  pool->set_eof();

  const GURL url = GURL::UTF8("http://example.com/pool.bin");
  mem_port->add_data(url, pool);

  GP<DataPool> out_pool = mem_port->request_data(nullptr, url);
  ASSERT_TRUE(out_pool != 0);

  GP<ByteStream> out = out_pool->get_stream();
  ASSERT_TRUE(out != 0);
  out->seek(0, SEEK_SET);
  GUTF8String text = out->getAsUTF8();
  EXPECT_GE(text.search("memory-data"), 0);
}

TEST(DjVuPortTest, ClosestRouteConsumesErrorBeforeFartherRoute)
{
  GP<RecordingPort> src = new RecordingPort();
  GP<RecordingPort> mid = new RecordingPort();
  GP<RecordingPort> far_port = new RecordingPort();
  DjVuPortcaster *pc = DjVuPort::get_portcaster();

  mid->consume_error = true;
  pc->add_route(src, mid);
  pc->add_route(mid, far_port);

  EXPECT_TRUE(pc->notify_error(src, GUTF8String("err")));
  EXPECT_EQ(1, mid->error_count);
  EXPECT_EQ(0, far_port->error_count);
}

TEST(DjVuPortTest, CopyConstructedPortCanBeRoutedAndUsed)
{
  GP<RecordingPort> src = new RecordingPort();
  GP<RecordingPort> dst = new RecordingPort();
  DjVuPortcaster *pc = DjVuPort::get_portcaster();

  GP<RecordingPort> src_copy = new RecordingPort(*src);
  pc->add_route(src_copy, dst);
  EXPECT_TRUE(pc->notify_status(src_copy, GUTF8String("status-from-copy")));
  EXPECT_EQ(1, dst->status_count);
}

TEST(DjVuPortTest, AssignmentCopiesOutgoingRoutes)
{
  GP<RecordingPort> src = new RecordingPort();
  GP<RecordingPort> dst = new RecordingPort();
  GP<RecordingPort> assigned = new RecordingPort();
  DjVuPortcaster *pc = DjVuPort::get_portcaster();

  pc->add_route(src, dst);
  *assigned = *src;

  EXPECT_TRUE(pc->notify_status(assigned, GUTF8String("status-from-assigned")));
  EXPECT_EQ(1, dst->status_count);
}

TEST(DjVuPortTest, DelPortRemovesRoutesAndAliases)
{
  GP<RecordingPort> src = new RecordingPort();
  GP<RecordingPort> dst = new RecordingPort();
  DjVuPortcaster *pc = DjVuPort::get_portcaster();

  DjVuPortcaster::clear_all_aliases();
  pc->add_alias(src, "gtest.delport.alias");
  pc->add_route(src, dst);
  EXPECT_TRUE(pc->notify_status(src, GUTF8String("before-delete")));
  EXPECT_EQ(1, dst->status_count);

  pc->del_port(src);
  EXPECT_TRUE(pc->alias_to_port("gtest.delport.alias") == 0);
  EXPECT_FALSE(pc->notify_status(src, GUTF8String("after-delete")));
  EXPECT_EQ(1, dst->status_count);
}

TEST(DjVuPortTest, MemoryPortReturnsNullForUnknownUrl)
{
  GP<DjVuMemoryPort> mem_port = new DjVuMemoryPort();
  const GURL missing = GURL::UTF8("http://example.com/missing.bin");
  GP<DataPool> out_pool = mem_port->request_data(nullptr, missing);
  EXPECT_TRUE(out_pool == 0);
}
