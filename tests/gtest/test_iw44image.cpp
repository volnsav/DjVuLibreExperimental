#include <gtest/gtest.h>

#include <vector>

#include "ByteStream.h"
#include "GBitmap.h"
#include "GPixmap.h"
#include "GRect.h"
#include "IFFByteStream.h"
#include "IW44Image.h"
#include "GException.h"

namespace
{

GP<GBitmap> MakeGrayBitmap(const int rows, const int cols)
{
  GP<GBitmap> bm = GBitmap::create(rows, cols, 0);
  bm->set_grays(256);
  for (int y = 0; y < rows; ++y)
  {
    for (int x = 0; x < cols; ++x)
      (*bm)[y][x] = static_cast<unsigned char>((x * 17 + y * 11) & 0xff);
  }
  return bm;
}

GP<GPixmap> MakeColorPixmap(const int rows, const int cols)
{
  GP<GPixmap> pm = GPixmap::create(rows, cols, &GPixel::BLACK);
  for (int y = 0; y < rows; ++y)
  {
    for (int x = 0; x < cols; ++x)
    {
      GPixel &px = (*pm)[y][x];
      px.r = static_cast<unsigned char>((x * 31 + y * 7) & 0xff);
      px.g = static_cast<unsigned char>((x * 13 + y * 19) & 0xff);
      px.b = static_cast<unsigned char>((x * 3 + y * 29) & 0xff);
    }
  }
  return pm;
}

IWEncoderParms MakeSliceParms(const int slices)
{
  IWEncoderParms p;
  p.slices = slices;
  p.bytes = 0;
  p.decibels = 0.0f;
  return p;
}

GP<ByteStream> ByteStreamFromBytes(const char *data, const int size)
{
  GP<ByteStream> bs = ByteStream::create();
  if (size > 0)
    bs->writall(data, static_cast<size_t>(size));
  bs->seek(0, SEEK_SET);
  return bs;
}

void ExtractPm44Payloads(
  const GP<ByteStream> &encoded_iff,
  std::vector<char> &first_payload,
  std::vector<char> &second_payload)
{
  encoded_iff->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(encoded_iff);
  GUTF8String chkid;

  ASSERT_GT(reader->get_chunk(chkid), 0);
  ASSERT_STREQ("FORM:PM44", chkid);

  const int size1 = reader->get_chunk(chkid);
  ASSERT_GT(size1, 0);
  ASSERT_STREQ("PM44", chkid);
  first_payload.resize(size1);
  ASSERT_EQ(static_cast<size_t>(size1), reader->read(&first_payload[0], size1));
  reader->close_chunk();

  const int size2 = reader->get_chunk(chkid);
  ASSERT_GT(size2, 0);
  ASSERT_STREQ("PM44", chkid);
  second_payload.resize(size2);
  ASSERT_EQ(static_cast<size_t>(size2), reader->read(&second_payload[0], size2));
  reader->close_chunk();
  reader->close_chunk();
}

void ExtractBm44Payloads(
  const GP<ByteStream> &encoded_iff,
  std::vector<char> &first_payload,
  std::vector<char> &second_payload)
{
  encoded_iff->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(encoded_iff);
  GUTF8String chkid;

  ASSERT_GT(reader->get_chunk(chkid), 0);
  ASSERT_STREQ("FORM:BM44", chkid);

  const int size1 = reader->get_chunk(chkid);
  ASSERT_GT(size1, 0);
  ASSERT_STREQ("BM44", chkid);
  first_payload.resize(size1);
  ASSERT_EQ(static_cast<size_t>(size1), reader->read(&first_payload[0], size1));
  reader->close_chunk();

  const int size2 = reader->get_chunk(chkid);
  ASSERT_GT(size2, 0);
  ASSERT_STREQ("BM44", chkid);
  second_payload.resize(size2);
  ASSERT_EQ(static_cast<size_t>(size2), reader->read(&second_payload[0], size2));
  reader->close_chunk();
  reader->close_chunk();
}

void ExtractSinglePayload(
  const GP<ByteStream> &encoded_iff,
  const char *form_id,
  const char *chunk_id,
  std::vector<char> &payload)
{
  encoded_iff->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(encoded_iff);
  GUTF8String chkid;

  ASSERT_GT(reader->get_chunk(chkid), 0);
  ASSERT_STREQ(form_id, chkid);

  const int size = reader->get_chunk(chkid);
  ASSERT_GT(size, 0);
  ASSERT_STREQ(chunk_id, chkid);
  payload.resize(size);
  ASSERT_EQ(static_cast<size_t>(size), reader->read(&payload[0], size));
  reader->close_chunk();
  reader->close_chunk();
}

GP<ByteStream> BuildSingleChunkForm(
  const char *form_id,
  const char *chunk_id,
  const std::vector<char> &payload)
{
  GP<ByteStream> storage = ByteStream::create();
  GP<IFFByteStream> writer = IFFByteStream::create(storage);
  writer->put_chunk(form_id, 1);
  writer->put_chunk(chunk_id);
  if (!payload.empty())
  {
    EXPECT_EQ(payload.size(), writer->write(&payload[0], payload.size()));
  }
  writer->close_chunk();
  writer->close_chunk();
  storage->seek(0, SEEK_SET);
  return storage;
}

} // namespace

