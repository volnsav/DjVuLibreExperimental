#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <vector>

#include "ByteStream.h"
#include "DataPool.h"
#include "DjVuDocument.h"
#include "DjVuFile.h"
#include "DjVuFileCache.h"
#include "DjVuImage.h"
#include "DjVuInfo.h"
#include "DjVmDoc.h"
#include "IFFByteStream.h"

namespace {

GP<ByteStream> MakeSinglePageDjvu()
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVU");

  iff->put_chunk("INFO");
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 100;
  info->height = 50;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  info->encode(*iff->get_bytestream());
  iff->close_chunk();

  iff->close_chunk();
  bs->seek(0, SEEK_SET);
  return bs;
}

GP<ByteStream> MakeBundledTwoPageDjvu()
{
  GP<DjVmDoc> doc = DjVmDoc::create();
  GP<ByteStream> p1 = MakeSinglePageDjvu();
  GP<ByteStream> p2 = MakeSinglePageDjvu();
  doc->insert_file(*p1, DjVmDir::File::PAGE, "p1.djvu", "p1.djvu", "Page 1");
  doc->insert_file(*p2, DjVmDir::File::PAGE, "p2.djvu", "p2.djvu", "Page 2");
  GP<ByteStream> out = ByteStream::create();
  doc->write(out);
  out->seek(0, SEEK_SET);
  return out;
}

std::filesystem::path MakeTempPath(const char *suffix)
{
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("djvu_gtest_djvudocument_extra_" + std::to_string(now) + "_" + suffix);
}

void WriteByteStreamToFile(const GP<ByteStream> &bs, const std::filesystem::path &path)
{
  GP<ByteStream> in = bs;
  in->seek(0, SEEK_SET);
  const long n = in->size();
  std::vector<char> data(static_cast<size_t>(n > 0 ? n : 0));
  if (n > 0)
    in->readall(data.data(), static_cast<size_t>(n));
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!data.empty())
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

void FakeImportCodec(GP<DataPool> &, const GURL &, bool &needs_compression, bool &needs_rename)
{
  needs_compression = true;
  needs_rename = true;
}

void FakeCompressCodec(GP<ByteStream> &, const GURL &, bool)
{
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

struct PageSignals
{
  bool text = false;
  bool anno = false;

  bool operator==(const PageSignals &other) const
  {
    return text == other.text && anno == other.anno;
  }
};

std::vector<PageSignals> CollectPageSignals(const GP<DjVuDocument> &doc)
{
  std::vector<PageSignals> out;
  for (int i = 0; i < doc->get_pages_num(); ++i)
  {
    GP<DjVuFile> file = doc->get_djvu_file(i, false);
    EXPECT_TRUE(file != 0);
    if (!file)
      continue;
    EXPECT_NO_THROW(file->resume_decode(true));
    PageSignals s;
    s.text = file->contains_text();
    s.anno = file->contains_anno();
    out.push_back(s);
  }
  return out;
}

struct MappingSnapshot
{
  std::vector<std::string> page_ids;
  std::vector<std::string> page_url_fnames;
  size_t url_names = 0;

  bool operator==(const MappingSnapshot &other) const
  {
    return page_ids == other.page_ids && page_url_fnames == other.page_url_fnames &&
           url_names == other.url_names;
  }
};

MappingSnapshot CollectMappingSnapshot(const GP<DjVuDocument> &doc)
{
  MappingSnapshot snapshot;
  const int pages = doc->get_pages_num();
  snapshot.page_ids.reserve(static_cast<size_t>(pages));
  snapshot.page_url_fnames.reserve(static_cast<size_t>(pages));
  for (int i = 0; i < pages; ++i)
  {
    const GUTF8String id = doc->page_to_id(i);
    const GURL url = doc->page_to_url(i);
    snapshot.page_ids.emplace_back((const char *)id);
    snapshot.page_url_fnames.emplace_back((const char *)url.fname());
    EXPECT_EQ(i, doc->id_to_page(id));
    EXPECT_EQ(i, doc->url_to_page(url));
    EXPECT_TRUE(doc->get_djvu_file(id, false) != 0);
    EXPECT_TRUE(doc->get_djvu_file(url, false) != 0);
  }
  snapshot.url_names = static_cast<size_t>(doc->get_url_names().size());
  return snapshot;
}

int CountSubstring(const std::string &haystack, const std::string &needle)
{
  int count = 0;
  size_t pos = 0;
  while ((pos = haystack.find(needle, pos)) != std::string::npos)
  {
    ++count;
    pos += needle.size();
  }
  return count;
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

}  // namespace

TEST(DjVuDocumentExtraTest, PageIdRoundtripForSinglePage)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());

  const GUTF8String id0 = doc->page_to_id(0);
  EXPECT_TRUE(id0.length() > 0);
  EXPECT_EQ(0, doc->id_to_page(id0));
}

