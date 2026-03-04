#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DjVuDocument.h"
#include "DjVuInfo.h"
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
