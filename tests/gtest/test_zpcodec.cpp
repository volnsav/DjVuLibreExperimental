#include <gtest/gtest.h>

#include <vector>

#include "ByteStream.h"
#include "GException.h"
#include "ZPCodec.h"

namespace
{

std::vector<int> MakeBitPattern(const int nbits)
{
  std::vector<int> bits;
  bits.reserve(nbits);
  for (int i = 0; i < nbits; ++i)
    bits.push_back(((i * 37 + (i >> 2)) ^ (i * 11)) & 1);
  return bits;
}

void BuildFlatTable(ZPCodec::Table (&table)[256])
{
  for (int i = 0; i < 256; ++i)
  {
    table[i].p = 0x4000;
    table[i].m = 0x8000;
    table[i].up = static_cast<BitContext>(i);
    table[i].dn = static_cast<BitContext>(i);
  }
}

void BuildAlternatingTable(ZPCodec::Table (&table)[256])
{
  for (int i = 0; i < 256; ++i)
  {
    table[i].p = static_cast<unsigned short>((i & 1) ? 0x1800 : 0x6800);
    table[i].m = static_cast<unsigned short>((i & 1) ? 0x1000 : 0x3000);
    table[i].up = static_cast<BitContext>((i + 3) & 0xff);
    table[i].dn = static_cast<BitContext>((i + 251) & 0xff);
  }
}

} // namespace

TEST(ZPCodecTest, AdaptiveEncodeDecodeRoundtripWithContexts)
{
  const std::vector<int> bits = MakeBitPattern(4000);
  GP<ByteStream> storage = ByteStream::create();

  {
    GP<ZPCodec> enc = ZPCodec::create(storage, true, true);
    BitContext ctx[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < bits.size(); ++i)
      enc->encoder(bits[i], ctx[(i + bits[i]) & 3]);
  }

  storage->seek(0, SEEK_SET);

  GP<ZPCodec> dec = ZPCodec::create(storage, false, true);
  BitContext ctx[4] = {0, 0, 0, 0};
  for (size_t i = 0; i < bits.size(); ++i)
    EXPECT_EQ(bits[i], dec->decoder(ctx[(i + bits[i]) & 3]));
}

