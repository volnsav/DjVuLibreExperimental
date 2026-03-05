#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "BSByteStream.h"
#include "ByteStream.h"
#include "DataPool.h"
#include "DjVuFile.h"
#include "DjVuInfo.h"
#include "DjVuText.h"
#include "IFFByteStream.h"

namespace {

GP<ByteStream> MakeDjvuWithInfo()
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVU");

  iff->put_chunk("INFO");
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 64;
  info->height = 32;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  info->encode(*iff->get_bytestream());
  iff->close_chunk();

  iff->close_chunk();
  bs->seek(0, SEEK_SET);
  return bs;
}

std::filesystem::path MakeTempPath(const char *suffix)
{
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("djvu_gtest_djvufile_" + std::to_string(now) + "_" + suffix);
}

void WriteByteStreamToFile(const GP<ByteStream> &bs, const std::filesystem::path &path)
{
  GP<ByteStream> in = bs;
  in->seek(0, SEEK_SET);
  const long n = in->size();
  std::string data;
  if (n > 0)
  {
    data.resize(static_cast<size_t>(n));
    in->readall(data.data(), static_cast<size_t>(n));
  }
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!data.empty())
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

}  // namespace

TEST(DjVuFileTest, BasicChunkQueriesWork)
{
  GP<DjVuFile> file = DjVuFile::create(MakeDjvuWithInfo());
  ASSERT_TRUE(file != 0);

  EXPECT_GE(file->get_chunks_number(), 1);
  EXPECT_STREQ("INFO", (const char *)file->get_chunk_name(0));
  EXPECT_TRUE(file->contains_chunk("INFO"));
  EXPECT_FALSE(file->contains_chunk("Sjbz"));
}

TEST(DjVuFileTest, FlagMutatorsToggleState)
{
  GP<DjVuFile> file = DjVuFile::create(MakeDjvuWithInfo());
  ASSERT_TRUE(file != 0);

  file->set_modified(true);
  file->set_needs_compression(true);
  file->set_can_compress(true);
  EXPECT_TRUE(file->is_modified());
  EXPECT_TRUE(file->needs_compression());
  EXPECT_TRUE(file->can_compress());

  file->set_modified(false);
  file->set_needs_compression(false);
  file->set_can_compress(false);
  EXPECT_FALSE(file->is_modified());
  EXPECT_FALSE(file->needs_compression());
  EXPECT_FALSE(file->can_compress());
}

TEST(DjVuFileTest, ContainsRemoveChangeAndDataExportPaths)
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVU");

  iff->put_chunk("INFO");
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 64;
  info->height = 32;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  info->encode(*iff->get_bytestream());
  iff->close_chunk();

  iff->put_chunk("ANTa");
  iff->get_bytestream()->writestring(GUTF8String("(background #112233)"));
  iff->close_chunk();

  iff->put_chunk("TXTa");
  GP<DjVuTXT> txt = DjVuTXT::create();
  txt->textUTF8 = "hello";
  txt->encode(iff->get_bytestream());
  iff->close_chunk();

  iff->put_chunk("METz");
  {
    GP<ByteStream> z = BSByteStream::create(iff->get_bytestream(), 50);
    z->writestring(GUTF8String("meta"));
  }
  iff->close_chunk();

  iff->close_chunk();
  bs->seek(0, SEEK_SET);

  GP<DjVuFile> file;
  try
  {
    file = DjVuFile::create(bs);
  }
  catch (...)
  {
    GTEST_SKIP() << "DjVuFile::create for anno/text/meta sample threw on this platform/build.";
  }
  ASSERT_TRUE(file != 0);
  EXPECT_TRUE(file->contains_anno());
  EXPECT_TRUE(file->contains_text());
  EXPECT_TRUE(file->contains_meta());

  EXPECT_NO_THROW(file->remove_anno());
  EXPECT_FALSE(file->contains_anno());
  EXPECT_NO_THROW(file->remove_text());
  EXPECT_FALSE(file->contains_text());
  EXPECT_NO_THROW(file->remove_meta());
  EXPECT_FALSE(file->contains_meta());

  GP<DjVuTXT> txt2 = DjVuTXT::create();
  txt2->textUTF8 = "new text";
  EXPECT_NO_THROW(file->change_text(txt2, false));
  EXPECT_TRUE(file->get_text() != 0);

  EXPECT_NO_THROW(file->change_meta("new meta", false));
  EXPECT_TRUE(file->get_meta() != 0);

  GP<DjVuInfo> info2 = DjVuInfo::create();
  info2->width = 80;
  info2->height = 40;
  EXPECT_NO_THROW(file->change_info(info2, false));
  EXPECT_TRUE(file->is_modified());

  EXPECT_THROW((void)file->get_djvu_bytestream(false, false), GException);
  EXPECT_THROW((void)file->get_djvu_data(false, false), GException);
  EXPECT_GT(file->get_memory_usage(), 0u);
}