TEST(IW44ImageTest, ParmDbfracChecksRangeForGrayAndColor)
{
  GP<IW44Image> gray = IW44Image::create_decode(IW44Image::GRAY);
  GP<IW44Image> color = IW44Image::create_decode(IW44Image::COLOR);

  ASSERT_TRUE(gray != 0);
  ASSERT_TRUE(color != 0);

  EXPECT_NO_THROW(gray->parm_dbfrac(1.0f));
  EXPECT_NO_THROW(color->parm_dbfrac(0.5f));
  EXPECT_THROW(gray->parm_dbfrac(0.0f), GException);
  EXPECT_THROW(color->parm_dbfrac(1.01f), GException);
}

TEST(IW44ImageTest, ParmCrcbDelayRejectsNegativeValuesByKeepingPrevious)
{
  GP<IW44Image> color = IW44Image::create_decode(IW44Image::COLOR);
  ASSERT_TRUE(color != 0);

  EXPECT_EQ(4, color->parm_crcbdelay(4));
  EXPECT_EQ(4, color->parm_crcbdelay(-1));
}

TEST(IW44ImageTest, DecodeIffRejectsUnknownFormType)
{
  GP<ByteStream> storage = ByteStream::create();
  GP<IFFByteStream> writer = IFFByteStream::create(storage);
  writer->put_chunk("FORM:TEST", 1);
  writer->close_chunk();

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);

  EXPECT_THROW(decoder->decode_iff(*reader, 1), GException);
}

TEST(IW44ImageTest, BitmapEncodeDecodeIffPreservesGeometryAndAllocatesState)
{
  GP<GBitmap> src = MakeGrayBitmap(8, 10);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(24);
  GP<ByteStream> storage = ByteStream::create();
  GP<IFFByteStream> writer = IFFByteStream::create(storage);
  encoder->encode_iff(*writer, 1, &parm);

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::GRAY);
  decoder->decode_iff(*reader, 99);

  EXPECT_EQ(10, decoder->get_width());
  EXPECT_EQ(8, decoder->get_height());
  EXPECT_GT(decoder->get_memory_usage(), 0u);
  EXPECT_GE(decoder->get_percent_memory(), 0);
  EXPECT_LE(decoder->get_percent_memory(), 100);

  GP<GBitmap> decoded = decoder->get_bitmap();
  ASSERT_TRUE(decoded != 0);
  EXPECT_EQ(8u, decoded->rows());
  EXPECT_EQ(10u, decoded->columns());
}

TEST(IW44ImageTest, PixmapEncodeDecodeChunkIncrementsSerialAndResetsOnClose)
{
  GP<GPixmap> src = MakeColorPixmap(9, 7);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parms[2];
  parms[0] = MakeSliceParms(1);
  parms[1] = MakeSliceParms(16);

  GP<ByteStream> storage = ByteStream::create();
  GP<IFFByteStream> writer = IFFByteStream::create(storage);
  encoder->encode_iff(*writer, 2, parms);

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);

  GUTF8String chkid;
  ASSERT_GT(reader->get_chunk(chkid), 0);
  ASSERT_STREQ("FORM:PM44", chkid);
  ASSERT_GT(reader->get_chunk(chkid), 0);
  ASSERT_STREQ("PM44", chkid);

  const int total_slices = decoder->decode_chunk(reader->get_bytestream());
  EXPECT_GE(total_slices, 1);
  EXPECT_EQ(1, decoder->get_serial());
  EXPECT_GT(decoder->get_memory_usage(), 0u);

  reader->close_chunk();
  reader->close_chunk();
  decoder->close_codec();
  EXPECT_EQ(0, decoder->get_serial());
}

TEST(IW44ImageTest, EncodeChunkRejectsZeroStoppingCriteria)
{
  GP<GPixmap> src = MakeColorPixmap(6, 6);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms empty;
  GP<ByteStream> chunk = ByteStream::create();
  EXPECT_THROW((void)encoder->encode_chunk(chunk, empty), GException);
}

TEST(IW44ImageTest, DecodeTwoChunksAdvancesSerialAcrossChunks)
{
  GP<GPixmap> src = MakeColorPixmap(10, 8);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parms[2];
  parms[0] = MakeSliceParms(1);
  parms[1] = MakeSliceParms(24);

  GP<ByteStream> storage = ByteStream::create();
  GP<IFFByteStream> writer = IFFByteStream::create(storage);
  encoder->encode_iff(*writer, 2, parms);

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);
  ASSERT_TRUE(decoder != 0);

  GUTF8String chkid;
  ASSERT_GT(reader->get_chunk(chkid), 0);
  ASSERT_STREQ("FORM:PM44", chkid);

  ASSERT_GT(reader->get_chunk(chkid), 0);
  ASSERT_STREQ("PM44", chkid);
  const int slices_after_first = decoder->decode_chunk(reader->get_bytestream());
  EXPECT_EQ(1, decoder->get_serial());
  reader->close_chunk();

  ASSERT_GT(reader->get_chunk(chkid), 0);
  ASSERT_STREQ("PM44", chkid);
  const int slices_after_second = decoder->decode_chunk(reader->get_bytestream());
  EXPECT_EQ(2, decoder->get_serial());
  EXPECT_GE(slices_after_second, slices_after_first);
  reader->close_chunk();
  reader->close_chunk();

  GP<GPixmap> decoded = decoder->get_pixmap();
  ASSERT_TRUE(decoded != 0);
  EXPECT_EQ(10u, decoded->rows());
  EXPECT_EQ(8u, decoded->columns());
}