TEST(ZPCodecTest, AdaptiveEncodeDecodeRoundtripWithoutDjvuCompat)
{
  const std::vector<int> bits = MakeBitPattern(4096);
  GP<ByteStream> storage = ByteStream::create();

  {
    GP<ZPCodec> enc = ZPCodec::create(storage, true, false);
    BitContext ctx[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    for (size_t i = 0; i < bits.size(); ++i)
      enc->encoder(bits[i], ctx[(i ^ bits[i]) & 7]);
  }

  storage->seek(0, SEEK_SET);
  GP<ZPCodec> dec = ZPCodec::create(storage, false, false);
  BitContext ctx[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  for (size_t i = 0; i < bits.size(); ++i)
    EXPECT_EQ(bits[i], dec->decoder(ctx[(i ^ bits[i]) & 7]));
}

TEST(ZPCodecTest, PassThroughEncodeDecodeRoundtrip)
{
  const std::vector<int> bits = MakeBitPattern(2048);
  GP<ByteStream> storage = ByteStream::create();

  {
    GP<ZPCodec> enc = ZPCodec::create(storage, true, true);
    for (size_t i = 0; i < bits.size(); ++i)
      enc->encoder(bits[i]);
  }

  storage->seek(0, SEEK_SET);
  GP<ZPCodec> dec = ZPCodec::create(storage, false, true);
  for (size_t i = 0; i < bits.size(); ++i)
    EXPECT_EQ(bits[i], dec->decoder());
}

TEST(ZPCodecTest, NoLearnEncodeDecodeRoundtrip)
{
  const std::vector<int> bits = MakeBitPattern(3000);
  GP<ByteStream> storage = ByteStream::create();

  {
    GP<ZPCodec> enc = ZPCodec::create(storage, true, true);
    BitContext ctx = enc->state(0.82f);
    for (size_t i = 0; i < bits.size(); ++i)
      enc->encoder_nolearn(bits[i], ctx);
  }

  storage->seek(0, SEEK_SET);
  GP<ZPCodec> dec = ZPCodec::create(storage, false, true);
  BitContext ctx = dec->state(0.82f);
  for (size_t i = 0; i < bits.size(); ++i)
    EXPECT_EQ(bits[i], dec->decoder_nolearn(ctx));
}

TEST(ZPCodecTest, IwEncodeDecodeRoundtrip)
{
  const std::vector<int> bits = MakeBitPattern(1536);
  GP<ByteStream> storage = ByteStream::create();

  {
    GP<ZPCodec> enc = ZPCodec::create(storage, true, true);
    for (size_t i = 0; i < bits.size(); ++i)
      enc->IWencoder(bits[i] != 0);
  }

  storage->seek(0, SEEK_SET);
  GP<ZPCodec> dec = ZPCodec::create(storage, false, true);
  for (size_t i = 0; i < bits.size(); ++i)
    EXPECT_EQ(bits[i], dec->IWdecoder());
}

TEST(ZPCodecTest, StateMpsParityMatchesProbabilitySide)
{
  GP<ByteStream> storage = ByteStream::create();
  GP<ZPCodec> codec = ZPCodec::create(storage, true, true);
  const BitContext low = codec->state(0.10f);
  const BitContext high = codec->state(0.90f);
  const BitContext half = codec->state(0.50f);

  EXPECT_EQ(0, low & 1);
  EXPECT_EQ(1, high & 1);
  EXPECT_EQ(0, half & 1);

  const BitContext zero = codec->state(0.0f);
  const BitContext one = codec->state(1.0f);
  EXPECT_EQ(0, zero & 1);
  EXPECT_EQ(1, one & 1);
}

TEST(ZPCodecTest, CustomTableRoundtripWorksWithStableContexts)
{
  const std::vector<int> bits = MakeBitPattern(1024);
  ZPCodec::Table table[256];
  BuildFlatTable(table);
  GP<ByteStream> storage = ByteStream::create();

  {
    GP<ZPCodec> enc = ZPCodec::create(storage, true, true);
    enc->newtable(table);
    BitContext ctx = 0;
    for (size_t i = 0; i < bits.size(); ++i)
      enc->encoder(bits[i], ctx);
  }

  storage->seek(0, SEEK_SET);
  GP<ZPCodec> dec = ZPCodec::create(storage, false, true);
  dec->newtable(table);
  BitContext ctx = 0;
  for (size_t i = 0; i < bits.size(); ++i)
    EXPECT_EQ(bits[i], dec->decoder(ctx));
}

TEST(ZPCodecTest, NoLearnRoundtripWithoutDjvuCompat)
{
  const std::vector<int> bits = MakeBitPattern(2500);
  GP<ByteStream> storage = ByteStream::create();

  {
    GP<ZPCodec> enc = ZPCodec::create(storage, true, false);
    BitContext ctx = enc->state(0.73f);
    for (size_t i = 0; i < bits.size(); ++i)
      enc->encoder_nolearn(bits[i], ctx);
  }

  storage->seek(0, SEEK_SET);
  GP<ZPCodec> dec = ZPCodec::create(storage, false, false);
  BitContext ctx = dec->state(0.73f);
  for (size_t i = 0; i < bits.size(); ++i)
    EXPECT_EQ(bits[i], dec->decoder_nolearn(ctx));
}

TEST(ZPCodecTest, MixedCodingModesRoundtripInSingleStream)
{
  const std::vector<int> a = MakeBitPattern(700);
  const std::vector<int> b = MakeBitPattern(900);
  const std::vector<int> c = MakeBitPattern(600);
  const std::vector<int> d = MakeBitPattern(800);

  GP<ByteStream> storage = ByteStream::create();
  {
    GP<ZPCodec> enc = ZPCodec::create(storage, true, true);
    BitContext ctx_adapt = 0;
    BitContext ctx_nolearn = enc->state(0.61f);

    for (size_t i = 0; i < a.size(); ++i)
      enc->encoder(a[i], ctx_adapt);
    for (size_t i = 0; i < b.size(); ++i)
      enc->encoder(b[i]);
    for (size_t i = 0; i < c.size(); ++i)
      enc->IWencoder(c[i] != 0);
    for (size_t i = 0; i < d.size(); ++i)
      enc->encoder_nolearn(d[i], ctx_nolearn);
  }

  storage->seek(0, SEEK_SET);
  GP<ZPCodec> dec = ZPCodec::create(storage, false, true);
  BitContext ctx_adapt = 0;
  BitContext ctx_nolearn = dec->state(0.61f);

  for (size_t i = 0; i < a.size(); ++i)
    EXPECT_EQ(a[i], dec->decoder(ctx_adapt));
  for (size_t i = 0; i < b.size(); ++i)
    EXPECT_EQ(b[i], dec->decoder());
  for (size_t i = 0; i < c.size(); ++i)
    EXPECT_EQ(c[i], dec->IWdecoder());
  for (size_t i = 0; i < d.size(); ++i)
    EXPECT_EQ(d[i], dec->decoder_nolearn(ctx_nolearn));
}

TEST(ZPCodecTest, CustomTableRoundtripWithoutDjvuCompat)
{
  const std::vector<int> bits = MakeBitPattern(1400);
  ZPCodec::Table table[256];
  BuildFlatTable(table);

  GP<ByteStream> storage = ByteStream::create();
  {
    GP<ZPCodec> enc = ZPCodec::create(storage, true, false);
    enc->newtable(table);
    BitContext ctx = 0;
    for (size_t i = 0; i < bits.size(); ++i)
      enc->encoder(bits[i], ctx);
  }

  storage->seek(0, SEEK_SET);
  GP<ZPCodec> dec = ZPCodec::create(storage, false, false);
  dec->newtable(table);
  BitContext ctx = 0;
  for (size_t i = 0; i < bits.size(); ++i)
    EXPECT_EQ(bits[i], dec->decoder(ctx));
}

TEST(ZPCodecTest, DecodingFromEmptyStreamYieldsBinaryBits)
{
  GP<ByteStream> storage = ByteStream::create();
  GP<ZPCodec> dec = ZPCodec::create(storage, false, true);
  BitContext ctx = 0;

  for (int i = 0; i < 4096; ++i)
  {
    const int bit = dec->decoder(ctx);
    EXPECT_TRUE(bit == 0 || bit == 1);
  }
}

TEST(ZPCodecTest, StateRemainsValidAcrossProbabilityExtremes)
{
  GP<ByteStream> storage = ByteStream::create();
  GP<ZPCodec> compat = ZPCodec::create(storage, true, true);
  GP<ZPCodec> noncompat = ZPCodec::create(storage, true, false);

  const float probes[] = {0.0f, 0.0001f, 0.10f, 0.5f, 0.9f, 0.9999f, 1.0f};
  for (size_t i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
  {
    const BitContext c0 = compat->state(probes[i]);
    const BitContext c1 = noncompat->state(probes[i]);
    EXPECT_LE(static_cast<int>(c0), 255);
    EXPECT_LE(static_cast<int>(c1), 255);
    if (probes[i] <= 0.5f)
    {
      EXPECT_EQ(0, c0 & 1);
      EXPECT_EQ(0, c1 & 1);
    }
    else
    {
      EXPECT_EQ(1, c0 & 1);
      EXPECT_EQ(1, c1 & 1);
    }
  }
}

TEST(ZPCodecTest, TableSwitchMidStreamRoundtripWorks)
{
  const std::vector<int> first = MakeBitPattern(1200);
  const std::vector<int> second = MakeBitPattern(1400);
  ZPCodec::Table flat[256];
  ZPCodec::Table alt[256];
  BuildFlatTable(flat);
  BuildAlternatingTable(alt);

  GP<ByteStream> storage = ByteStream::create();
  {
    GP<ZPCodec> enc = ZPCodec::create(storage, true, true);
    BitContext ctx0 = 0;
    enc->newtable(flat);
    for (size_t i = 0; i < first.size(); ++i)
      enc->encoder(first[i], ctx0);

    BitContext ctx1 = 17;
    enc->newtable(alt);
    for (size_t i = 0; i < second.size(); ++i)
      enc->encoder(second[i], ctx1);
  }

  storage->seek(0, SEEK_SET);
  GP<ZPCodec> dec = ZPCodec::create(storage, false, true);
  BitContext ctx0 = 0;
  dec->newtable(flat);
  for (size_t i = 0; i < first.size(); ++i)
    EXPECT_EQ(first[i], dec->decoder(ctx0));

  BitContext ctx1 = 17;
  dec->newtable(alt);
  for (size_t i = 0; i < second.size(); ++i)
    EXPECT_EQ(second[i], dec->decoder(ctx1));
}

TEST(ZPCodecTest, DecoderNoLearnFromEmptyStreamYieldsBinaryBits)
{
  GP<ByteStream> storage = ByteStream::create();
  GP<ZPCodec> dec = ZPCodec::create(storage, false, true);
  BitContext ctx = dec->state(0.77f);

  int observed = 0;
  bool threw = false;
  try
  {
    for (int i = 0; i < 4096; ++i)
    {
      const int bit = dec->decoder_nolearn(ctx);
      EXPECT_TRUE(bit == 0 || bit == 1);
      observed += 1;
    }
  }
  catch (...)
  {
    threw = true;
  }
  EXPECT_GT(observed, 0);
  EXPECT_TRUE(threw || observed == 4096);
}