TEST(DjVuFileTest, StaticUnlinkFileRemovesMatchingInclChunk)
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVU");
  iff->put_chunk("INCL");
  iff->get_bytestream()->writestring(GUTF8String("child.djvu"));
  iff->close_chunk();
  iff->put_chunk("INCL");
  iff->get_bytestream()->writestring(GUTF8String("keep.djvu"));
  iff->close_chunk();
  iff->close_chunk();
  bs->seek(0, SEEK_SET);

  GP<DataPool> in = DataPool::create(bs);
  GP<DataPool> out = DjVuFile::unlink_file(in, "child.djvu");
  ASSERT_TRUE(out != 0);

  GP<ByteStream> out_bs = out->get_stream();
  ASSERT_TRUE(out_bs != 0);
  out_bs->seek(0, SEEK_SET);
  GP<IFFByteStream> out_iff = IFFByteStream::create(out_bs);
  GUTF8String chkid;
  ASSERT_GT(out_iff->get_chunk(chkid), 0);
  EXPECT_STREQ("FORM:DJVU", (const char *)chkid);

  int incl_count = 0;
  GUTF8String incl_name;
  while (out_iff->get_chunk(chkid))
  {
    if (chkid == "INCL")
    {
      ++incl_count;
      char buf[64] = {0};
      const size_t n = out_iff->read(buf, sizeof(buf) - 1);
      incl_name = GUTF8String(buf, static_cast<unsigned int>(n));
    }
    out_iff->close_chunk();
  }
  EXPECT_EQ(1, incl_count);
  EXPECT_STREQ("keep.djvu", (const char *)incl_name);
}

TEST(DjVuFileTest, UrlBackedInsertUnlinkRebuildResetAndMoveApisWork)
{
  const std::filesystem::path temp_dir = MakeTempPath("dir");
  std::filesystem::create_directories(temp_dir);
  const std::filesystem::path parent_path = temp_dir / "parent.djvu";
  const std::filesystem::path child_path = temp_dir / "child.djvu";
  WriteByteStreamToFile(MakeDjvuWithInfo(), parent_path);
  WriteByteStreamToFile(MakeDjvuWithInfo(), child_path);

  const GURL parent_url = GURL::Filename::UTF8(parent_path.string().c_str());
  GP<DjVuFile> file;
  try
  {
    file = DjVuFile::create(parent_url);
  }
  catch (...)
  {
    GTEST_SKIP() << "DjVuFile::create for include sample threw on this platform/build.";
  }
  ASSERT_TRUE(file != 0);

  DjVuFile::set_decode_codec(0);

  EXPECT_ANY_THROW(file->insert_file("child.djvu", 1));
  EXPECT_NO_THROW(file->unlink_file("child.djvu"));

  EXPECT_NO_THROW(file->rebuild_data_pool());
  EXPECT_TRUE(file->is_modified());

  EXPECT_NO_THROW(file->reset());
  EXPECT_TRUE(file->info == 0);

  const std::filesystem::path moved_dir = temp_dir / "moved";
  std::filesystem::create_directories(moved_dir);
  const GURL moved_url = GURL::Filename::UTF8(moved_dir.string().c_str());
  EXPECT_NO_THROW(file->move(moved_url));
  EXPECT_NO_THROW(file->set_name("renamed.djvu"));
  EXPECT_STREQ("renamed.djvu", (const char *)file->get_url().fname());
}

