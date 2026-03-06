#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DataPool.h"
#include "DjVuErrorList.h"

TEST(DjVuErrorListTest, ErrorQueueGetAndClear)
{
  GP<DjVuErrorList> err = DjVuErrorList::create();

  EXPECT_FALSE(err->HasError());
  EXPECT_TRUE(err->notify_error(nullptr, "err-1"));
  EXPECT_TRUE(err->notify_error(nullptr, "err-2"));
  EXPECT_TRUE(err->HasError());

  EXPECT_STREQ("err-1", err->GetError());
  EXPECT_STREQ("err-2", err->GetError());
  EXPECT_STREQ("", err->GetError());
  EXPECT_FALSE(err->HasError());

  err->notify_error(nullptr, "err-3");
  GList<GUTF8String> all = err->GetErrorList();
  EXPECT_EQ(1, all.size());
  EXPECT_FALSE(err->HasError());
}

TEST(DjVuErrorListTest, StatusQueueGetAndClear)
{
  GP<DjVuErrorList> err = DjVuErrorList::create();

  EXPECT_FALSE(err->HasStatus());
  EXPECT_TRUE(err->notify_status(nullptr, "st-1"));
  EXPECT_TRUE(err->notify_status(nullptr, "st-2"));
  EXPECT_TRUE(err->HasStatus());

  EXPECT_STREQ("st-1", err->GetStatus());
  EXPECT_STREQ("st-2", err->GetStatus());
  EXPECT_STREQ("", err->GetStatus());
  EXPECT_FALSE(err->HasStatus());

  err->notify_status(nullptr, "st-3");
  err->ClearStatus();
  EXPECT_FALSE(err->HasStatus());
}

TEST(DjVuErrorListTest, RequestDataReturnsConfiguredDataPool)
{
  GP<DjVuErrorList> err = DjVuErrorList::create();

  GP<ByteStream> bs = ByteStream::create();
  const char payload[] = "abc";
  bs->writall(payload, sizeof(payload) - 1);
  bs->seek(0, SEEK_SET);

  const GURL url = err->set_stream(bs);
  GP<DataPool> pool = err->request_data(nullptr, url);
  ASSERT_TRUE(pool != 0);

  GP<ByteStream> in = pool->get_stream();
  char out[4] = {0, 0, 0, 0};
  ASSERT_EQ((size_t)3, in->readall(out, 3));
  EXPECT_STREQ("abc", out);
}