TEST(DjVuDocumentExtraTest, InitDataPoolExistsForByteStreamInit)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());

  GP<DataPool> pool = doc->get_init_data_pool();
  EXPECT_TRUE(pool != 0);
}

TEST(DjVuDocumentExtraTest, NewStyleDirectoryAccessThrowsForSinglePage)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());

  EXPECT_THROW(doc->get_djvm_dir(), GException);
}

TEST(DjVuDocumentExtraTest, OldStyleDirectoryAccessThrowsForSinglePage)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());

  EXPECT_THROW(doc->get_djvm_dir0(), GException);
}

TEST(DjVuDocumentExtraTest, GetPageMinusOneMatchesSinglePageGeometry)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());

  GP<DjVuImage> page = doc->get_page(-1, true);
  ASSERT_TRUE(page != 0);
  EXPECT_EQ(100, page->get_width());
  EXPECT_EQ(50, page->get_height());
}

TEST(DjVuDocumentExtraTest, BundledDocumentMappingsAndIdMapWork)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeBundledTwoPageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());
  EXPECT_EQ(DjVuDocument::BUNDLED, doc->get_doc_type());
  ASSERT_EQ(2, doc->get_pages_num());

  const GURL p0 = doc->page_to_url(0);
  const GURL p1 = doc->page_to_url(1);
  EXPECT_FALSE(p0.is_empty());
  EXPECT_FALSE(p1.is_empty());
  EXPECT_EQ(0, doc->url_to_page(p0));
  EXPECT_EQ(1, doc->url_to_page(p1));
  EXPECT_EQ(-1, doc->url_to_page(GURL::UTF8("file:///tmp/missing.djvu")));

  const GList<GUTF8String> ids = doc->get_id_list();
  EXPECT_EQ(2, ids.size());
  GMap<GUTF8String, void *> id_map;
  doc->map_ids(id_map);
  EXPECT_EQ(2, id_map.size());
  GPosition pos = ids;
  ASSERT_TRUE(pos);
  EXPECT_FALSE(doc->id_to_url(ids[pos]).is_empty());
}

TEST(DjVuDocumentExtraTest, BundledDocumentWriteAndSaveAsPaths)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeBundledTwoPageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());

  GP<ByteStream> out_a = ByteStream::create();
  EXPECT_NO_THROW(doc->write(out_a, false));
  EXPECT_GT(out_a->size(), 0);

  GP<ByteStream> out_b = ByteStream::create();
  GMap<GUTF8String, void *> reserved;
  reserved["reserved.djvu"] = 0;
  EXPECT_NO_THROW(doc->write(out_b, reserved));
  EXPECT_GT(out_b->size(), 0);

  GP<ByteStream> out_c = ByteStream::create();
  EXPECT_NO_THROW(doc->write(out_c, true));
  EXPECT_GT(out_c->size(), 0);

  const std::filesystem::path bundled_path = MakeTempPath("bundled.djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_path.string().c_str());
  EXPECT_NO_THROW(doc->save_as(bundled_url, true));
  EXPECT_TRUE(std::filesystem::exists(bundled_path));

  const std::filesystem::path indirect_dir = MakeTempPath("indir");
  std::filesystem::create_directories(indirect_dir);
  const std::filesystem::path indirect_idx = indirect_dir / "index.djvu";
  const GURL indirect_url = GURL::Filename::UTF8(indirect_idx.string().c_str());
  EXPECT_NO_THROW(doc->save_as(indirect_url, false));
  EXPECT_TRUE(std::filesystem::exists(indirect_idx));
}