TEST(IW44ImageTest, CrcbNoneModeDecodesAsGrayPixmap)
{
  GP<GPixmap> src = MakeColorPixmap(7, 9);
  GP<IW44Image> encoder = IW44Image::create_encode(*src, 0, IW44Image::CRCBnone);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(20);
  GP<ByteStream> storage = ByteStream::create();
  GP<IFFByteStream> writer = IFFByteStream::create(storage);
  encoder->encode_iff(*writer, 1, &parm);

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);
  decoder->decode_iff(*reader, 5);

  GP<GPixmap> pm = decoder->get_pixmap();
  ASSERT_TRUE(pm != 0);
  const GPixel p0 = (*pm)[0][0];
  const GPixel p1 = (*pm)[3][4];
  EXPECT_EQ(p0.r, p0.g);
  EXPECT_EQ(p0.g, p0.b);
  EXPECT_EQ(p1.r, p1.g);
  EXPECT_EQ(p1.g, p1.b);
}

TEST(IW44ImageTest, DecodeIffRejectsWrongSerialInSecondChunk)
{
  GP<GPixmap> src = MakeColorPixmap(8, 8);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parms[2];
  parms[0] = MakeSliceParms(1);
  parms[1] = MakeSliceParms(16);

  GP<ByteStream> encoded = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(encoded);
    encoder->encode_iff(*writer, 2, parms);
  }

  std::vector<char> payload1;
  std::vector<char> payload2;
  ExtractPm44Payloads(encoded, payload1, payload2);
  ASSERT_GT(payload2.size(), 0);

  // Corrupt primary header serial of chunk #2 so decoder expects 1 but sees 0.
  payload2[0] = 0;

  GP<ByteStream> corrupted = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(corrupted);
    writer->put_chunk("FORM:PM44", 1);
    writer->put_chunk("PM44");
    ASSERT_EQ(payload1.size(), writer->write(&payload1[0], payload1.size()));
    writer->close_chunk();
    writer->put_chunk("PM44");
    ASSERT_EQ(payload2.size(), writer->write(&payload2[0], payload2.size()));
    writer->close_chunk();
    writer->close_chunk();
  }

  corrupted->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(corrupted);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);
  EXPECT_THROW(decoder->decode_iff(*reader, 99), GException);
}

TEST(IW44ImageTest, DecodeChunkThenDecodeIffThrowsLeftOpen)
{
  GP<GPixmap> src = MakeColorPixmap(8, 8);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parms[2];
  parms[0] = MakeSliceParms(1);
  parms[1] = MakeSliceParms(16);

  GP<ByteStream> encoded = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(encoded);
    encoder->encode_iff(*writer, 2, parms);
  }

  std::vector<char> payload1;
  std::vector<char> payload2;
  ExtractPm44Payloads(encoded, payload1, payload2);

  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);
  GP<ByteStream> first_chunk_bs = ByteStreamFromBytes(&payload1[0], static_cast<int>(payload1.size()));
  EXPECT_NO_THROW((void)decoder->decode_chunk(first_chunk_bs));

  encoded->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(encoded);
  EXPECT_THROW(decoder->decode_iff(*reader, 99), GException);
}

TEST(IW44ImageTest, DecodeIffSingleChunkThenDecodeChunkFromSecondFailsSerial)
{
  GP<GPixmap> src = MakeColorPixmap(8, 8);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parms[2];
  parms[0] = MakeSliceParms(1);
  parms[1] = MakeSliceParms(16);

  GP<ByteStream> encoded = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(encoded);
    encoder->encode_iff(*writer, 2, parms);
  }

  std::vector<char> payload1;
  std::vector<char> payload2;
  ExtractPm44Payloads(encoded, payload1, payload2);

  encoded->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(encoded);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);
  EXPECT_NO_THROW(decoder->decode_iff(*reader, 1));

  GP<ByteStream> second_chunk_bs = ByteStreamFromBytes(&payload2[0], static_cast<int>(payload2.size()));
  EXPECT_THROW((void)decoder->decode_chunk(second_chunk_bs), GException);
}

TEST(IW44ImageTest, GrayDecoderRejectsPm44Form)
{
  GP<GPixmap> src = MakeColorPixmap(7, 6);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(18);
  GP<ByteStream> storage = ByteStream::create();
  GP<IFFByteStream> writer = IFFByteStream::create(storage);
  encoder->encode_iff(*writer, 1, &parm); // FORM:PM44

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> gray_decoder = IW44Image::create_decode(IW44Image::GRAY);
  EXPECT_THROW(gray_decoder->decode_iff(*reader, 99), GException);
}

TEST(IW44ImageTest, ColorDecoderAcceptsBm44AndProducesGrayChannels)
{
  GP<GBitmap> src = MakeGrayBitmap(6, 8);
  GP<IW44Image> encoder = IW44Image::create_encode(*src); // FORM:BM44
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(20);
  GP<ByteStream> storage = ByteStream::create();
  GP<IFFByteStream> writer = IFFByteStream::create(storage);
  encoder->encode_iff(*writer, 1, &parm);

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> color_decoder = IW44Image::create_decode(IW44Image::COLOR);
  EXPECT_NO_THROW(color_decoder->decode_iff(*reader, 99));

  GP<GPixmap> pm = color_decoder->get_pixmap();
  ASSERT_TRUE(pm != 0);
  ASSERT_EQ(6u, pm->rows());
  ASSERT_EQ(8u, pm->columns());
  const GPixel px = (*pm)[2][3];
  EXPECT_EQ(px.r, px.g);
  EXPECT_EQ(px.g, px.b);
}

