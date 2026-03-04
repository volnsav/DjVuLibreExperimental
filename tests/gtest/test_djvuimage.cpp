#include <gtest/gtest.h>

#include "ByteStream.h"
#include "DjVuFile.h"
#include "DjVuImage.h"
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
  info->width = 40;
  info->height = 20;
  info->dpi = 400;
  info->gamma = 1.8;
  info->version = 24;
  info->encode(*iff->get_bytestream());

  iff->close_chunk();
  iff->close_chunk();
  bs->seek(0, SEEK_SET);
  return bs;
}

}  // namespace

TEST(DjVuImageTest, EmptyImageReturnsDefaults)
{
  GP<DjVuImage> image = DjVuImage::create();
  ASSERT_TRUE(image != 0);

  EXPECT_EQ(0, image->get_width());
  EXPECT_EQ(0, image->get_height());
  EXPECT_EQ(300, image->get_dpi());
  EXPECT_DOUBLE_EQ(2.2, image->get_gamma());
  EXPECT_TRUE(image->get_mimetype().length() == 0);
  EXPECT_FALSE(image->wait_for_complete_decode());
}

TEST(DjVuImageTest, ConnectedImageUsesFileInfoAndRotation)
{
  GP<DjVuFile> file = DjVuFile::create(MakeDjvuWithInfo());
  ASSERT_TRUE(file != 0);

  GP<DjVuInfo> info = DjVuInfo::create();
  info->width = 40;
  info->height = 20;
  info->dpi = 400;
  info->gamma = 1.8;
  file->info = info;
  file->mimetype = "image/djvu";
  file->file_size = 4096;
  file->description = "desc";

  GP<DjVuImage> image = DjVuImage::create(file);
  ASSERT_TRUE(image != 0);

  EXPECT_EQ(40, image->get_width());
  EXPECT_EQ(20, image->get_height());
  EXPECT_EQ(40, image->get_real_width());
  EXPECT_EQ(20, image->get_real_height());
  EXPECT_EQ(400, image->get_dpi());
  EXPECT_DOUBLE_EQ(1.8, image->get_gamma());
  EXPECT_STREQ("image/djvu", (const char *)image->get_mimetype());
  EXPECT_EQ(24, image->get_version());

  image->set_rotate(1);
  EXPECT_EQ(20, image->get_width());
  EXPECT_EQ(40, image->get_height());

  EXPECT_TRUE(image->get_short_description().length() > 0);
  EXPECT_STREQ("desc", (const char *)image->get_long_description());
}

TEST(DjVuImageTest, GetDjVuFileReturnsConnectedFile)
{
  GP<DjVuFile> file = DjVuFile::create(MakeDjvuWithInfo());
  ASSERT_TRUE(file != 0);

  GP<DjVuImage> image = DjVuImage::create(file);
  ASSERT_TRUE(image != 0);

  GP<DjVuFile> returned = image->get_djvu_file();
  EXPECT_TRUE(returned == file);
}