TEST(DjVuDocumentExtraTest, SinglePageWriteUsesDirectDjvuUnlessForcedDjvm)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());
  ASSERT_EQ(DjVuDocument::SINGLE_PAGE, doc->get_doc_type());

  GP<ByteStream> direct = ByteStream::create();
  EXPECT_NO_THROW(doc->write(direct, false));
  const std::string direct_bytes = ReadAll(direct);
  EXPECT_NE(std::string::npos, direct_bytes.find("DJVU"));
  EXPECT_EQ(std::string::npos, direct_bytes.find("DJVM"));

  direct->seek(0, SEEK_SET);
  GP<DjVuDocument> direct_doc = DjVuDocument::create(direct);
  ASSERT_TRUE(direct_doc != 0);
  ASSERT_TRUE(direct_doc->wait_for_complete_init());
  EXPECT_EQ(DjVuDocument::SINGLE_PAGE, direct_doc->get_doc_type());
  EXPECT_EQ(1, direct_doc->get_pages_num());

  GP<ByteStream> forced = ByteStream::create();
  EXPECT_NO_THROW(doc->write(forced, true));
  const std::string forced_bytes = ReadAll(forced);
  EXPECT_NE(std::string::npos, forced_bytes.find("DJVM"));

  forced->seek(0, SEEK_SET);
  GP<DjVuDocument> forced_doc = DjVuDocument::create(forced);
  ASSERT_TRUE(forced_doc != 0);
  ASSERT_TRUE(forced_doc->wait_for_complete_init());
  EXPECT_EQ(1, forced_doc->get_pages_num());
}

TEST(DjVuDocumentExtraTest, ExpandAndDjvmDocBuildWorkForSinglePage)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());

  GP<DjVmDoc> djvm = doc->get_djvm_doc();
  ASSERT_TRUE(djvm != 0);
  ASSERT_TRUE(djvm->get_djvm_dir() != 0);

  const std::filesystem::path out_dir = MakeTempPath("expand");
  std::filesystem::create_directories(out_dir);
  const GURL codebase = GURL::Filename::UTF8(out_dir.string().c_str());
  EXPECT_NO_THROW(doc->expand(codebase, "index.djvu"));
  EXPECT_TRUE(std::filesystem::exists(out_dir / "index.djvu"));
}

TEST(DjVuDocumentExtraTest, CreateWaitFromUrlAndLookupApisWork)
{
  const std::filesystem::path bundled_path = MakeTempPath("lookup_bundle.djvu");
  WriteByteStreamToFile(MakeBundledTwoPageDjvu(), bundled_path);
  const GURL bundled_url = GURL::Filename::UTF8(bundled_path.string().c_str());

  GP<DjVuDocument> doc = DjVuDocument::create_wait(bundled_url);
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->is_init_complete());
  ASSERT_EQ(2, doc->get_pages_num());

  const GUTF8String id0 = doc->page_to_id(0);
  const GURL url0 = doc->page_to_url(0);
  EXPECT_GT(id0.length(), 0);
  EXPECT_FALSE(url0.is_empty());
  EXPECT_EQ(0, doc->id_to_page(id0));
  EXPECT_FALSE(doc->id_to_url(id0).is_empty());

  GP<DjVuFile> f0 = doc->get_djvu_file(0, false);
  ASSERT_TRUE(f0 != 0);
  EXPECT_TRUE(f0->inherits("DjVuFile"));

  GP<DjVuFile> by_id = doc->get_djvu_file(id0, false);
  EXPECT_TRUE(by_id != 0);
  GP<DjVuFile> by_url = doc->get_djvu_file(url0, false);
  EXPECT_TRUE(by_url != 0);

  GP<ByteStream> xml = ByteStream::create();
  EXPECT_NO_THROW(doc->writeDjVuXML(xml, 0, -1));
  EXPECT_GT(xml->size(), 0);

  GList<GURL> names = doc->get_url_names();
  EXPECT_GE(names.size(), 1);
}

