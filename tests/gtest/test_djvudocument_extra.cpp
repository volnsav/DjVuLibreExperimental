#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
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