TEST(IW44ImageTest, DecodeIffWithZeroMaxChunksLeavesImageEmpty)
{
  GP<GBitmap> src = MakeGrayBitmap(9, 9);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(24);
  GP<ByteStream> storage = ByteStream::create();
  GP<IFFByteStream> writer = IFFByteStream::create(storage);
  encoder->encode_iff(*writer, 1, &parm);

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::GRAY);
  EXPECT_NO_THROW(decoder->decode_iff(*reader, 0));

  EXPECT_EQ(0, decoder->get_width());
  EXPECT_EQ(0, decoder->get_height());
  EXPECT_TRUE(decoder->get_bitmap() == 0);
}

TEST(IW44ImageTest, DecodeIffMaxChunksOneDecodesLessThanFullTwoChunkStream)
{
  GP<GBitmap> src = MakeGrayBitmap(18, 18);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parms[2];
  parms[0] = MakeSliceParms(1);
  parms[1] = MakeSliceParms(28);

  GP<ByteStream> storage = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(storage);
    encoder->encode_iff(*writer, 2, parms);
  }

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader_limited = IFFByteStream::create(storage);
  GP<IW44Image> limited = IW44Image::create_decode(IW44Image::GRAY);
  limited->decode_iff(*reader_limited, 1);

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader_full = IFFByteStream::create(storage);
  GP<IW44Image> full = IW44Image::create_decode(IW44Image::GRAY);
  full->decode_iff(*reader_full, 99);

  EXPECT_EQ(limited->get_width(), full->get_width());
  EXPECT_EQ(limited->get_height(), full->get_height());
  EXPECT_LE(limited->get_memory_usage(), full->get_memory_usage());
}

TEST(IW44ImageTest, DecodeIffNegativeMaxChunksBehavesLikeNoChunks)
{
  GP<GBitmap> src = MakeGrayBitmap(8, 8);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(16);
  GP<ByteStream> storage = ByteStream::create();
  GP<IFFByteStream> writer = IFFByteStream::create(storage);
  encoder->encode_iff(*writer, 1, &parm);

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::GRAY);
  EXPECT_NO_THROW(decoder->decode_iff(*reader, -1));
  EXPECT_EQ(0, decoder->get_width());
  EXPECT_EQ(0, decoder->get_height());
}

TEST(IW44ImageTest, Bm44DecodeIffMaxChunksOneDecodesLessThanFullTwoChunkStream)
{
  GP<GBitmap> src = MakeGrayBitmap(20, 20);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parms[2];
  parms[0] = MakeSliceParms(1);
  parms[1] = MakeSliceParms(30);

  GP<ByteStream> storage = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(storage);
    encoder->encode_iff(*writer, 2, parms);
  }

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader_limited = IFFByteStream::create(storage);
  GP<IW44Image> limited = IW44Image::create_decode(IW44Image::GRAY);
  limited->decode_iff(*reader_limited, 1);

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader_full = IFFByteStream::create(storage);
  GP<IW44Image> full = IW44Image::create_decode(IW44Image::GRAY);
  full->decode_iff(*reader_full, 99);

  EXPECT_EQ(limited->get_width(), full->get_width());
  EXPECT_EQ(limited->get_height(), full->get_height());
  EXPECT_LE(limited->get_memory_usage(), full->get_memory_usage());
}

TEST(IW44ImageTest, GrayEncodeIffThrowsIfChunkEncoderAlreadyOpen)
{
  GP<GBitmap> src = MakeGrayBitmap(10, 10);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms chunk_parm = MakeSliceParms(8);
  GP<ByteStream> chunk_out = ByteStream::create();
  EXPECT_NO_THROW((void)encoder->encode_chunk(chunk_out, chunk_parm));

  GP<ByteStream> iff_storage = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(iff_storage);
  IWEncoderParms iff_parm = MakeSliceParms(16);
  EXPECT_THROW(encoder->encode_iff(*iff, 1, &iff_parm), GException);
}

TEST(IW44ImageTest, ColorEncodeIffThrowsIfChunkEncoderAlreadyOpen)
{
  GP<GPixmap> src = MakeColorPixmap(10, 10);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms chunk_parm = MakeSliceParms(8);
  GP<ByteStream> chunk_out = ByteStream::create();
  EXPECT_NO_THROW((void)encoder->encode_chunk(chunk_out, chunk_parm));

  GP<ByteStream> iff_storage = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(iff_storage);
  IWEncoderParms iff_parm = MakeSliceParms(16);
  EXPECT_THROW(encoder->encode_iff(*iff, 1, &iff_parm), GException);
}

TEST(IW44ImageTest, GrayEncodeIffWithZeroChunksProducesEmptyDecodableForm)
{
  GP<GBitmap> src = MakeGrayBitmap(8, 8);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  GP<ByteStream> storage = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(storage);
    encoder->encode_iff(*writer, 0, 0);
  }

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::GRAY);
  EXPECT_NO_THROW(decoder->decode_iff(*reader, 99));
  EXPECT_EQ(0, decoder->get_width());
  EXPECT_EQ(0, decoder->get_height());
}