TEST(DjVuDocumentExtraTest, CodecSettersAndStopInitPathAreReachable)
{
  const std::filesystem::path single_path = MakeTempPath("single_url.djvu");
  WriteByteStreamToFile(MakeSinglePageDjvu(), single_path);
  const GURL single_url = GURL::Filename::UTF8(single_path.string().c_str());

  DjVuDocument::set_import_codec(&FakeImportCodec);
  DjVuDocument::set_compress_codec(&FakeCompressCodec);

  GP<DjVuDocument> doc = DjVuDocument::create(single_url);
  ASSERT_TRUE(doc != 0);
  EXPECT_NO_THROW(doc->stop_init());
  EXPECT_TRUE(doc->is_init_complete());

  DjVuDocument::set_import_codec(0);
  DjVuDocument::set_compress_codec(0);

  EXPECT_THROW((void)DjVuDocument::create_wait(GURL()), GException);
}

TEST(DjVuDocumentExtraTest, IndirectDocumentLookupAndSavePathsWork)
{
  const std::filesystem::path dir = MakeTempPath("indirect_doc");
  std::filesystem::create_directories(dir);
  const GURL codebase = GURL::Filename::UTF8(dir.string().c_str());

  GP<DjVmDoc> src = DjVmDoc::create();
  GP<ByteStream> p1 = MakeSinglePageDjvu();
  GP<ByteStream> p2 = MakeSinglePageDjvu();
  src->insert_file(*p1, DjVmDir::File::PAGE, "p1.djvu", "p1.djvu", "P1");
  src->insert_file(*p2, DjVmDir::File::PAGE, "p2.djvu", "p2.djvu", "P2");
  src->expand(codebase, "index.djvu");

  const GURL index_url = GURL::Filename::UTF8((dir / "index.djvu").string().c_str());
  GP<DjVuDocument> doc = DjVuDocument::create_wait(index_url);
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());
  EXPECT_EQ(DjVuDocument::INDIRECT, doc->get_doc_type());
  ASSERT_EQ(2, doc->get_pages_num());

  const GURL p0 = doc->page_to_url(0);
  const GURL p1_url = doc->page_to_url(1);
  EXPECT_FALSE(p0.is_empty());
  EXPECT_FALSE(p1_url.is_empty());
  EXPECT_EQ(0, doc->url_to_page(p0));
  EXPECT_EQ(1, doc->url_to_page(p1_url));

  const GUTF8String id0 = doc->page_to_id(0);
  EXPECT_FALSE(doc->id_to_url(id0).is_empty());
  EXPECT_EQ(0, doc->id_to_page(id0));

  GP<ByteStream> out = ByteStream::create();
  EXPECT_NO_THROW(doc->write(out, false));
  EXPECT_GT(out->size(), 0);

  const std::filesystem::path bundled_out = MakeTempPath("from_indirect.djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_out.string().c_str());
  EXPECT_NO_THROW(doc->save_as(bundled_url, true));
  EXPECT_TRUE(std::filesystem::exists(bundled_out));
}

TEST(DjVuDocumentExtraTest, VirtualPortLookupOverloadsAreCallable)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeBundledTwoPageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());
  ASSERT_EQ(2, doc->get_pages_num());

  const GUTF8String id0 = doc->page_to_id(0);
  const DjVuPort *source = (const DjVuPort *)(DjVuDocument *)doc;

  const GURL u0 = doc->id_to_url(source, id0);
  EXPECT_FALSE(u0.is_empty());

  GP<DjVuFile> f0 = doc->id_to_file(source, id0);
  EXPECT_TRUE(f0 != 0);

  GP<DataPool> pool = doc->request_data(source, u0);
  EXPECT_TRUE(pool != 0);
}

TEST(DjVuDocumentExtraTest, SinglePageGetUrlNamesCoversRecursivePath)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());
  ASSERT_EQ(DjVuDocument::SINGLE_PAGE, doc->get_doc_type());

  GList<GURL> names1;
  GList<GURL> names2;
  EXPECT_NO_THROW(names1 = doc->get_url_names());
  EXPECT_NO_THROW(names2 = doc->get_url_names());
  EXPECT_EQ(names1.size(), names2.size());
}

