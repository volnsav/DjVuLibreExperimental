#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DataPool.h"
#include "DjVmDoc.h"
#include "IFFByteStream.h"

namespace {

GP<ByteStream> MakeMinimalDjvuFile()
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);

  iff->put_chunk("FORM:DJVU");
  iff->put_chunk("DATA");
  const char payload[] = "x";
  EXPECT_EQ(sizeof(payload) - 1, iff->write(payload, sizeof(payload) - 1));
  iff->close_chunk();
  iff->close_chunk();

  bs->seek(0, SEEK_SET);
  return bs;
}

}  // namespace

TEST(DjVmDocTest, InsertGetAndDeleteFile)
{
  GP<DjVmDoc> doc = DjVmDoc::create();
  ASSERT_TRUE(doc != 0);

  GP<ByteStream> page = MakeMinimalDjvuFile();
  doc->insert_file(*page, DjVmDir::File::PAGE, "page1.djvu", "page1.djvu", "Page 1");

  GP<DjVmDir> dir = doc->get_djvm_dir();
  ASSERT_TRUE(dir != 0);
  EXPECT_EQ(1, dir->get_files_num());
  EXPECT_EQ(1, dir->get_pages_num());

  GP<DataPool> pool = doc->get_data("page1.djvu");
  ASSERT_TRUE(pool != 0);
  GP<ByteStream> out = pool->get_stream();
  ASSERT_TRUE(out != 0);

  out->seek(0, SEEK_SET);
  GP<IFFByteStream> in_iff = IFFByteStream::create(out);
  GUTF8String chunk;
  EXPECT_GT(in_iff->get_chunk(chunk), 0);
  EXPECT_STREQ("FORM:DJVU", (const char *)chunk);

  doc->delete_file("page1.djvu");
  EXPECT_EQ(0, dir->get_files_num());
}

TEST(DjVmDocTest, DuplicateInsertThrows)
{
  GP<DjVmDoc> doc = DjVmDoc::create();
  ASSERT_TRUE(doc != 0);

  GP<ByteStream> first = MakeMinimalDjvuFile();
  GP<ByteStream> second = MakeMinimalDjvuFile();

  doc->insert_file(*first, DjVmDir::File::PAGE, "page1.djvu", "page1.djvu", "Page 1");
  EXPECT_THROW(
    doc->insert_file(*second, DjVmDir::File::PAGE, "page1.djvu", "page1.djvu", "Page 1 dup"),
    GException);
}

TEST(DjVmDocTest, WriteThenReadRoundtripPreservesDirectory)
{
  GP<DjVmDoc> writer = DjVmDoc::create();
  ASSERT_TRUE(writer != 0);

  GP<ByteStream> p1 = MakeMinimalDjvuFile();
  GP<ByteStream> p2 = MakeMinimalDjvuFile();
  writer->insert_file(*p1, DjVmDir::File::PAGE, "page1.djvu", "page1.djvu", "Page 1");
  writer->insert_file(*p2, DjVmDir::File::PAGE, "page2.djvu", "page2.djvu", "Page 2");

  GP<ByteStream> bundled = ByteStream::create();
  writer->write(bundled);
  bundled->seek(0, SEEK_SET);

  GP<DjVmDoc> reader = DjVmDoc::create();
  ASSERT_TRUE(reader != 0);
  reader->read(*bundled);

  GP<DjVmDir> dir = reader->get_djvm_dir();
  ASSERT_TRUE(dir != 0);
  EXPECT_EQ(2, dir->get_files_num());
  EXPECT_EQ(2, dir->get_pages_num());
}