TEST(IW44ImageTest, ColorEncodeIffWithZeroChunksProducesEmptyDecodableForm)
{
  GP<GPixmap> src = MakeColorPixmap(8, 8);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  GP<ByteStream> storage = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(storage);
    encoder->encode_iff(*writer, 0, 0);
  }

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);
  EXPECT_NO_THROW(decoder->decode_iff(*reader, 99));
  EXPECT_EQ(0, decoder->get_width());
  EXPECT_EQ(0, decoder->get_height());
}

TEST(IW44ImageTest, GrayDecodeRejectsChunkWithColorMajorFlag)
{
  GP<GBitmap> src = MakeGrayBitmap(8, 8);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(18);
  GP<ByteStream> encoded = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(encoded);
    encoder->encode_iff(*writer, 1, &parm);
  }

  std::vector<char> payload;
  ExtractSinglePayload(encoded, "FORM:BM44", "BM44", payload);
  ASSERT_GT(payload.size(), 3u);
  payload[2] = static_cast<char>(payload[2] & 0x7f);

  GP<ByteStream> corrupted = BuildSingleChunkForm("FORM:BM44", "BM44", payload);
  GP<IFFByteStream> reader = IFFByteStream::create(corrupted);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::GRAY);
  EXPECT_THROW(decoder->decode_iff(*reader, 99), GException);
}

TEST(IW44ImageTest, ColorDecodeRejectsChunkWithFutureMinorVersion)
{
  GP<GPixmap> src = MakeColorPixmap(8, 8);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(18);
  GP<ByteStream> encoded = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(encoded);
    encoder->encode_iff(*writer, 1, &parm);
  }

  std::vector<char> payload;
  ExtractSinglePayload(encoded, "FORM:PM44", "PM44", payload);
  ASSERT_GT(payload.size(), 4u);
  payload[3] = static_cast<char>(0xff);

  GP<ByteStream> corrupted = BuildSingleChunkForm("FORM:PM44", "PM44", payload);
  GP<IFFByteStream> reader = IFFByteStream::create(corrupted);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);
  EXPECT_THROW(decoder->decode_iff(*reader, 99), GException);
}

TEST(IW44ImageTest, ColorDecodeRejectsChunkWithIncompatibleMajorVersion)
{
  GP<GPixmap> src = MakeColorPixmap(8, 8);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(18);
  GP<ByteStream> encoded = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(encoded);
    encoder->encode_iff(*writer, 1, &parm);
  }

  std::vector<char> payload;
  ExtractSinglePayload(encoded, "FORM:PM44", "PM44", payload);
  ASSERT_GT(payload.size(), 3u);
  payload[2] = static_cast<char>(0x82);

  GP<ByteStream> corrupted = BuildSingleChunkForm("FORM:PM44", "PM44", payload);
  GP<IFFByteStream> reader = IFFByteStream::create(corrupted);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);
  EXPECT_THROW(decoder->decode_iff(*reader, 99), GException);
}

TEST(IW44ImageTest, Pm44MixedChunkListRespectsMaxChunksBudget)
{
  GP<GPixmap> src = MakeColorPixmap(8, 8);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(20);
  GP<ByteStream> encoded = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(encoded);
    encoder->encode_iff(*writer, 1, &parm);
  }

  std::vector<char> payload;
  ExtractSinglePayload(encoded, "FORM:PM44", "PM44", payload);

  GP<ByteStream> mixed = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(mixed);
    writer->put_chunk("FORM:PM44", 1);
    writer->put_chunk("JUNK");
    const char marker = 'x';
    ASSERT_EQ(1u, writer->write(&marker, 1));
    writer->close_chunk();
    writer->put_chunk("PM44");
    ASSERT_EQ(payload.size(), writer->write(&payload[0], payload.size()));
    writer->close_chunk();
    writer->close_chunk();
  }

  mixed->seek(0, SEEK_SET);
  GP<IFFByteStream> reader_limited = IFFByteStream::create(mixed);
  GP<IW44Image> limited = IW44Image::create_decode(IW44Image::COLOR);
  limited->decode_iff(*reader_limited, 1);
  EXPECT_EQ(0, limited->get_width());
  EXPECT_EQ(0, limited->get_height());

  mixed->seek(0, SEEK_SET);
  GP<IFFByteStream> reader_two = IFFByteStream::create(mixed);
  GP<IW44Image> decoded = IW44Image::create_decode(IW44Image::COLOR);
  decoded->decode_iff(*reader_two, 2);
  EXPECT_GT(decoded->get_width(), 0);
  EXPECT_GT(decoded->get_height(), 0);
}

