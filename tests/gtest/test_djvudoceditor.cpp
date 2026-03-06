#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "ByteStream.h"
#include "DjVuDocEditor.h"
#include "DjVuFile.h"
#include "DjVuInfo.h"
#include "DjVmDoc.h"
#include "DjVmNav.h"
#include "IFFByteStream.h"

namespace {

class TestableDjVuDocEditor : public DjVuDocEditor
{
public:
  static GP<TestableDjVuDocEditor> create_wait_for_test(const GURL &url)
  {
    GP<TestableDjVuDocEditor> editor = new TestableDjVuDocEditor();
    editor->init(url);
    return editor;
  }

  GP<DataPool> call_get_thumbnail(int page_num, bool dont_decode)
  {
    return get_thumbnail(page_num, dont_decode);
  }
};

std::filesystem::path MakeTempPath(const char *suffix)
{
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path() /
         ("djvu_gtest_doceditor_" + std::to_string(now) + "_" + suffix);
}

std::filesystem::path MakeTempDir(const char *suffix)
{
  const std::filesystem::path path = MakeTempPath(suffix);
  std::filesystem::create_directories(path);
  return path;
}

std::vector<char> ReadAllBytes(const GP<ByteStream> &bs)
{
  GP<ByteStream> in = bs;
  in->seek(0, SEEK_SET);
  const long n = in->size();
  std::vector<char> data(static_cast<size_t>(n > 0 ? n : 0));
  if (n > 0)
    in->readall(data.data(), static_cast<size_t>(n));
  return data;
}

std::vector<char> MakeSinglePageDjvuBytes(int w = 96, int h = 64)
{
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  iff->put_chunk("FORM:DJVU");

  iff->put_chunk("INFO");
  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = w;
  info->height = h;
  info->dpi = 300;
  info->gamma = 2.2;
  info->version = 24;
  info->encode(*iff->get_bytestream());
  iff->close_chunk();

  iff->close_chunk();
  return ReadAllBytes(bs);
}

GP<ByteStream> MakeByteStreamFromBytes(const std::vector<char> &bytes)
{
  GP<ByteStream> bs = ByteStream::create();
  if (!bytes.empty())
    bs->writall(bytes.data(), bytes.size());
  bs->seek(0, SEEK_SET);
  return bs;
}

std::vector<char> MakeBundledDjvmBytes()
{
  const std::vector<char> p1 = MakeSinglePageDjvuBytes(72, 48);
  const std::vector<char> p2 = MakeSinglePageDjvuBytes(88, 64);
  GP<DjVmDoc> doc = DjVmDoc::create();
  doc->insert_file(*MakeByteStreamFromBytes(p1), DjVmDir::File::PAGE, "p1.djvu", "p1.djvu",
                   "Page 1");
  doc->insert_file(*MakeByteStreamFromBytes(p2), DjVmDir::File::PAGE, "p2.djvu", "p2.djvu",
                   "Page 2");

  GP<ByteStream> bs = ByteStream::create();
  doc->write(bs);
  return ReadAllBytes(bs);
}

std::vector<char> MakeBundledDjvmBytesWithPages(int pages)
{
  GP<DjVmDoc> doc = DjVmDoc::create();
  for (int i = 0; i < pages; ++i)
  {
    const int w = 72 + i * 8;
    const int h = 48 + i * 8;
    const std::string name = "p" + std::to_string(i + 1) + ".djvu";
    const std::string title = "Page " + std::to_string(i + 1);
    doc->insert_file(*MakeByteStreamFromBytes(MakeSinglePageDjvuBytes(w, h)), DjVmDir::File::PAGE,
                     name.c_str(), name.c_str(), title.c_str());
  }
  GP<ByteStream> bs = ByteStream::create();
  doc->write(bs);
  return ReadAllBytes(bs);
}

void WriteBytes(const std::filesystem::path &path, const std::vector<char> &bytes)
{
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

bool CountThumbCallbacks(int, void *data)
{
  int *count = static_cast<int *>(data);
  if (count)
    ++(*count);
  return false;
}

void CountProgressCallbacks(float, void *data)
{
  int *count = static_cast<int *>(data);
  if (count)
    ++(*count);
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

struct EditorSignals
{
  std::vector<std::string> page_ids;
  std::vector<bool> has_text;
  std::vector<bool> has_anno;
};

EditorSignals CollectEditorSignals(DjVuDocument *doc)
{
  EditorSignals signals;
  for (int i = 0; i < doc->get_pages_num(); ++i)
  {
    const GUTF8String id = doc->page_to_id(i);
    GP<DjVuFile> file = doc->get_djvu_file(i, false);
    EXPECT_TRUE(file != 0);
    if (!file)
      continue;
    EXPECT_NO_THROW(file->resume_decode(true));
    signals.page_ids.emplace_back((const char *)id);
    signals.has_text.push_back(file->contains_text());
    signals.has_anno.push_back(file->contains_anno());
  }
  return signals;
}

}  // namespace

TEST(DjVuDocEditorTest, CreateEmptyEditorInitializes)
{
  GP<DjVuDocEditor> editor = DjVuDocEditor::create_wait();
  ASSERT_TRUE(editor != 0);

  EXPECT_TRUE(editor->is_init_complete());
  EXPECT_TRUE(editor->is_init_ok());
  EXPECT_TRUE(editor->inherits("DjVuDocEditor"));
  EXPECT_TRUE(editor->inherits("DjVuDocument"));
  EXPECT_TRUE(editor->inherits("DjVuPort"));
}

TEST(DjVuDocEditorTest, EmptyEditorSaveCapabilitiesMatchUnknownOriginalType)
{
  GP<DjVuDocEditor> editor = DjVuDocEditor::create_wait();
  ASSERT_TRUE(editor != 0);

  EXPECT_EQ(DjVuDocument::UNKNOWN_TYPE, editor->get_orig_doc_type());
  EXPECT_FALSE(editor->can_be_saved());
  EXPECT_EQ(DjVuDocument::UNKNOWN_TYPE, editor->get_save_doc_type());
}

TEST(DjVuDocEditorTest, DocUrlIsPresent)
{
  GP<DjVuDocEditor> editor = DjVuDocEditor::create_wait();
  ASSERT_TRUE(editor != 0);

  GURL url = editor->get_doc_url();
  EXPECT_FALSE(url.is_empty());
}

TEST(DjVuDocEditorTest, OpenBundledEditorInsertGroupRemoveAndSaveAsIndirect)
{
  const std::filesystem::path source_doc_path = MakeTempPath("source.djvu");
  WriteBytes(source_doc_path, MakeBundledDjvmBytes());
  const GURL source_doc_url = GURL::Filename::UTF8(source_doc_path.string().c_str());

  const std::filesystem::path add1_path = MakeTempPath("add1.djvu");
  const std::filesystem::path add2_path = MakeTempPath("add2.djvu");
  WriteBytes(add1_path, MakeSinglePageDjvuBytes(120, 90));
  WriteBytes(add2_path, MakeSinglePageDjvuBytes(90, 120));
  const GURL add1_url = GURL::Filename::UTF8(add1_path.string().c_str());
  const GURL add2_url = GURL::Filename::UTF8(add2_path.string().c_str());

  GP<DjVuDocEditor> editor = DjVuDocEditor::create_wait(source_doc_url);
  ASSERT_TRUE(editor != 0);
  ASSERT_TRUE(editor->is_init_ok());
  EXPECT_EQ(DjVuDocument::BUNDLED, editor->get_orig_doc_type());
  ASSERT_GE(editor->get_pages_num(), 2);
  EXPECT_TRUE(editor->can_be_saved());

  GList<GURL> group;
  group.append(add1_url);
  group.append(add2_url);
  EXPECT_NO_THROW(editor->insert_group(group, 1));
  ASSERT_GE(editor->get_pages_num(), 4);

  EXPECT_NO_THROW(editor->insert_page(add1_url, -1));
  ASSERT_GE(editor->get_pages_num(), 5);

  EXPECT_NO_THROW(editor->move_page(0, editor->get_pages_num() - 1));

  GList<int> remove_pages;
  remove_pages.append(1);
  remove_pages.append(2);
  EXPECT_NO_THROW(editor->remove_pages(remove_pages, true));
  ASSERT_GE(editor->get_pages_num(), 3);

  EXPECT_NO_THROW(editor->remove_file(editor->page_to_id(0), false));
  ASSERT_GE(editor->get_pages_num(), 2);

  const std::filesystem::path indirect_dir = MakeTempDir("indirect");
  const std::filesystem::path indirect_index = indirect_dir / "index.djvu";
  const GURL indirect_url = GURL::Filename::UTF8(indirect_index.string().c_str());
  EXPECT_NO_THROW(editor->save_as(indirect_url, false));
  EXPECT_TRUE(std::filesystem::exists(indirect_index));
  EXPECT_EQ(DjVuDocument::INDIRECT, editor->get_orig_doc_type());
  EXPECT_EQ(DjVuDocument::INDIRECT, editor->get_save_doc_type());

  const std::filesystem::path bundled_path = MakeTempPath("saved_bundled.djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_path.string().c_str());
  EXPECT_NO_THROW(editor->save_as(bundled_url, true));
  EXPECT_TRUE(std::filesystem::exists(bundled_path));
  EXPECT_NO_THROW(editor->save());
}

TEST(DjVuDocEditorTest, MovePagesAndRenameApisWork)
{
  const std::filesystem::path source_doc_path = MakeTempPath("move_rename_source.djvu");
  WriteBytes(source_doc_path, MakeBundledDjvmBytesWithPages(4));
  const GURL source_doc_url = GURL::Filename::UTF8(source_doc_path.string().c_str());

  GP<DjVuDocEditor> editor = DjVuDocEditor::create_wait(source_doc_url);
  ASSERT_TRUE(editor != 0);
  ASSERT_TRUE(editor->is_init_ok());
  ASSERT_EQ(4, editor->get_pages_num());

  GList<int> move_list;
  move_list.append(0);
  move_list.append(2);
  EXPECT_NO_THROW(editor->move_pages(move_list, 1));
  EXPECT_NO_THROW(editor->move_pages(move_list, -2));

  const GUTF8String id0 = editor->page_to_id(0);
  ASSERT_TRUE(id0.length() > 0);
  EXPECT_NO_THROW(editor->set_file_name(id0, "renamed0.djvu"));
  EXPECT_NO_THROW(editor->set_file_title(id0, "Renamed Zero"));
  EXPECT_NO_THROW(editor->set_page_name(1, "renamed1.djvu"));
  EXPECT_NO_THROW(editor->set_page_title(1, "Renamed One"));

  GP<DjVmDir> dir = editor->get_djvm_dir();
  ASSERT_TRUE(dir != 0);
  GP<DjVmDir::File> rec0 = dir->id_to_file(id0);
  ASSERT_TRUE(rec0 != 0);
  EXPECT_STREQ("renamed0.djvu", (const char *)rec0->get_save_name());
  EXPECT_STREQ("Renamed Zero", (const char *)rec0->get_title());
}

TEST(DjVuDocEditorTest, InsertFileByParentIdCreatesAndRemovesInclude)
{
  const std::filesystem::path source_doc_path = MakeTempPath("incl_parent_source.djvu");
  WriteBytes(source_doc_path, MakeBundledDjvmBytes());
  const GURL source_doc_url = GURL::Filename::UTF8(source_doc_path.string().c_str());

  const std::filesystem::path child_path = MakeTempPath("incl_child.djvu");
  WriteBytes(child_path, MakeSinglePageDjvuBytes(128, 96));
  const GURL child_url = GURL::Filename::UTF8(child_path.string().c_str());

  GP<DjVuDocEditor> editor = DjVuDocEditor::create_wait(source_doc_url);
  ASSERT_TRUE(editor != 0);
  ASSERT_TRUE(editor->is_init_ok());
  ASSERT_GE(editor->get_pages_num(), 1);

  const GUTF8String parent_id = editor->page_to_id(0);
  ASSERT_TRUE(parent_id.length() > 0);

  GUTF8String new_include_id;
  EXPECT_NO_THROW(new_include_id = editor->insert_file(child_url, parent_id, 1, 0));
  EXPECT_TRUE(new_include_id.length() > 0);

  GP<DjVuFile> parent = editor->get_djvu_file(parent_id, false);
  ASSERT_TRUE(parent != 0);
  EXPECT_TRUE(parent->contains_chunk("INCL"));

  EXPECT_NO_THROW(editor->remove_file(new_include_id, true));
}

TEST(DjVuDocEditorTest, WriteAndSavePagesApisAreReachable)
{
  const std::filesystem::path source_doc_path = MakeTempPath("write_source.djvu");
  WriteBytes(source_doc_path, MakeBundledDjvmBytesWithPages(3));
  const GURL source_doc_url = GURL::Filename::UTF8(source_doc_path.string().c_str());

  GP<DjVuDocEditor> editor = DjVuDocEditor::create_wait(source_doc_url);
  ASSERT_TRUE(editor != 0);
  ASSERT_TRUE(editor->is_init_ok());
  ASSERT_EQ(3, editor->get_pages_num());

  GP<ByteStream> out_force = ByteStream::create();
  EXPECT_NO_THROW(editor->write(out_force, true));
  EXPECT_GT(out_force->size(), 0);

  GP<ByteStream> out_reserved = ByteStream::create();
  GMap<GUTF8String, void *> reserved;
  reserved["reserved.djvu"] = 0;
  EXPECT_NO_THROW(editor->write(out_reserved, reserved));
  EXPECT_GT(out_reserved->size(), 0);

  GP<ByteStream> out_pages = ByteStream::create();
  GList<int> page_list;
  page_list.append(0);
  page_list.append(2);
  EXPECT_NO_THROW(editor->save_pages_as(out_pages, page_list));
  EXPECT_GT(out_pages->size(), 0);
}

TEST(DjVuDocEditorTest, ThumbnailAnnoAndNavApisAreReachable)
{
  const std::filesystem::path source_doc_path = MakeTempPath("thumbs_source.djvu");
  WriteBytes(source_doc_path, MakeBundledDjvmBytesWithPages(2));
  const GURL source_doc_url = GURL::Filename::UTF8(source_doc_path.string().c_str());

  GP<TestableDjVuDocEditor> editor = TestableDjVuDocEditor::create_wait_for_test(source_doc_url);
  ASSERT_TRUE(editor != 0);
  ASSERT_TRUE(editor->is_init_ok());
  ASSERT_EQ(2, editor->get_pages_num());

  EXPECT_EQ(0, editor->get_thumbnails_num());
  EXPECT_EQ(-1, editor->get_thumbnails_size());

  EXPECT_NO_THROW(editor->generate_thumbnails(64, 0));
  int thumb_cb_count = 0;
  EXPECT_NO_THROW(editor->generate_thumbnails(64, &CountThumbCallbacks, &thumb_cb_count));
  EXPECT_EQ(2, thumb_cb_count);

  EXPECT_GE(editor->get_thumbnails_num(), 1);
  EXPECT_GT(editor->get_thumbnails_size(), 0);
  GP<DataPool> thumb = editor->call_get_thumbnail(0, false);
  EXPECT_TRUE(thumb != 0);

  GP<ByteStream> out = ByteStream::create();
  EXPECT_NO_THROW(editor->write(out, true));
  EXPECT_GT(out->size(), 0);

  int progress_count = 0;
  EXPECT_NO_THROW(editor->simplify_anno(&CountProgressCallbacks, &progress_count));
  EXPECT_NO_THROW(editor->create_shared_anno_file(&CountProgressCallbacks, &progress_count));
  EXPECT_GT(progress_count, 0);

  GP<DjVuFile> shared = editor->get_shared_anno_file();
  EXPECT_TRUE(shared != 0);

  GP<DjVmNav> nav = DjVmNav::create();
  nav->append(DjVmNav::DjVuBookMark::create(0, "Root", "#root"));
  EXPECT_NO_THROW(editor->set_djvm_nav(nav));

  EXPECT_NO_THROW(editor->remove_thumbnails());
  EXPECT_EQ(0, editor->get_thumbnails_num());
}

TEST(DjVuDocEditorTest, ReferenceBundledFixtureCanRoundtripThroughIndirectAndPreserveSignals)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("mp3_bundled_mixed_layers.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocEditor> editor = DjVuDocEditor::create_wait(fixture_url);
  ASSERT_TRUE(editor != 0);
  ASSERT_TRUE(editor->is_init_ok());
  ASSERT_EQ(3, editor->get_pages_num());

  const EditorSignals source = CollectEditorSignals((DjVuDocument *)editor);

  const std::filesystem::path indirect_dir = MakeTempDir("ref_editor_indirect");
  const std::filesystem::path indirect_index = indirect_dir / "index.djvu";
  const GURL indirect_url = GURL::Filename::UTF8(indirect_index.string().c_str());
  EXPECT_NO_THROW(editor->save_as(indirect_url, false));
  EXPECT_TRUE(std::filesystem::exists(indirect_index));

  GP<DjVuDocument> indirect = DjVuDocument::create_wait(indirect_url);
  ASSERT_TRUE(indirect != 0);
  ASSERT_TRUE(indirect->is_init_complete());
  EXPECT_EQ(source.page_ids, CollectEditorSignals((DjVuDocument *)indirect).page_ids);
  EXPECT_EQ(source.has_text, CollectEditorSignals((DjVuDocument *)indirect).has_text);
  EXPECT_EQ(source.has_anno, CollectEditorSignals((DjVuDocument *)indirect).has_anno);

  const std::filesystem::path bundled_path = MakeTempPath("ref_editor_bundle.djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_path.string().c_str());
  EXPECT_NO_THROW(editor->save_as(bundled_url, true));
  EXPECT_TRUE(std::filesystem::exists(bundled_path));

  GP<DjVuDocument> bundled = DjVuDocument::create_wait(bundled_url);
  ASSERT_TRUE(bundled != 0);
  ASSERT_TRUE(bundled->is_init_complete());
  EXPECT_EQ(source.page_ids, CollectEditorSignals((DjVuDocument *)bundled).page_ids);
  EXPECT_EQ(source.has_text, CollectEditorSignals((DjVuDocument *)bundled).has_text);
  EXPECT_EQ(source.has_anno, CollectEditorSignals((DjVuDocument *)bundled).has_anno);
}

TEST(DjVuDocEditorTest, ReferenceIndirectFixtureRenameAndSavePreservesMappingAccess)
{
  const std::optional<std::filesystem::path> fixture =
      FindReferenceFixtureFile("mp3_indirect_mixed_layers/index.djvu");
  if (!fixture)
    GTEST_SKIP() << "reference fixture not found";

  const GURL fixture_url = GURL::Filename::UTF8(fixture->string().c_str());
  GP<DjVuDocEditor> editor = DjVuDocEditor::create_wait(fixture_url);
  ASSERT_TRUE(editor != 0);
  ASSERT_TRUE(editor->is_init_ok());
  ASSERT_EQ(3, editor->get_pages_num());

  const GUTF8String old_id = editor->page_to_id(1);
  ASSERT_TRUE(old_id.length() > 0);
  EXPECT_NO_THROW(editor->set_page_name(1, "renamed-page-2.djvu"));
  EXPECT_NO_THROW(editor->set_page_title(1, "Renamed Page Two"));

  const GUTF8String current_id = editor->page_to_id(1);
  EXPECT_EQ(old_id, current_id);
  EXPECT_EQ(1, editor->id_to_page(current_id));
  EXPECT_TRUE(editor->get_djvu_file(current_id, false) != 0);

  GP<DjVmDir> dir = editor->get_djvm_dir();
  ASSERT_TRUE(dir != 0);
  GP<DjVmDir::File> rec = dir->page_to_file(1);
  ASSERT_TRUE(rec != 0);
  EXPECT_STREQ("renamed-page-2.djvu", (const char *)rec->get_save_name());
  EXPECT_STREQ("Renamed Page Two", (const char *)rec->get_title());

  const std::filesystem::path bundled_path = MakeTempPath("ref_editor_renamed_bundle.djvu");
  const GURL bundled_url = GURL::Filename::UTF8(bundled_path.string().c_str());
  EXPECT_NO_THROW(editor->save_as(bundled_url, true));

  GP<DjVuDocument> bundled = DjVuDocument::create_wait(bundled_url);
  ASSERT_TRUE(bundled != 0);
  ASSERT_TRUE(bundled->is_init_complete());
  ASSERT_EQ(3, bundled->get_pages_num());
  EXPECT_EQ(1, bundled->id_to_page(bundled->page_to_id(1)));
  EXPECT_TRUE(bundled->get_djvu_file(bundled->page_to_id(1), false) != 0);
}
