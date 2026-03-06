#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "miniexp.h"

namespace {

FILE *OpenTempFileWithText(const std::string &text)
{
  FILE *f = tmpfile();
  if (!f)
    return nullptr;
  if (!text.empty())
    fwrite(text.data(), 1, text.size(), f);
  fflush(f);
  fseek(f, 0, SEEK_SET);
  return f;
}

miniexp_t ReadExpr(const std::string &text)
{
  FILE *f = OpenTempFileWithText(text);
  if (!f)
    return miniexp_dummy;
  miniexp_io_t io;
  miniexp_io_init(&io);
  miniexp_io_set_input(&io, f);
  miniexp_t v = miniexp_read_r(&io);
  fclose(f);
  return v;
}

std::string PrintExpr(miniexp_t value, bool pretty = false, bool newline = false, int width = 80,
                      int flags = 0)
{
  FILE *f = tmpfile();
  if (!f)
    return std::string();
  miniexp_io_t io;
  miniexp_io_init(&io);
  miniexp_io_set_output(&io, f);
  io.p_flags = &flags;

  if (pretty)
  {
    if (newline)
      miniexp_pprint_r(&io, value, width);
    else
      miniexp_pprin_r(&io, value, width);
  }
  else
  {
    if (newline)
      miniexp_print_r(&io, value);
    else
      miniexp_prin_r(&io, value);
  }
  fflush(f);
  fseek(f, 0, SEEK_SET);
  std::string out;
  char buf[256];
  size_t got = 0;
  while ((got = fread(buf, 1, sizeof(buf), f)) > 0)
    out.append(buf, got);
  fclose(f);
  return out;
}

}  // namespace

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

TEST(MiniExpTest, CarCdrFamiliesAndNonPairMutationPaths)
{
  miniexp_t nested =
      miniexp_cons(miniexp_cons(miniexp_number(1), miniexp_number(2)),
                   miniexp_cons(miniexp_cons(miniexp_number(3), miniexp_number(4)),
                                miniexp_cons(miniexp_number(5), miniexp_nil)));

  EXPECT_EQ(1, miniexp_to_int(miniexp_caar(nested)));
  EXPECT_EQ(3, miniexp_to_int(miniexp_car(miniexp_cadr(nested))));
  EXPECT_EQ(2, miniexp_to_int(miniexp_cdar(nested)));
  EXPECT_EQ(5, miniexp_to_int(miniexp_caddr(nested)));
  EXPECT_TRUE(miniexp_consp(miniexp_cddr(nested)));
  EXPECT_EQ(miniexp_nil, miniexp_cdddr(nested));

  EXPECT_EQ(miniexp_nil, miniexp_nth(100, nested));
  EXPECT_EQ(miniexp_car(nested), miniexp_nth(-1, nested));

  EXPECT_EQ(miniexp_nil, miniexp_rplaca(miniexp_symbol("s"), miniexp_number(1)));
  EXPECT_EQ(miniexp_nil, miniexp_rplacd(miniexp_symbol("s"), miniexp_number(1)));
}

TEST(MiniExpTest, ObjectClassAndIsaForStringAndFloat)
{
  miniexp_t s = miniexp_string("hello");
  miniexp_t scls = miniexp_classof(s);
  ASSERT_TRUE(miniexp_symbolp(scls));
  EXPECT_STREQ("string", miniexp_to_name(scls));
  EXPECT_EQ(scls, miniexp_isa(s, scls));

  miniexp_t f = miniexp_floatnum(1.25);
  miniexp_t fcls = miniexp_classof(f);
  ASSERT_TRUE(miniexp_symbolp(fcls));
  EXPECT_STREQ("floatnum", miniexp_to_name(fcls));
  EXPECT_EQ(fcls, miniexp_isa(f, fcls));

  EXPECT_EQ(miniexp_nil, miniexp_classof(miniexp_number(5)));
  EXPECT_EQ(miniexp_nil, miniexp_isa(miniexp_number(5), fcls));
}

TEST(MiniExpTest, ReaderParsesListsSymbolsNumbersAndEscapedStrings)
{
  miniexp_t list = ReadExpr("(alpha (beta . gamma) 42 -7 3.5)");
  ASSERT_TRUE(miniexp_consp(list));
  EXPECT_STREQ("alpha", miniexp_to_name(miniexp_car(list)));
  EXPECT_EQ(42, miniexp_to_int(miniexp_nth(2, list)));
  EXPECT_EQ(-7, miniexp_to_int(miniexp_nth(3, list)));
  EXPECT_DOUBLE_EQ(3.5, miniexp_to_double(miniexp_nth(4, list)));

  miniexp_t dotted = miniexp_nth(1, list);
  ASSERT_TRUE(miniexp_consp(dotted));
  EXPECT_STREQ("beta", miniexp_to_name(miniexp_car(dotted)));
  EXPECT_STREQ("gamma", miniexp_to_name(miniexp_cdr(dotted)));

  miniexp_t quoted = ReadExpr("|needs space|");
  ASSERT_TRUE(miniexp_symbolp(quoted));
  EXPECT_STREQ("needs space", miniexp_to_name(quoted));

  miniexp_t str = ReadExpr("\"A\\n\\x21\\u0416\"");
  ASSERT_TRUE(miniexp_stringp(str));
  const char *buf = nullptr;
  const size_t len = miniexp_to_lstr(str, &buf);
  ASSERT_NE(nullptr, buf);
  EXPECT_EQ(5u, len);
  EXPECT_EQ(0, std::memcmp("A\n!\xD0\x96", buf, len));
}