TEST(IW44ImageTest, ColorPixmapSubsampleRectPathsAndValidation)
{
  GP<GPixmap> src = MakeColorPixmap(48, 64);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(40);
  GP<ByteStream> storage = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(storage);
    encoder->encode_iff(*writer, 1, &parm);
  }

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);
  decoder->decode_iff(*reader, 99);

  const GRect full1(0, 0, 64, 48);
  const GRect full2(0, 0, 32, 24);
  const GRect full4(0, 0, 16, 12);
  const GRect full8(0, 0, 8, 6);
  const GRect full16(0, 0, 4, 3);
  const GRect full32(0, 0, 2, 2);

  EXPECT_TRUE(decoder->get_pixmap(1, full1) != 0);
  EXPECT_TRUE(decoder->get_pixmap(2, full2) != 0);
  EXPECT_TRUE(decoder->get_pixmap(4, full4) != 0);
  EXPECT_TRUE(decoder->get_pixmap(8, full8) != 0);
  EXPECT_TRUE(decoder->get_pixmap(16, full16) != 0);
  EXPECT_TRUE(decoder->get_pixmap(32, full32) != 0);

  EXPECT_THROW((void)decoder->get_pixmap(3, full1), GException);
  EXPECT_THROW((void)decoder->get_pixmap(1, GRect(0, 0, 0, 0)), GException);
  EXPECT_THROW((void)decoder->get_pixmap(2, GRect(-1, 0, 4, 4)), GException);
  EXPECT_THROW((void)decoder->get_pixmap(2, GRect(100, 100, 4, 4)), GException);
}

TEST(IW44ImageTest, GrayBitmapSubsampleRectPathsAndValidation)
{
  GP<GBitmap> src = MakeGrayBitmap(40, 56);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(36);
  GP<ByteStream> storage = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(storage);
    encoder->encode_iff(*writer, 1, &parm);
  }

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::GRAY);
  decoder->decode_iff(*reader, 99);

  EXPECT_TRUE(decoder->get_bitmap(1, GRect(0, 0, 56, 40)) != 0);
  EXPECT_TRUE(decoder->get_bitmap(2, GRect(0, 0, 28, 20)) != 0);
  EXPECT_TRUE(decoder->get_bitmap(4, GRect(0, 0, 14, 10)) != 0);
  EXPECT_TRUE(decoder->get_bitmap(8, GRect(0, 0, 7, 5)) != 0);

  EXPECT_THROW((void)decoder->get_bitmap(6, GRect(0, 0, 56, 40)), GException);
  EXPECT_THROW((void)decoder->get_bitmap(1, GRect(0, 0, 0, 0)), GException);
  EXPECT_THROW((void)decoder->get_bitmap(2, GRect(-1, 0, 4, 4)), GException);
}

TEST(IW44ImageTest, EncodeWithBitmapMaskExercisesMaskedPath)
{
  GP<GBitmap> src = MakeGrayBitmap(32, 48);
  GP<GBitmap> mask = GBitmap::create(32, 48, 0);
  mask->set_grays(2);
  for (int y = 0; y < 32; ++y)
    for (int x = 0; x < 48; ++x)
      (*mask)[y][x] = ((x + y) % 5 == 0) ? 1 : 0;

  GP<IW44Image> encoder = IW44Image::create_encode(*src, mask);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(30);
  GP<ByteStream> storage = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(storage);
    encoder->encode_iff(*writer, 1, &parm);
  }

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::GRAY);
  EXPECT_NO_THROW(decoder->decode_iff(*reader, 99));
  EXPECT_TRUE(decoder->get_bitmap() != 0);
}

TEST(IW44ImageTest, EncodeWithPixmapMaskAndCrcbModesExercisesColorBranches)
{
  GP<GPixmap> src = MakeColorPixmap(36, 44);
  GP<GBitmap> mask = GBitmap::create(36, 44, 0);
  mask->set_grays(2);
  for (int y = 0; y < 36; ++y)
    for (int x = 0; x < 44; ++x)
      (*mask)[y][x] = ((x * 3 + y * 7) % 11 == 0) ? 1 : 0;

  const IW44Image::CRCBMode modes[] = {
      IW44Image::CRCBhalf, IW44Image::CRCBnormal, IW44Image::CRCBfull};
  for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i)
  {
    GP<IW44Image> encoder = IW44Image::create_encode(*src, mask, modes[i]);
    ASSERT_TRUE(encoder != 0);

    IWEncoderParms parm = MakeSliceParms(28);
    GP<ByteStream> storage = ByteStream::create();
    {
      GP<IFFByteStream> writer = IFFByteStream::create(storage);
      encoder->encode_iff(*writer, 1, &parm);
    }

    storage->seek(0, SEEK_SET);
    GP<IFFByteStream> reader = IFFByteStream::create(storage);
    GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);
    EXPECT_NO_THROW(decoder->decode_iff(*reader, 99));
    EXPECT_TRUE(decoder->get_pixmap() != 0);
  }
}

TEST(IW44ImageTest, BitmapEncodeChunkWithDecibelAndByteStopPaths)
{
  GP<GBitmap> src = MakeGrayBitmap(64, 64);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms p_decibel;
  p_decibel.slices = 0;
  p_decibel.bytes = 0;
  p_decibel.decibels = 5.0f;

  GP<ByteStream> out1 = ByteStream::create();
  EXPECT_NO_THROW((void)encoder->encode_chunk(out1, p_decibel));
  EXPECT_GT(out1->tell(), 0);

  encoder->close_codec();

  IWEncoderParms p_bytes;
  p_bytes.slices = 0;
  p_bytes.bytes = 96;
  p_bytes.decibels = 0.0f;
  GP<ByteStream> out2 = ByteStream::create();
  EXPECT_NO_THROW((void)encoder->encode_chunk(out2, p_bytes));
  EXPECT_GT(out2->tell(), 0);
}

