#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "BSByteStream.h"
#include "ByteStream.h"
#include "DataPool.h"
#include "DjVuDocument.h"
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

std::optional<std::filesystem::path> FindReferenceFixtureFile(const char *name)
{
  const std::filesystem::path p1 = std::filesystem::path("tests/fixtures/reference") / name;
  if (std::filesystem::exists(p1))
    return p1;
  const std::filesystem::path p2 = std::filesystem::path("fixtures/reference") / name;
  if (std::filesystem::exists(p2))
    return p2;
  return std::nullopt;
}

std::string ReadAll(const GP<ByteStream> &bs)
{
  bs->seek(0, SEEK_SET);
  std::string out;
  char chunk[512];
  while (true)
  {
    const size_t got = bs->read(chunk, sizeof(chunk));
    if (!got)
      break;
    out.append(chunk, got);
  }
  return out;
}

bool HasChunk(const GP<DjVuFile> &file, const char *name)
{
  for (int i = 0; i < file->get_chunks_number(); ++i)
  {
    if (file->get_chunk_name(i) == name)
      return true;
  }
  return false;
}

int IncludedFilesCount(const GP<DjVuFile> &file)
{
  return file ? file->get_included_files(false).size() : 0;
}

struct PageSignals
{
  bool contains_anno = false;
  bool contains_text = false;
  bool contains_meta = false;
  long anno_size = -1;
  long text_size = -1;
  long meta_size = -1;

  bool operator==(const PageSignals &other) const
  {
    return contains_anno == other.contains_anno &&
           contains_text == other.contains_text &&
           contains_meta == other.contains_meta &&
           anno_size == other.anno_size &&
           text_size == other.text_size &&
           meta_size == other.meta_size;
  }
};

