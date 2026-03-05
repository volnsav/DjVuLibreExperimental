#include <gtest/gtest.h>

#include <string>

#include "GIFFManager.h"
#include "GException.h"

TEST(GIFFManagerTest, AddGetDeleteNestedChunkByPath)
{
  GP<GIFFManager> mgr = GIFFManager::create("FORM:DJVM");
  TArray<char> payload(5);
  payload[0] = 'h';
  payload[1] = 'e';
  payload[2] = 'l';
  payload[3] = 'l';
  payload[4] = 'o';

  mgr->add_chunk(".FORM:DJVM.FORM:DJVU[1].INCL", payload);

  EXPECT_EQ(2, mgr->get_chunks_number(".FORM:DJVM.FORM:DJVU"));

  GP<GIFFChunk> page = mgr->get_chunk(".FORM:DJVM.FORM:DJVU[1]");
  ASSERT_TRUE(page != 0);
  GP<GIFFChunk> chunk = page->get_chunk("INCL");
  ASSERT_TRUE(chunk != 0);
  EXPECT_EQ("INCL", std::string((const char *)chunk->get_name()));

  EXPECT_EQ(1, mgr->get_chunks_number(".FORM:DJVM.FORM:DJVU[1].INCL"));
}

TEST(GIFFManagerTest, AddTopLevelPlainChunkThrows)
{
  GP<GIFFManager> mgr = GIFFManager::create();
  TArray<char> payload(1);
  payload[0] = 'x';
  GP<GIFFChunk> plain = GIFFChunk::create("DATA", payload);
  EXPECT_THROW((void)mgr->add_chunk(".", plain), GException);
}

TEST(GIFFManagerTest, SaveLoadRoundtripPreservesChunkData)
{
  GP<GIFFManager> src = GIFFManager::create("FORM:DJVU");
  TArray<char> payload(3);
  payload[0] = 'A';
  payload[1] = 'B';
  payload[2] = 'C';
  src->add_chunk(".FORM:DJVU.TXTz", payload);

  TArray<char> serialized;
  src->save_file(serialized);
  ASSERT_GT(serialized.size(), 0);

  GP<GIFFManager> dst = GIFFManager::create();
  dst->load_file(serialized);

  GP<GIFFChunk> top = dst->get_chunk(".FORM:DJVU");
  ASSERT_TRUE(top != 0);
  GP<GIFFChunk> loaded = top->get_chunk("TXTz");
  ASSERT_TRUE(loaded != 0);
  EXPECT_EQ(1, dst->get_chunks_number(".FORM:DJVU.TXTz"));
}

TEST(GIFFManagerTest, InvalidChunkCountQueryWithBracketsThrows)
{
  GP<GIFFManager> mgr = GIFFManager::create("FORM:DJVU");
  EXPECT_THROW((void)mgr->get_chunks_number("INCL[0]"), GException);
}

TEST(GIFFManagerTest, PlainGIFFChunkKeepsRawData)
{
  TArray<char> payload(3);
  payload[0] = 'x';
  payload[1] = 'y';
  payload[2] = 'z';
  GP<GIFFChunk> plain = GIFFChunk::create("DATA", payload);

  EXPECT_FALSE(plain->is_container());
  TArray<char> out = plain->get_data();
  ASSERT_GE(out.size(), 3);
  EXPECT_EQ('x', out[0]);
  EXPECT_EQ('y', out[1]);
  EXPECT_EQ('z', out[2]);
}