TEST(DjVuDocumentExtraTest, NotifyFileFlagsChangedWithCacheTriggersCachePath)
{
  GP<DjVuFileCache> cache = DjVuFileCache::create();
  ASSERT_TRUE(cache != 0);

  GP<DjVuDocument> doc = DjVuDocument::create(MakeBundledTwoPageDjvu(), 0, (DjVuFileCache *)cache);
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());
  ASSERT_EQ(2, doc->get_pages_num());

  GP<DjVuFile> f0 = doc->get_djvu_file(0, false);
  ASSERT_TRUE(f0 != 0);
  EXPECT_NO_THROW(doc->notify_file_flags_changed((const DjVuFile *)f0, DjVuFile::DECODE_OK, 0));
}

TEST(DjVuDocumentExtraTest, ReferenceIndirectFixtureSupportsLookupXmlAndBundledSaveAs)
{
  const std::optional<std::filesystem::path> fixture =
    FindReferenceFixtureFile("mp3_indirect_mixed_layers/index.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> doc = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->is_init_complete());
  ASSERT_EQ(DjVuDocument::INDIRECT, doc->get_doc_type());
  ASSERT_EQ(3, doc->get_pages_num());

  for (int i = 0; i < 3; ++i)
  {
    const GUTF8String id = doc->page_to_id(i);
    const GURL url = doc->page_to_url(i);
    EXPECT_GT(id.length(), 0);
    EXPECT_FALSE(url.is_empty());
    EXPECT_EQ(i, doc->id_to_page(id));
    EXPECT_EQ(i, doc->url_to_page(url));
    EXPECT_FALSE(doc->id_to_url(id).is_empty());
    EXPECT_TRUE(doc->get_djvu_file(id, false) != 0);
    EXPECT_TRUE(doc->get_djvu_file(url, false) != 0);
  }

  const GList<GURL> names = doc->get_url_names();
  EXPECT_GE(names.size(), 3);

  GP<ByteStream> xml = ByteStream::create();
  EXPECT_NO_THROW(doc->writeDjVuXML(xml, 1, 2));
  EXPECT_GT(xml->size(), 0);

  const std::filesystem::path bundled_out = MakeTempPath("ref_mp3_bundle.djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_out.string().c_str());
  EXPECT_NO_THROW(doc->save_as(bundled_url, true));
  EXPECT_TRUE(std::filesystem::exists(bundled_out));
}

TEST(DjVuDocumentExtraTest, ReferenceSharedResourceFixtureExpandsAndSavesIndirect)
{
  const std::optional<std::filesystem::path> fixture =
    FindReferenceFixtureFile("mp2_bundled_shared_resources.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> doc = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->is_init_complete());
  ASSERT_EQ(DjVuDocument::BUNDLED, doc->get_doc_type());
  ASSERT_EQ(2, doc->get_pages_num());

  const GList<GURL> names = doc->get_url_names();
  EXPECT_GE(names.size(), 3);

  GP<DjVuFile> p0 = doc->get_djvu_file(0, false);
  GP<DjVuFile> p1 = doc->get_djvu_file(1, false);
  ASSERT_TRUE(p0 != 0);
  ASSERT_TRUE(p1 != 0);
  EXPECT_FALSE(p0->get_url().is_empty());
  EXPECT_FALSE(p1->get_url().is_empty());

  const std::filesystem::path expand_dir = MakeTempPath("shared_expand");
  std::filesystem::create_directories(expand_dir);
  const GURL expand_url = GURL::Filename::UTF8(expand_dir.string().c_str());
  EXPECT_NO_THROW(doc->expand(expand_url, "index.djvu"));
  EXPECT_TRUE(std::filesystem::exists(expand_dir / "index.djvu"));

  const std::filesystem::path indirect_dir = MakeTempPath("shared_save_indirect");
  std::filesystem::create_directories(indirect_dir);
  const std::filesystem::path indirect_idx = indirect_dir / "index.djvu";
  const GURL indirect_url = GURL::Filename::UTF8(indirect_idx.string().c_str());
  EXPECT_NO_THROW(doc->save_as(indirect_url, false));
  EXPECT_TRUE(std::filesystem::exists(indirect_idx));
}

TEST(DjVuDocumentExtraTest, ReferenceBundledFixtureWriteRoundtripAndIndirectReopenWork)
{
  const std::optional<std::filesystem::path> fixture =
    FindReferenceFixtureFile("mp3_bundled_mixed_layers.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> doc = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->is_init_complete());
  ASSERT_EQ(DjVuDocument::BUNDLED, doc->get_doc_type());
  ASSERT_EQ(3, doc->get_pages_num());

  GP<ByteStream> bundled = ByteStream::create();
  EXPECT_NO_THROW(doc->write(bundled, true));
  EXPECT_GT(bundled->size(), 0);
  bundled->seek(0, SEEK_SET);

  GP<DjVuDocument> reopened = DjVuDocument::create(bundled);
  ASSERT_TRUE(reopened != 0);
  ASSERT_TRUE(reopened->wait_for_complete_init());
  EXPECT_EQ(DjVuDocument::BUNDLED, reopened->get_doc_type());
  EXPECT_EQ(3, reopened->get_pages_num());
  EXPECT_GT(reopened->page_to_id(2).length(), 0);

  const std::filesystem::path indirect_dir = MakeTempPath("ref_bundle_to_indirect");
  std::filesystem::create_directories(indirect_dir);
  const std::filesystem::path indirect_idx = indirect_dir / "index.djvu";
  const GURL indirect_url = GURL::Filename::UTF8(indirect_idx.string().c_str());
  EXPECT_NO_THROW(doc->save_as(indirect_url, false));
  EXPECT_TRUE(std::filesystem::exists(indirect_idx));

  GP<DjVuDocument> indirect = DjVuDocument::create_wait(indirect_url);
  ASSERT_TRUE(indirect != 0);
  ASSERT_TRUE(indirect->is_init_complete());
  EXPECT_EQ(DjVuDocument::INDIRECT, indirect->get_doc_type());
  EXPECT_EQ(3, indirect->get_pages_num());
}

TEST(DjVuDocumentExtraTest, ReferenceIndirectFixtureWriteRoundtripAndXmlSubsetWork)
{
  const std::optional<std::filesystem::path> fixture =
    FindReferenceFixtureFile("mp3_indirect_mixed_layers/index.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> doc = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->is_init_complete());
  ASSERT_EQ(DjVuDocument::INDIRECT, doc->get_doc_type());
  ASSERT_EQ(3, doc->get_pages_num());

  GP<ByteStream> bundled = ByteStream::create();
  EXPECT_NO_THROW(doc->write(bundled, false));
  EXPECT_GT(bundled->size(), 0);
  bundled->seek(0, SEEK_SET);

  GP<DjVuDocument> reopened = DjVuDocument::create(bundled);
  ASSERT_TRUE(reopened != 0);
  ASSERT_TRUE(reopened->wait_for_complete_init());
  EXPECT_EQ(3, reopened->get_pages_num());

  GP<ByteStream> xml = ByteStream::create();
  EXPECT_NO_THROW(doc->writeDjVuXML(xml, 0, 1));
  EXPECT_GT(xml->size(), 0);

  const std::filesystem::path bundled_out = MakeTempPath("ref_indirect_roundtrip.djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_out.string().c_str());
  EXPECT_NO_THROW(doc->save_as(bundled_url, true));
  EXPECT_TRUE(std::filesystem::exists(bundled_out));

  GP<DjVuDocument> bundled_doc = DjVuDocument::create_wait(bundled_url);
  ASSERT_TRUE(bundled_doc != 0);
  ASSERT_TRUE(bundled_doc->is_init_complete());
  EXPECT_EQ(DjVuDocument::BUNDLED, bundled_doc->get_doc_type());
  EXPECT_EQ(3, bundled_doc->get_pages_num());
}

TEST(DjVuDocumentExtraTest, ReferenceBundledFixtureExpandReopenAndBundleRoundtripPreserveSignals)
{
  const std::optional<std::filesystem::path> fixture =
    FindReferenceFixtureFile("mp3_bundled_mixed_layers.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocument> doc = DjVuDocument::create_wait(fixture_url);
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->is_init_complete());
  ASSERT_EQ(3, doc->get_pages_num());
  const std::vector<PageSignals> source = CollectPageSignals(doc);
  ASSERT_EQ(3u, source.size());

  const std::filesystem::path expand_dir = MakeTempPath("ref_bundle_expand_roundtrip");
  std::filesystem::create_directories(expand_dir);
  const GURL expand_url = GURL::Filename::UTF8(expand_dir.string().c_str());
  EXPECT_NO_THROW(doc->expand(expand_url, "index.djvu"));
  const std::filesystem::path expanded_index = expand_dir / "index.djvu";
  EXPECT_TRUE(std::filesystem::exists(expanded_index));

  GP<DjVuDocument> expanded =
      DjVuDocument::create_wait(GURL::Filename::UTF8(expanded_index.string().c_str()));
  ASSERT_TRUE(expanded != 0);
  ASSERT_TRUE(expanded->is_init_complete());
  EXPECT_EQ(DjVuDocument::INDIRECT, expanded->get_doc_type());
  EXPECT_EQ(source, CollectPageSignals(expanded));

  GP<ByteStream> xml = ByteStream::create();
  EXPECT_NO_THROW(expanded->writeDjVuXML(xml, 0, -1));
  xml->seek(0, SEEK_SET);
  const std::string xml_text = (const char *)xml->getAsUTF8();
  EXPECT_EQ(3, CountSubstring(xml_text, "<OBJECT"));

  const std::filesystem::path bundled_out = MakeTempPath("ref_expand_rebundle.djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_out.string().c_str());
  EXPECT_NO_THROW(expanded->save_as(bundled_url, true));
  EXPECT_TRUE(std::filesystem::exists(bundled_out));

  GP<DjVuDocument> rebundled = DjVuDocument::create_wait(bundled_url);
  ASSERT_TRUE(rebundled != 0);
  ASSERT_TRUE(rebundled->is_init_complete());
  EXPECT_EQ(DjVuDocument::BUNDLED, rebundled->get_doc_type());
  EXPECT_EQ(source, CollectPageSignals(rebundled));
}

TEST(DjVuDocumentExtraTest, ReferenceSharedResourceFixtureExpandReopenPreservesLookupsAndSignals)
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
  const std::vector<PageSignals> source = CollectPageSignals(doc);
  ASSERT_EQ(2u, source.size());

  const std::filesystem::path expand_dir = MakeTempPath("ref_shared_expand_reopen");
  std::filesystem::create_directories(expand_dir);
  const GURL expand_url = GURL::Filename::UTF8(expand_dir.string().c_str());
  EXPECT_NO_THROW(doc->expand(expand_url, "index.djvu"));
  const std::filesystem::path expanded_index = expand_dir / "index.djvu";
  EXPECT_TRUE(std::filesystem::exists(expanded_index));

  GP<DjVuDocument> expanded =
      DjVuDocument::create_wait(GURL::Filename::UTF8(expanded_index.string().c_str()));
  ASSERT_TRUE(expanded != 0);
  ASSERT_TRUE(expanded->is_init_complete());
  EXPECT_EQ(DjVuDocument::INDIRECT, expanded->get_doc_type());
  EXPECT_EQ(source, CollectPageSignals(expanded));

  for (int i = 0; i < expanded->get_pages_num(); ++i)
  {
    const GUTF8String id = expanded->page_to_id(i);
    const GURL url = expanded->page_to_url(i);
    EXPECT_GT(id.length(), 0);
    EXPECT_FALSE(url.is_empty());
    EXPECT_EQ(i, expanded->id_to_page(id));
    EXPECT_EQ(i, expanded->url_to_page(url));
    EXPECT_TRUE(expanded->get_djvu_file(id, false) != 0);
    EXPECT_TRUE(expanded->get_djvu_file(url, false) != 0);
  }
}

TEST(DjVuDocumentExtraTest, SequentialExpandSaveReopenKeepsMappingsForBundledFixture)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("mp3_bundled_mixed_layers.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  GP<DjVuDocument> source =
      DjVuDocument::create_wait(GURL::Filename::UTF8(fixture->string().c_str()));
  ASSERT_TRUE(source != 0);
  ASSERT_TRUE(source->is_init_complete());
  ASSERT_EQ(3, source->get_pages_num());

  const std::vector<PageSignals> source_signals = CollectPageSignals(source);
  const MappingSnapshot source_mapping = CollectMappingSnapshot(source);

  const std::filesystem::path expand_dir1 = MakeTempPath("seq_expand_1");
  std::filesystem::create_directories(expand_dir1);
  const GURL expand_url1 = GURL::Filename::UTF8(expand_dir1.string().c_str());
  EXPECT_NO_THROW(source->expand(expand_url1, "index.djvu"));
  const std::filesystem::path index1 = expand_dir1 / "index.djvu";

  GP<DjVuDocument> indirect1 =
      DjVuDocument::create_wait(GURL::Filename::UTF8(index1.string().c_str()));
  ASSERT_TRUE(indirect1 != 0);
  ASSERT_TRUE(indirect1->is_init_complete());
  EXPECT_EQ(source_signals, CollectPageSignals(indirect1));
  EXPECT_EQ(source_mapping, CollectMappingSnapshot(indirect1));

  const std::filesystem::path bundled_path = MakeTempPath("seq_bundle.djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_path.string().c_str());
  EXPECT_NO_THROW(indirect1->save_as(bundled_url, true));

  GP<DjVuDocument> bundled =
      DjVuDocument::create_wait(GURL::Filename::UTF8(bundled_path.string().c_str()));
  ASSERT_TRUE(bundled != 0);
  ASSERT_TRUE(bundled->is_init_complete());
  EXPECT_EQ(source_signals, CollectPageSignals(bundled));
  EXPECT_EQ(source_mapping, CollectMappingSnapshot(bundled));

  const std::filesystem::path expand_dir2 = MakeTempPath("seq_expand_2");
  std::filesystem::create_directories(expand_dir2);
  const GURL expand_url2 = GURL::Filename::UTF8(expand_dir2.string().c_str());
  EXPECT_NO_THROW(bundled->expand(expand_url2, "index.djvu"));
  const std::filesystem::path index2 = expand_dir2 / "index.djvu";

  GP<DjVuDocument> indirect2 =
      DjVuDocument::create_wait(GURL::Filename::UTF8(index2.string().c_str()));
  ASSERT_TRUE(indirect2 != 0);
  ASSERT_TRUE(indirect2->is_init_complete());
  EXPECT_EQ(source_signals, CollectPageSignals(indirect2));
  EXPECT_EQ(source_mapping, CollectMappingSnapshot(indirect2));
}

TEST(DjVuDocumentExtraTest, SequentialExpandSaveReopenKeepsMappingsForSharedResourceFixture)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("mp2_bundled_shared_resources.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  GP<DjVuDocument> source =
      DjVuDocument::create_wait(GURL::Filename::UTF8(fixture->string().c_str()));
  ASSERT_TRUE(source != 0);
  ASSERT_TRUE(source->is_init_complete());
  ASSERT_EQ(2, source->get_pages_num());

  const std::vector<PageSignals> source_signals = CollectPageSignals(source);
  const MappingSnapshot source_mapping = CollectMappingSnapshot(source);

  const std::filesystem::path expand_dir = MakeTempPath("shared_seq_expand");
  std::filesystem::create_directories(expand_dir);
  const GURL expand_url = GURL::Filename::UTF8(expand_dir.string().c_str());
  EXPECT_NO_THROW(source->expand(expand_url, "index.djvu"));
  const std::filesystem::path index = expand_dir / "index.djvu";

  GP<DjVuDocument> indirect =
      DjVuDocument::create_wait(GURL::Filename::UTF8(index.string().c_str()));
  ASSERT_TRUE(indirect != 0);
  ASSERT_TRUE(indirect->is_init_complete());
  EXPECT_EQ(source_signals, CollectPageSignals(indirect));
  EXPECT_EQ(source_mapping, CollectMappingSnapshot(indirect));

  const std::filesystem::path rebundled_path = MakeTempPath("shared_seq_bundle.djvu");
  const GURL rebundled_url = GURL::Filename::UTF8(rebundled_path.string().c_str());
  EXPECT_NO_THROW(indirect->save_as(rebundled_url, true));

  GP<DjVuDocument> rebundled =
      DjVuDocument::create_wait(GURL::Filename::UTF8(rebundled_path.string().c_str()));
  ASSERT_TRUE(rebundled != 0);
  ASSERT_TRUE(rebundled->is_init_complete());
  EXPECT_EQ(source_signals, CollectPageSignals(rebundled));
  EXPECT_EQ(source_mapping, CollectMappingSnapshot(rebundled));
}