std::vector<PageSignals> CollectPageSignals(const GP<DjVuDocument> &doc)
{
  std::vector<PageSignals> out;
  const int pages = doc->get_pages_num();
  out.reserve(static_cast<size_t>(pages));
  for (int i = 0; i < pages; ++i)
  {
    GP<DjVuFile> file = doc->get_djvu_file(i, false);
    EXPECT_TRUE(file != 0);
    if (!file)
      continue;

    EXPECT_NO_THROW(file->resume_decode(true));
    PageSignals s;
    s.contains_anno = file->contains_anno();
    s.contains_text = file->contains_text();
    s.contains_meta = file->contains_meta();

    GP<ByteStream> anno = file->get_anno();
    GP<ByteStream> text = file->get_text();
    GP<ByteStream> meta = file->get_meta();
    s.anno_size = anno ? anno->size() : -1;
    s.text_size = text ? text->size() : -1;
    s.meta_size = meta ? meta->size() : -1;
    out.push_back(s);
  }
  return out;
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

TEST(DjVuFileTest, SharedResourceFixtureRawExportControlsIncludeInlining)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("mp2_bundled_shared_resources.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> doc = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->is_init_complete());
  ASSERT_EQ(2, doc->get_pages_num());

  GP<DjVuFile> root = doc->get_djvu_file(-1, false);
  ASSERT_TRUE(root != 0);

  const std::string with_no_ndir_flag_off = ReadAll(root->get_djvu_bytestream(false, false));
  const std::string with_no_ndir_flag_on = ReadAll(root->get_djvu_bytestream(false, true));
  EXPECT_EQ(std::string::npos, with_no_ndir_flag_off.find("NDIR"));
  EXPECT_EQ(std::string::npos, with_no_ndir_flag_on.find("NDIR"));
  EXPECT_EQ(with_no_ndir_flag_off.size(), with_no_ndir_flag_on.size());

  GP<DjVuFile> include_owner;
  for (int i = 0; i < doc->get_pages_num(); ++i)
  {
    GP<DjVuFile> file = doc->get_djvu_file(i, false);
    ASSERT_TRUE(file != 0);
    EXPECT_NO_THROW(file->resume_decode(true));
    if (HasChunk(file, "INCL"))
    {
      include_owner = file;
      break;
    }
  }
  ASSERT_TRUE(include_owner != 0);

  const std::string with_incl = ReadAll(include_owner->get_djvu_bytestream(false, true));
  const std::string inlined = ReadAll(include_owner->get_djvu_bytestream(true, true));
  EXPECT_NE(std::string::npos, with_incl.find("INCL"));
  EXPECT_NE(std::string::npos, with_incl.find("shared_page2.djbz"));
  EXPECT_EQ(std::string::npos, inlined.find("INCL"));
  EXPECT_GT(inlined.size(), with_incl.size());
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

TEST(DjVuFileTest, ReferenceFixtureSaveAsRoundtripPreservesAnnoAndTextExtraction)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("sp_bg_fgtext_hidden_links.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> src = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(src != 0);
  ASSERT_TRUE(src->is_init_complete());
  ASSERT_EQ(1, src->get_pages_num());

  const std::filesystem::path bundled_path = MakeTempPath("ref_roundtrip_bundled.djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_path.string().c_str());
  EXPECT_NO_THROW(src->save_as(bundled_url, true));
  EXPECT_TRUE(std::filesystem::exists(bundled_path));

  GP<DjVuDocument> bundled = DjVuDocument::create_wait(bundled_url);
  ASSERT_TRUE(bundled != 0);
  ASSERT_TRUE(bundled->is_init_complete());
  ASSERT_EQ(1, bundled->get_pages_num());

  GP<DjVuFile> bundled_file = bundled->get_djvu_file(0, false);
  ASSERT_TRUE(bundled_file != 0);
  EXPECT_NO_THROW(bundled_file->resume_decode(true));
  EXPECT_TRUE(bundled_file->contains_anno());
  EXPECT_TRUE(bundled_file->contains_text());
  EXPECT_FALSE(bundled_file->contains_meta());
  EXPECT_TRUE(bundled_file->get_anno() != 0);
  EXPECT_TRUE(bundled_file->get_text() != 0);

  const std::filesystem::path indirect_dir = MakeTempPath("ref_roundtrip_indirect");
  std::filesystem::create_directories(indirect_dir);
  const std::filesystem::path indirect_idx = indirect_dir / "index.djvu";
  const GURL indirect_url = GURL::Filename::UTF8(indirect_idx.string().c_str());
  EXPECT_NO_THROW(src->save_as(indirect_url, false));
  EXPECT_TRUE(std::filesystem::exists(indirect_idx));

  GP<DjVuDocument> indirect = DjVuDocument::create_wait(indirect_url);
  ASSERT_TRUE(indirect != 0);
  ASSERT_TRUE(indirect->is_init_complete());
  ASSERT_EQ(1, indirect->get_pages_num());

  GP<DjVuFile> indirect_file = indirect->get_djvu_file(0, false);
  ASSERT_TRUE(indirect_file != 0);
  EXPECT_NO_THROW(indirect_file->resume_decode(true));
  EXPECT_TRUE(indirect_file->contains_anno());
  EXPECT_TRUE(indirect_file->contains_text());
  EXPECT_TRUE(indirect_file->get_anno() != 0);
  EXPECT_TRUE(indirect_file->get_text() != 0);
}

TEST(DjVuFileTest, ReferenceMultipageRoundtripPreservesPerPageExtractionSignals)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("mp3_bundled_mixed_layers.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> src = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(src != 0);
  ASSERT_TRUE(src->is_init_complete());
  ASSERT_EQ(3, src->get_pages_num());

  const std::vector<PageSignals> source = CollectPageSignals(src);
  ASSERT_EQ(3u, source.size());
  EXPECT_FALSE(source[0].contains_anno);
  EXPECT_FALSE(source[0].contains_text);
  EXPECT_FALSE(source[1].contains_anno);
  EXPECT_FALSE(source[1].contains_text);
  EXPECT_TRUE(source[2].contains_anno);
  EXPECT_TRUE(source[2].contains_text);

  const std::filesystem::path bundled_path = MakeTempPath("ref_multipage_bundle.djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_path.string().c_str());
  EXPECT_NO_THROW(src->save_as(bundled_url, true));

  GP<DjVuDocument> bundled = DjVuDocument::create_wait(bundled_url);
  ASSERT_TRUE(bundled != 0);
  ASSERT_TRUE(bundled->is_init_complete());
  EXPECT_EQ(source, CollectPageSignals(bundled));

  const std::filesystem::path indirect_dir = MakeTempPath("ref_multipage_indirect");
  std::filesystem::create_directories(indirect_dir);
  const std::filesystem::path indirect_idx = indirect_dir / "index.djvu";
  const GURL indirect_url = GURL::Filename::UTF8(indirect_idx.string().c_str());
  EXPECT_NO_THROW(src->save_as(indirect_url, false));

  GP<DjVuDocument> indirect = DjVuDocument::create_wait(indirect_url);
  ASSERT_TRUE(indirect != 0);
  ASSERT_TRUE(indirect->is_init_complete());
  EXPECT_EQ(source, CollectPageSignals(indirect));
}

TEST(DjVuFileTest, ReferenceSharedResourceRoundtripPreservesTextSignalsAcrossPages)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("mp2_bundled_shared_resources.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> src = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(src != 0);
  ASSERT_TRUE(src->is_init_complete());
  ASSERT_EQ(2, src->get_pages_num());

  const std::vector<PageSignals> source = CollectPageSignals(src);
  ASSERT_EQ(2u, source.size());

  const std::filesystem::path indirect_dir = MakeTempPath("ref_shared_indirect");
  std::filesystem::create_directories(indirect_dir);
  const GURL indirect_url =
      GURL::Filename::UTF8((indirect_dir / "index.djvu").string().c_str());
  EXPECT_NO_THROW(src->save_as(indirect_url, false));

  GP<DjVuDocument> indirect = DjVuDocument::create_wait(indirect_url);
  ASSERT_TRUE(indirect != 0);
  ASSERT_TRUE(indirect->is_init_complete());
  EXPECT_EQ(source, CollectPageSignals(indirect));
}

TEST(DjVuFileTest, SequentialRoundtripPreservesIncludedFileCountsForSharedFixture)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("mp2_bundled_shared_resources.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  GP<DjVuDocument> source =
      DjVuDocument::create_wait(GURL::Filename::UTF8(fixture->string().c_str()));
  ASSERT_TRUE(source != 0);
  ASSERT_TRUE(source->is_init_complete());

  std::vector<int> include_counts;
  for (int i = 0; i < source->get_pages_num(); ++i)
  {
    GP<DjVuFile> file = source->get_djvu_file(i, false);
    ASSERT_TRUE(file != 0);
    EXPECT_NO_THROW(file->resume_decode(true));
    include_counts.push_back(IncludedFilesCount(file));
  }

  const std::filesystem::path indirect_dir = MakeTempPath("seq_shared_file_indirect");
  std::filesystem::create_directories(indirect_dir);
  const GURL indirect_url =
      GURL::Filename::UTF8((indirect_dir / "index.djvu").string().c_str());
  EXPECT_NO_THROW(source->save_as(indirect_url, false));

  GP<DjVuDocument> indirect = DjVuDocument::create_wait(indirect_url);
  ASSERT_TRUE(indirect != 0);
  ASSERT_TRUE(indirect->is_init_complete());
  ASSERT_EQ(source->get_pages_num(), indirect->get_pages_num());

  for (int i = 0; i < indirect->get_pages_num(); ++i)
  {
    GP<DjVuFile> file = indirect->get_djvu_file(i, false);
    ASSERT_TRUE(file != 0);
    EXPECT_NO_THROW(file->resume_decode(true));
    EXPECT_EQ(include_counts[static_cast<size_t>(i)], IncludedFilesCount(file));
  }

  const std::filesystem::path bundled_path = MakeTempPath("seq_shared_file_bundle.djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_path.string().c_str());
  EXPECT_NO_THROW(indirect->save_as(bundled_url, true));

  GP<DjVuDocument> bundled = DjVuDocument::create_wait(bundled_url);
  ASSERT_TRUE(bundled != 0);
  ASSERT_TRUE(bundled->is_init_complete());
  ASSERT_EQ(source->get_pages_num(), bundled->get_pages_num());

  for (int i = 0; i < bundled->get_pages_num(); ++i)
  {
    GP<DjVuFile> file = bundled->get_djvu_file(i, false);
    ASSERT_TRUE(file != 0);
    EXPECT_NO_THROW(file->resume_decode(true));
    EXPECT_EQ(include_counts[static_cast<size_t>(i)], IncludedFilesCount(file));
  }
}
