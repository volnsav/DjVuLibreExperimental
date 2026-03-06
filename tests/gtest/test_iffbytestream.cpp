#include <gtest/gtest.h>

#include "ByteStream.h"
#include "IFFByteStream.h"

TEST(IFFByteStreamTest, CheckIdClassifiesIds)
{
  EXPECT_EQ(1, IFFByteStream::check_id("FORM"));
  EXPECT_EQ(0, IFFByteStream::check_id("DATA"));
  EXPECT_EQ(-1, IFFByteStream::check_id("FOR1"));
}

TEST(IFFByteStreamTest, PutAndGetChunkRoundtrip)
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> writer = IFFByteStream::create(bs);

  writer->put_chunk("FORM:TEST", 1);
  writer->put_chunk("DATA");
  const char payload[] = "abc";
  EXPECT_EQ(sizeof(payload) - 1, writer->write(payload, sizeof(payload) - 1));
  writer->close_chunk();
  writer->close_chunk();

  bs->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(bs);

  GUTF8String chkid;
  const int form_size = reader->get_chunk(chkid);
  EXPECT_GT(form_size, 0);
  EXPECT_STREQ("FORM:TEST", chkid);
  EXPECT_TRUE(reader->has_magic_att);

  const int data_size = reader->get_chunk(chkid);
  EXPECT_EQ(3, data_size);
  EXPECT_STREQ("DATA", chkid);

  GUTF8String full;
  reader->full_id(full);
  EXPECT_STREQ("TEST.DATA", full);

  char out[4] = {0};
  EXPECT_EQ((size_t)3, reader->read(out, 3));
  EXPECT_STREQ("abc", out);

  reader->close_chunk();
  reader->close_chunk();
}

TEST(IFFByteStreamTest, CompareMatchesEqualStreams)
{
  GP<ByteStream> bs1 = ByteStream::create();
  GP<ByteStream> bs2 = ByteStream::create();

  {
    GP<IFFByteStream> w1 = IFFByteStream::create(bs1);
    w1->put_chunk("FORM:DJVU");
    w1->put_chunk("INFO");
    const char data[] = {1, 2, 3, 4};
    w1->write(data, sizeof(data));
    w1->close_chunk();
    w1->close_chunk();
  }
  {
    GP<IFFByteStream> w2 = IFFByteStream::create(bs2);
    w2->put_chunk("FORM:DJVU");
    w2->put_chunk("INFO");
    const char data[] = {1, 2, 3, 4};
    w2->write(data, sizeof(data));
    w2->close_chunk();
    w2->close_chunk();
  }

  bs1->seek(0, SEEK_SET);
  bs2->seek(0, SEEK_SET);
  GP<IFFByteStream> r1 = IFFByteStream::create(bs1);
  GP<IFFByteStream> r2 = IFFByteStream::create(bs2);

  EXPECT_TRUE(r1->compare(*r2));
}
