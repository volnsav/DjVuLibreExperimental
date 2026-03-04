#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DataPool.h"
#include "DjVuFile.h"
#include "DjVuInfo.h"
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