TEST(IW44ImageTest, PixmapEncodeChunkWithDecibelAndByteStopPaths)
{
  GP<GPixmap> src = MakeColorPixmap(64, 64);
  GP<IW44Image> encoder = IW44Image::create_encode(*src, 0, IW44Image::CRCBnormal);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms p_decibel;
  p_decibel.slices = 0;
  p_decibel.bytes = 0;
  p_decibel.decibels = 8.0f;

  GP<ByteStream> out1 = ByteStream::create();
  EXPECT_NO_THROW((void)encoder->encode_chunk(out1, p_decibel));
  EXPECT_GT(out1->tell(), 0);

  encoder->close_codec();

  IWEncoderParms p_bytes;
  p_bytes.slices = 0;
  p_bytes.bytes = 120;
  p_bytes.decibels = 0.0f;
  GP<ByteStream> out2 = ByteStream::create();
  EXPECT_NO_THROW((void)encoder->encode_chunk(out2, p_bytes));
  EXPECT_GT(out2->tell(), 0);
}

TEST(IW44ImageTest, CreateEncodeByTypeNeedsInitBeforeEncoding)
{
  GP<IW44Image> enc_gray = IW44Image::create_encode(IW44Image::GRAY);
  GP<IW44Image> enc_color = IW44Image::create_encode(IW44Image::COLOR);
  ASSERT_TRUE(enc_gray != 0);
  ASSERT_TRUE(enc_color != 0);

  IWEncoderParms p = MakeSliceParms(8);
  GP<ByteStream> out = ByteStream::create();
  EXPECT_THROW((void)enc_gray->encode_chunk(out, p), GException);
  EXPECT_THROW((void)enc_color->encode_chunk(out, p), GException);
}

TEST(IW44ImageTest, InvalidImageTypeFactoriesReturnNull)
{
  GP<IW44Image> bad_dec = IW44Image::create_decode(static_cast<IW44Image::ImageType>(99));
  GP<IW44Image> bad_enc = IW44Image::create_encode(static_cast<IW44Image::ImageType>(99));
  EXPECT_TRUE(bad_dec == 0);
  EXPECT_TRUE(bad_enc == 0);
}

TEST(IW44ImageTest, SerialAndPercentMemoryAccessorsAreUsable)
{
  GP<GPixmap> src = MakeColorPixmap(24, 24);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(20);
  GP<ByteStream> storage = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(storage);
    encoder->encode_iff(*writer, 1, &parm);
  }

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);
  decoder->decode_iff(*reader, 99);

  EXPECT_GE(decoder->get_serial(), 0);
  EXPECT_GE(decoder->get_percent_memory(), 0);
  EXPECT_LE(decoder->get_percent_memory(), 100);
}

TEST(IW44ImageTest, CrcbNoneSubsampleRectPixmapUsesGrayFallbackPath)
{
  GP<GPixmap> src = MakeColorPixmap(20, 20);
  GP<IW44Image> encoder = IW44Image::create_encode(*src, 0, IW44Image::CRCBnone);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(16);
  GP<ByteStream> storage = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(storage);
    encoder->encode_iff(*writer, 1, &parm);
  }

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);
  decoder->decode_iff(*reader, 99);

  GP<GPixmap> pm = decoder->get_pixmap(2, GRect(0, 0, 10, 10));
  ASSERT_TRUE(pm != 0);
  const GPixel px = (*pm)[3][4];
  EXPECT_EQ(px.r, px.g);
  EXPECT_EQ(px.g, px.b);
}

TEST(IW44ImageTest, DecodeObjectsRejectEncodeApis)
{
  GP<IW44Image> dec_gray = IW44Image::create_decode(IW44Image::GRAY);
  GP<IW44Image> dec_color = IW44Image::create_decode(IW44Image::COLOR);
  ASSERT_TRUE(dec_gray != 0);
  ASSERT_TRUE(dec_color != 0);

  IWEncoderParms p = MakeSliceParms(8);
  GP<ByteStream> bs = ByteStream::create();
  GP<IFFByteStream> iff = IFFByteStream::create(bs);
  EXPECT_THROW((void)dec_gray->encode_chunk(bs, p), GException);
  EXPECT_THROW(dec_gray->encode_iff(*iff, 1, &p), GException);
  EXPECT_THROW((void)dec_color->encode_chunk(bs, p), GException);
  EXPECT_THROW(dec_color->encode_iff(*iff, 1, &p), GException);
}

TEST(IW44ImageTest, Bm44MixedChunkListRespectsMaxChunksBudget)
{
  GP<GBitmap> src = MakeGrayBitmap(8, 8);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parm = MakeSliceParms(20);
  GP<ByteStream> encoded = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(encoded);
    encoder->encode_iff(*writer, 1, &parm);
  }

  std::vector<char> payload;
  ExtractSinglePayload(encoded, "FORM:BM44", "BM44", payload);

  GP<ByteStream> mixed = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(mixed);
    writer->put_chunk("FORM:BM44", 1);
    writer->put_chunk("JUNK");
    const char marker = 'x';
    ASSERT_EQ(1u, writer->write(&marker, 1));
    writer->close_chunk();
    writer->put_chunk("BM44");
    ASSERT_EQ(payload.size(), writer->write(&payload[0], payload.size()));
    writer->close_chunk();
    writer->close_chunk();
  }

  mixed->seek(0, SEEK_SET);
  GP<IFFByteStream> reader_limited = IFFByteStream::create(mixed);
  GP<IW44Image> limited = IW44Image::create_decode(IW44Image::GRAY);
  limited->decode_iff(*reader_limited, 1);
  EXPECT_EQ(0, limited->get_width());
  EXPECT_EQ(0, limited->get_height());

  mixed->seek(0, SEEK_SET);
  GP<IFFByteStream> reader_two = IFFByteStream::create(mixed);
  GP<IW44Image> decoded = IW44Image::create_decode(IW44Image::GRAY);
  decoded->decode_iff(*reader_two, 2);
  EXPECT_GT(decoded->get_width(), 0);
  EXPECT_GT(decoded->get_height(), 0);
}