TEST(MiniExpTest, ReaderErrorPathReturnsDummy)
{
  miniexp_t bad1 = ReadExpr("(a .");
  EXPECT_EQ(miniexp_dummy, bad1);

  miniexp_t bad2 = ReadExpr("\"unterminated");
  EXPECT_EQ(miniexp_dummy, bad2);
}

TEST(MiniExpTest, PrinterAndPrettyPrinterGenerateExpectedText)
{
  miniexp_t expr = ReadExpr("(alpha (beta gamma) \"str\" 123)");
  ASSERT_NE(miniexp_dummy, expr);

  const std::string plain = PrintExpr(expr, false, false);
  EXPECT_NE(std::string::npos, plain.find("alpha"));
  EXPECT_NE(std::string::npos, plain.find("\"str\""));
  EXPECT_NE(std::string::npos, plain.find("123"));

  const std::string with_newline = PrintExpr(expr, false, true);
  ASSERT_FALSE(with_newline.empty());
  EXPECT_EQ('\n', with_newline.back());

  const std::string pretty = PrintExpr(expr, true, false, 10);
  EXPECT_NE(std::string::npos, pretty.find('\n'));

  const std::string pretty_newline = PrintExpr(expr, true, true, 10);
  ASSERT_FALSE(pretty_newline.empty());
  EXPECT_EQ('\n', pretty_newline.back());
}

TEST(MiniExpTest, QuotedSymbolAndUnicodeEscapingFlagsAffectPrinting)
{
  miniexp_t sym = miniexp_symbol("12abc");
  miniexp_t str = miniexp_string("A\xD0\x96");

  int flags = miniexp_io_quotemoresymbols;
  std::string printed_sym = PrintExpr(sym, false, false, 80, flags);
  EXPECT_NE(std::string::npos, printed_sym.find("|12abc|"));

  flags = miniexp_io_u4escape;
  std::string printed_str_u4 = PrintExpr(str, false, false, 80, flags);
  EXPECT_FALSE(printed_str_u4.empty());

  flags = miniexp_io_print7bits | miniexp_io_u6escape;
  std::string printed_str_u6 = PrintExpr(str, false, false, 80, flags);
  EXPECT_FALSE(printed_str_u6.empty());
}

TEST(MiniExpTest, PnameAndCompatApisWorkWithGlobalIo)
{
  miniexp_t expr = ReadExpr("(a b c)");
  ASSERT_NE(miniexp_dummy, expr);

  miniexp_t pname = miniexp_pname(expr, 12);
  ASSERT_TRUE(miniexp_stringp(pname));
  EXPECT_TRUE(miniexp_to_lstr(pname, nullptr) > 0);

  FILE *in = OpenTempFileWithText("(x y)");
  ASSERT_NE(nullptr, in);
  minilisp_set_input(in);
  miniexp_t read = miniexp_read();
  EXPECT_TRUE(miniexp_consp(read));
  fclose(in);

  FILE *out = tmpfile();
  ASSERT_NE(nullptr, out);
  minilisp_set_output(out);
  miniexp_print(read);
  fflush(out);
  fseek(out, 0, SEEK_SET);
  char buf[128] = {0};
  const size_t n = fread(buf, 1, sizeof(buf) - 1, out);
  EXPECT_GT(n, 0u);
  fclose(out);
}

TEST(MiniExpTest, MinivarAndGcLockApisCanBeCalled)
{
  minivar_t *var = minivar_alloc();
  ASSERT_NE(nullptr, var);
  miniexp_t *ptr = minivar_pointer(var);
  ASSERT_NE(nullptr, ptr);
  *ptr = miniexp_symbol("held");
  EXPECT_STREQ("held", miniexp_to_name(*ptr));
  minivar_free(var);

  miniexp_t token = miniexp_symbol("gc-lock");
  EXPECT_EQ(token, minilisp_acquire_gc_lock(token));
  EXPECT_EQ(token, minilisp_release_gc_lock(token));

  minilisp_debug(0);
  minilisp_info();
  minilisp_gc();
}
