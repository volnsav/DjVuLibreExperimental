#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>

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

std::filesystem::path MakeTempPath(const char *suffix)
{
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("djvu_gtest_djvmdoc_" + std::to_string(now) + "_" + suffix);
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

TEST(DjVmDocTest, ExpandReadByUrlAndWriteIndexPaths)
{
  const std::filesystem::path out_dir = MakeTempPath("expand");
  std::filesystem::create_directories(out_dir);
  const GURL codebase = GURL::Filename::UTF8(out_dir.string().c_str());

  GP<DjVmDoc> writer = DjVmDoc::create();
  ASSERT_TRUE(writer != 0);
  GP<ByteStream> p1 = MakeMinimalDjvuFile();
  GP<ByteStream> p2 = MakeMinimalDjvuFile();
  writer->insert_file(*p1, DjVmDir::File::PAGE, "p1.djvu", "p1.djvu", "P1");
  writer->insert_file(*p2, DjVmDir::File::PAGE, "p2.djvu", "p2.djvu", "P2");
  writer->expand(codebase, "index.djvu");

  const GURL index_url = GURL::Filename::UTF8((out_dir / "index.djvu").string().c_str());
  GP<DjVmDoc> reader = DjVmDoc::create();
  ASSERT_TRUE(reader != 0);
  reader->read(index_url);

  GP<DjVmDir> dir = reader->get_djvm_dir();
  ASSERT_TRUE(dir != 0);
  EXPECT_EQ(2, dir->get_pages_num());

  GP<ByteStream> idx = ByteStream::create();
  EXPECT_NO_THROW(reader->write_index(idx));
  EXPECT_GT(idx->size(), 0);

  GP<DjVmDir::File> page0 = dir->page_to_file(0);
  ASSERT_TRUE(page0 != 0);
  EXPECT_NO_THROW(reader->save_page(codebase, *page0));
  EXPECT_NO_THROW(reader->save_file(codebase, *page0));
}
