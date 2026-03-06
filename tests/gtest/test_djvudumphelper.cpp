#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DataPool.h"
#include "DjVuDumpHelper.h"
#include "DjVuInfo.h"
#include "DjVmDoc.h"
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

TEST(DjVuDumpHelperTest, DumpDataPoolOverloadAndBundledDirectoryDetails)
{
  GP<ByteStream> page = ByteStream::create();
  GP<IFFByteStream> p_iff = IFFByteStream::create(page);
  p_iff->put_chunk("FORM:DJVU");
  p_iff->put_chunk("INFO");
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 80;
  info->height = 60;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  info->encode(*p_iff->get_bytestream());
  p_iff->close_chunk();
  p_iff->put_chunk("INCL");
  p_iff->get_bytestream()->writestring(GUTF8String("shared.djvi"));
  p_iff->close_chunk();
  p_iff->close_chunk();
  page->seek(0, SEEK_SET);

  GP<ByteStream> shared = ByteStream::create();
  GP<IFFByteStream> s_iff = IFFByteStream::create(shared);
  s_iff->put_chunk("FORM:DJVI");
  s_iff->put_chunk("ANTa");
  s_iff->get_bytestream()->writestring(GUTF8String("(background #010203)"));
  s_iff->close_chunk();
  s_iff->close_chunk();
  shared->seek(0, SEEK_SET);

  GP<DjVmDoc> doc = DjVmDoc::create();
  doc->insert_file(*shared, DjVmDir::File::INCLUDE, "shared.djvi", "shared.djvi", "Shared");
  doc->insert_file(*page, DjVmDir::File::PAGE, "page1.djvu", "page1.djvu", "Page 1");

  GP<ByteStream> bundled = ByteStream::create();
  doc->write(bundled);
  bundled->seek(0, SEEK_SET);

  GP<DataPool> pool = DataPool::create(bundled);
  DjVuDumpHelper helper;
  GP<ByteStream> out = helper.dump(pool);
  ASSERT_TRUE(out != 0);
  out->seek(0, SEEK_SET);

  const GUTF8String text = out->getAsUTF8();
  EXPECT_GE(text.search("Document directory"), 0);
  EXPECT_GE(text.search("{page1.djvu}"), 0);
  EXPECT_GE(text.search("INCL"), 0);
}

TEST(DjVuDumpHelperTest, DumpSummarizesCodecAndTextChunkKinds)
{
  GP<ByteStream> input = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(input);
  iff->put_chunk("FORM:DJVU");

  iff->put_chunk("Smmr");
  iff->get_bytestream()->write8(0);
  iff->close_chunk();

  iff->put_chunk("Sjbz");
  iff->get_bytestream()->write8(0);
  iff->close_chunk();

  iff->put_chunk("Djbz");
  iff->get_bytestream()->write8(0);
  iff->close_chunk();

  iff->put_chunk("FGbz");
  iff->get_bytestream()->write8(1);
  iff->get_bytestream()->write16(3);
  iff->close_chunk();

  iff->put_chunk("FG44");
  {
    GP<ByteStream> bs = iff->get_bytestream();
    bs->write8(0);  // serial
    bs->write8(2);  // slices
    bs->write8(1);  // major
    bs->write8(2);  // minor
    bs->write8(0);
    bs->write8(64);
    bs->write8(0);
    bs->write8(32);
  }
  iff->close_chunk();

  iff->put_chunk("ANTa");
  iff->get_bytestream()->writestring(GUTF8String("(background #010203)"));
  iff->close_chunk();

  iff->put_chunk("TXTa");
  iff->get_bytestream()->writestring(GUTF8String("hello"));
  iff->close_chunk();

  iff->close_chunk();
  input->seek(0, SEEK_SET);

  DjVuDumpHelper helper;
  GP<ByteStream> out = helper.dump(input);
  ASSERT_TRUE(out != 0);
  out->seek(0, SEEK_SET);
  const GUTF8String text = out->getAsUTF8();

  EXPECT_GE(text.search("G4/MMR stencil data"), 0);
  EXPECT_GE(text.search("JB2 bilevel data"), 0);
  EXPECT_GE(text.search("JB2 shared dictionary"), 0);
  EXPECT_GE(text.search("JB2 colors data"), 0);
  EXPECT_GE(text.search("IW4 data #"), 0);
  EXPECT_GE(text.search("Page annotation"), 0);
  EXPECT_GE(text.search("Hidden text"), 0);
}

TEST(DjVuDumpHelperTest, DumpSummarizesTh44FormChunk)
{
  GP<ByteStream> input = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(input);
  iff->put_chunk("FORM:THUM");
  iff->put_chunk("TH44");
  {
    GP<ByteStream> bs = iff->get_bytestream();
    bs->write8(0);  // serial
    bs->write8(1);  // slices
    bs->write8(1);  // major
    bs->write8(0);  // minor
    bs->write8(0);
    bs->write8(16);
    bs->write8(0);
    bs->write8(16);
  }
  iff->close_chunk();
  iff->close_chunk();
  input->seek(0, SEEK_SET);

  DjVuDumpHelper helper;
  GP<ByteStream> out = helper.dump(input);
  ASSERT_TRUE(out != 0);
  out->seek(0, SEEK_SET);
  const GUTF8String text = out->getAsUTF8();
  EXPECT_GE(text.search("Thumbnail icon"), 0);
}
