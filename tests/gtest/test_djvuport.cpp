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
