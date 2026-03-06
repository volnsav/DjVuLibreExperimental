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
  info->width = 80;
  info->height = 60;
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

TEST(DjVuDocumentTest, InitFromByteStreamCompletesAndHasSinglePage)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu());
  ASSERT_TRUE(doc != 0);

  EXPECT_TRUE(doc->wait_for_complete_init());
  EXPECT_TRUE(doc->is_init_complete());
  EXPECT_FALSE(doc->is_init_failed());

  EXPECT_EQ(1, doc->get_pages_num());
  EXPECT_EQ(1, doc->wait_get_pages_num());
  EXPECT_EQ(DjVuDocument::SINGLE_PAGE, doc->get_doc_type());
}

TEST(DjVuDocumentTest, PageToUrlForSinglePageReturnsInitUrl)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());

  GURL init_url = doc->get_init_url();
  GURL page0 = doc->page_to_url(0);
  EXPECT_TRUE(init_url == page0);
}

TEST(DjVuDocumentTest, InvalidPageNumberThrowsForSinglePage)
{
  GP<DjVuDocument> doc = DjVuDocument::create(MakeSinglePageDjvu());
  ASSERT_TRUE(doc != 0);
  ASSERT_TRUE(doc->wait_for_complete_init());

  EXPECT_THROW(doc->page_to_url(1), GException);
}