TEST(IW44ImageTest, DecodeIffWithOnlyUnknownChunksLeavesImageEmpty)
{
  GP<ByteStream> storage = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(storage);
    writer->put_chunk("FORM:PM44", 1);
    writer->put_chunk("JUNK");
    const char marker = 'q';
    ASSERT_EQ(1u, writer->write(&marker, 1));
    writer->close_chunk();
    writer->close_chunk();
  }

  storage->seek(0, SEEK_SET);
  GP<IFFByteStream> reader = IFFByteStream::create(storage);
  GP<IW44Image> decoder = IW44Image::create_decode(IW44Image::COLOR);
  EXPECT_NO_THROW(decoder->decode_iff(*reader, 99));
  EXPECT_EQ(0, decoder->get_width());
  EXPECT_EQ(0, decoder->get_height());
}

TEST(IW44ImageTest, Pm44InterposedUnknownChunkConsumesMaxChunksBudget)
{
  GP<GPixmap> src = MakeColorPixmap(16, 16);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parms[2];
  parms[0] = MakeSliceParms(1);
  parms[1] = MakeSliceParms(28);

  GP<ByteStream> encoded = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(encoded);
    encoder->encode_iff(*writer, 2, parms);
  }

  std::vector<char> payload1;
  std::vector<char> payload2;
  ExtractPm44Payloads(encoded, payload1, payload2);

  GP<ByteStream> mixed = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(mixed);
    writer->put_chunk("FORM:PM44", 1);
    writer->put_chunk("PM44");
    ASSERT_EQ(payload1.size(), writer->write(&payload1[0], payload1.size()));
    writer->close_chunk();
    writer->put_chunk("JUNK");
    const char marker = 'j';
    ASSERT_EQ(1u, writer->write(&marker, 1));
    writer->close_chunk();
    writer->put_chunk("PM44");
    ASSERT_EQ(payload2.size(), writer->write(&payload2[0], payload2.size()));
    writer->close_chunk();
    writer->close_chunk();
  }

  mixed->seek(0, SEEK_SET);
  GP<IFFByteStream> reader_limited = IFFByteStream::create(mixed);
  GP<IW44Image> limited = IW44Image::create_decode(IW44Image::COLOR);
  limited->decode_iff(*reader_limited, 2);

  mixed->seek(0, SEEK_SET);
  GP<IFFByteStream> reader_full = IFFByteStream::create(mixed);
  GP<IW44Image> full = IW44Image::create_decode(IW44Image::COLOR);
  full->decode_iff(*reader_full, 99);

  EXPECT_EQ(limited->get_width(), full->get_width());
  EXPECT_EQ(limited->get_height(), full->get_height());
  EXPECT_LE(limited->get_memory_usage(), full->get_memory_usage());
}

TEST(IW44ImageTest, Bm44InterposedUnknownChunkConsumesMaxChunksBudget)
{
  GP<GBitmap> src = MakeGrayBitmap(16, 16);
  GP<IW44Image> encoder = IW44Image::create_encode(*src);
  ASSERT_TRUE(encoder != 0);

  IWEncoderParms parms[2];
  parms[0] = MakeSliceParms(1);
  parms[1] = MakeSliceParms(28);

  GP<ByteStream> encoded = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(encoded);
    encoder->encode_iff(*writer, 2, parms);
  }

  std::vector<char> payload1;
  std::vector<char> payload2;
  ExtractBm44Payloads(encoded, payload1, payload2);

  GP<ByteStream> mixed = ByteStream::create();
  {
    GP<IFFByteStream> writer = IFFByteStream::create(mixed);
    writer->put_chunk("FORM:BM44", 1);
    writer->put_chunk("BM44");
    ASSERT_EQ(payload1.size(), writer->write(&payload1[0], payload1.size()));
    writer->close_chunk();
    writer->put_chunk("JUNK");
    const char marker = 'j';
    ASSERT_EQ(1u, writer->write(&marker, 1));
    writer->close_chunk();
    writer->put_chunk("BM44");
    ASSERT_EQ(payload2.size(), writer->write(&payload2[0], payload2.size()));
    writer->close_chunk();
    writer->close_chunk();
  }

  mixed->seek(0, SEEK_SET);
  GP<IFFByteStream> reader_limited = IFFByteStream::create(mixed);
  GP<IW44Image> limited = IW44Image::create_decode(IW44Image::GRAY);
  limited->decode_iff(*reader_limited, 2);

  mixed->seek(0, SEEK_SET);
  GP<IFFByteStream> reader_full = IFFByteStream::create(mixed);
  GP<IW44Image> full = IW44Image::create_decode(IW44Image::GRAY);
  full->decode_iff(*reader_full, 99);

  EXPECT_EQ(limited->get_width(), full->get_width());
  EXPECT_EQ(limited->get_height(), full->get_height());
  EXPECT_LE(limited->get_memory_usage(), full->get_memory_usage());
}
