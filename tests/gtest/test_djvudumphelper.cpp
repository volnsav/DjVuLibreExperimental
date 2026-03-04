#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DjVuDumpHelper.h"
#include "IFFByteStream.h"

TEST(DjVuDumpHelperTest, DumpProducesChunkListing)
{
  GP<ByteStream> input = ByteStream::create();
  GP<IFFByteStream> writer = IFFByteStream::create(input);
  writer->put_chunk("FORM:TEST");
  writer->put_chunk("DATA");
  const char payload[] = "abc";
  ASSERT_EQ(sizeof(payload) - 1, writer->write(payload, sizeof(payload) - 1));
  writer->close_chunk();
  writer->close_chunk();

  input->seek(0, SEEK_SET);
  DjVuDumpHelper helper;
  GP<ByteStream> out = helper.dump(input);
  ASSERT_TRUE(out != 0);

  out->seek(0, SEEK_SET);
  const GUTF8String text = out->getAsUTF8();
  EXPECT_GE(text.search("FORM"), 0);
  EXPECT_GE(text.search("DATA"), 0);
}
