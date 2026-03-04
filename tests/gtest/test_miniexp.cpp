#include <gtest/gtest.h>

#include <cstring>

#include "miniexp.h"

TEST(MiniExpTest, NumberRoundtripAndPredicates)
{
  miniexp_t value = miniexp_number(123);
  EXPECT_TRUE(miniexp_numberp(value));
  EXPECT_FALSE(miniexp_symbolp(value));
  EXPECT_FALSE(miniexp_objectp(value));
  EXPECT_EQ(123, miniexp_to_int(value));

  miniexp_t negative = miniexp_number(-77);
  EXPECT_TRUE(miniexp_numberp(negative));
  EXPECT_EQ(-77, miniexp_to_int(negative));
}

TEST(MiniExpTest, SymbolInterningAndNameLookup)
{
  miniexp_t sym1 = miniexp_symbol("djvu:test-symbol");
  miniexp_t sym2 = miniexp_symbol("djvu:test-symbol");
  miniexp_t sym3 = miniexp_symbol("djvu:another-symbol");

  EXPECT_TRUE(miniexp_symbolp(sym1));
  EXPECT_EQ(sym1, sym2);
  EXPECT_NE(sym1, sym3);
  EXPECT_STREQ("djvu:test-symbol", miniexp_to_name(sym1));
}

TEST(MiniExpTest, ListCoreOperations)
{
  miniexp_t list = miniexp_cons(
    miniexp_number(1),
    miniexp_cons(miniexp_number(2), miniexp_cons(miniexp_number(3), miniexp_nil)));

  EXPECT_TRUE(miniexp_listp(list));
  EXPECT_TRUE(miniexp_consp(list));
  EXPECT_EQ(3, miniexp_length(list));
  EXPECT_EQ(1, miniexp_to_int(miniexp_car(list)));
  EXPECT_EQ(2, miniexp_to_int(miniexp_cadr(list)));
  EXPECT_EQ(3, miniexp_to_int(miniexp_nth(2, list)));
  EXPECT_EQ(miniexp_nil, miniexp_nth(99, list));
}

TEST(MiniExpTest, ListMutationAndReverse)
{
  miniexp_t list = miniexp_cons(
    miniexp_number(10),
    miniexp_cons(miniexp_number(20), miniexp_cons(miniexp_number(30), miniexp_nil)));

  miniexp_rplaca(list, miniexp_number(11));
  EXPECT_EQ(11, miniexp_to_int(miniexp_car(list)));

  miniexp_t tail = miniexp_cdr(list);
  miniexp_rplacd(tail, miniexp_cons(miniexp_number(99), miniexp_nil));
  EXPECT_EQ(3, miniexp_length(list));
  EXPECT_EQ(99, miniexp_to_int(miniexp_nth(2, list)));

  miniexp_t reversed = miniexp_reverse(list);
  EXPECT_EQ(99, miniexp_to_int(miniexp_nth(0, reversed)));
  EXPECT_EQ(20, miniexp_to_int(miniexp_nth(1, reversed)));
  EXPECT_EQ(11, miniexp_to_int(miniexp_nth(2, reversed)));
}

TEST(MiniExpTest, CircularListLengthIsMinusOne)
{
  miniexp_t cell = miniexp_cons(miniexp_number(1), miniexp_nil);
  miniexp_rplacd(cell, cell);
  EXPECT_EQ(-1, miniexp_length(cell));
}

TEST(MiniExpTest, StringApisAndConcat)
{
  const char raw_bytes[] = {'a', '\0', 'b', 'c'};
  miniexp_t str = miniexp_lstring(sizeof(raw_bytes), raw_bytes);
  EXPECT_TRUE(miniexp_stringp(str));

  const char *buffer = nullptr;
  size_t len = miniexp_to_lstr(str, &buffer);
  ASSERT_EQ(sizeof(raw_bytes), len);
  ASSERT_NE(nullptr, buffer);
  EXPECT_EQ(0, std::memcmp(raw_bytes, buffer, len));

  miniexp_t concat_input = miniexp_cons(
    miniexp_string("dj"),
    miniexp_cons(miniexp_string("vu"), miniexp_cons(miniexp_string("-lib"), miniexp_nil)));
  miniexp_t merged = miniexp_concat(concat_input);
  EXPECT_TRUE(miniexp_stringp(merged));
  const char *merged_buf = nullptr;
  size_t merged_len = miniexp_to_lstr(merged, &merged_buf);
  ASSERT_EQ(8u, merged_len);
  ASSERT_NE(nullptr, merged_buf);
  EXPECT_EQ(0, std::memcmp("djvu-lib", merged_buf, merged_len));

  miniexp_t sub = miniexp_substring("abcdef", 3);
  EXPECT_TRUE(miniexp_stringp(sub));
  const char *sub_buf = nullptr;
  size_t sub_len = miniexp_to_lstr(sub, &sub_buf);
  ASSERT_EQ(3u, sub_len);
  ASSERT_NE(nullptr, sub_buf);
  EXPECT_EQ(0, std::memcmp("abc", sub_buf, sub_len));
}

TEST(MiniExpTest, DoubleApis)
{
  miniexp_t integral = miniexp_double(42.0);
  EXPECT_TRUE(miniexp_numberp(integral));
  EXPECT_TRUE(miniexp_doublep(integral));
  EXPECT_DOUBLE_EQ(42.0, miniexp_to_double(integral));

  miniexp_t fractional = miniexp_double(3.25);
  EXPECT_TRUE(miniexp_floatnump(fractional));
  EXPECT_TRUE(miniexp_doublep(fractional));
  EXPECT_DOUBLE_EQ(3.25, miniexp_to_double(fractional));

  miniexp_t symbol = miniexp_symbol("not-a-number");
  EXPECT_FALSE(miniexp_doublep(symbol));
  EXPECT_DOUBLE_EQ(0.0, miniexp_to_double(symbol));
}