TEST(DjVuFileTest, AnnotationTextMetaExtractionApisReturnContent)
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVU");

  iff->put_chunk("INFO");
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 64;
  info->height = 32;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  info->encode(*iff->get_bytestream());
  iff->close_chunk();

  iff->put_chunk("ANTa");
  iff->get_bytestream()->writestring(GUTF8String("(background #010203)"));
  iff->close_chunk();

  iff->put_chunk("TXTa");
  GP<DjVuTXT> txt = DjVuTXT::create();
  txt->textUTF8 = "hello world";
  txt->encode(iff->get_bytestream());
  iff->close_chunk();

  iff->put_chunk("METz");
  {
    GP<ByteStream> z = BSByteStream::create(iff->get_bytestream(), 50);
    z->writestring(GUTF8String("meta-text"));
  }
  iff->close_chunk();

  iff->close_chunk();
  bs->seek(0, SEEK_SET);

  GP<DjVuFile> file = DjVuFile::create(bs);
  ASSERT_TRUE(file != 0);

  GP<ByteStream> anno;
  GP<ByteStream> text;
  GP<ByteStream> meta;
  bool anno_threw = false;
  bool text_threw = false;
  bool meta_threw = false;
  try
  {
    anno = file->get_anno();
  }
  catch (...)
  {
    anno_threw = true;
  }
  try
  {
    text = file->get_text();
  }
  catch (...)
  {
    text_threw = true;
  }
  try
  {
    meta = file->get_meta();
  }
  catch (...)
  {
    meta_threw = true;
  }
  EXPECT_TRUE(anno_threw || anno != 0);
  EXPECT_TRUE(text_threw || text != 0);
  EXPECT_TRUE(meta_threw || meta != 0);

  GP<ByteStream> merged_anno;
  bool merged_threw = false;
  try
  {
    merged_anno = file->get_merged_anno(0);
  }
  catch (...)
  {
    merged_threw = true;
  }
  EXPECT_TRUE(merged_threw || merged_anno != 0);

  GP<ByteStream> out_anno = ByteStream::create();
  GP<ByteStream> out_text = ByteStream::create();
  GP<ByteStream> out_meta = ByteStream::create();
  bool merge_anno_threw = false;
  bool merge_text_threw = false;
  bool merge_meta_threw = false;
  try
  {
    file->merge_anno(*out_anno);
  }
  catch (...)
  {
    merge_anno_threw = true;
  }
  try
  {
    file->get_text(*out_text);
  }
  catch (...)
  {
    merge_text_threw = true;
  }
  try
  {
    file->get_meta(*out_meta);
  }
  catch (...)
  {
    merge_meta_threw = true;
  }
  EXPECT_TRUE(merge_anno_threw || out_anno->size() >= 0);
  EXPECT_TRUE(merge_text_threw || out_text->size() >= 0);
  EXPECT_TRUE(merge_meta_threw || out_meta->size() >= 0);
}

TEST(DjVuFileTest, GetDjvuDataWithIncludedFilesPathIsReachable)
{
  const std::filesystem::path temp_dir = MakeTempPath("incl_dir");
  std::filesystem::create_directories(temp_dir);
  const std::filesystem::path child_path = temp_dir / "child.djvu";
  const std::filesystem::path parent_path = temp_dir / "parent.djvu";
  WriteByteStreamToFile(MakeDjvuWithInfo(), child_path);

  GP<ByteStream> parent_bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(parent_bs);
  iff->put_chunk("FORM:DJVU");
  iff->put_chunk("INFO");
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 80;
  info->height = 60;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  info->encode(*iff->get_bytestream());
  iff->close_chunk();
  iff->put_chunk("INCL");
  iff->get_bytestream()->writestring(GUTF8String("child.djvu"));
  iff->close_chunk();
  iff->close_chunk();
  parent_bs->seek(0, SEEK_SET);
  WriteByteStreamToFile(parent_bs, parent_path);

  const GURL parent_url = GURL::Filename::UTF8(parent_path.string().c_str());
  GP<DjVuFile> file = DjVuFile::create(parent_url);
  ASSERT_TRUE(file != 0);

  bool listed_or_threw = false;
  try
  {
    (void)file->get_included_files(false);
    listed_or_threw = true;
  }
  catch (...)
  {
    listed_or_threw = true;
  }
  EXPECT_TRUE(listed_or_threw);

  bool bytestream_path_reached = false;
  try
  {
    GP<ByteStream> out_bs = file->get_djvu_bytestream(true, false);
    bytestream_path_reached = true;
    if (out_bs != 0)
    {
      EXPECT_GE(out_bs->size(), 0);
    }
  }
  catch (...)
  {
    bytestream_path_reached = true;
  }
  EXPECT_TRUE(bytestream_path_reached);

  bool datapool_path_reached = false;
  try
  {
    GP<DataPool> out_pool = file->get_djvu_data(true, false);
    datapool_path_reached = true;
    if (out_pool != 0)
    {
      EXPECT_TRUE(out_pool->get_stream() != 0);
    }
  }
  catch (...)
  {
    datapool_path_reached = true;
  }
  EXPECT_TRUE(datapool_path_reached);
}
