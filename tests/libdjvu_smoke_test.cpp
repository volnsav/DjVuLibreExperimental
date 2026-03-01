#include <iostream>

#include "ByteStream.h"
#include "GString.h"
#include "GURL.h"

static int g_failures = 0;

static void check_true(bool condition, const char *message)
{
  if (!condition)
    {
      std::cerr << "FAIL: " << message << std::endl;
      ++g_failures;
    }
}

static void check_eq_long(long actual, long expected, const char *message)
{
  if (actual != expected)
    {
      std::cerr << "FAIL: " << message
                << " (actual=" << actual << ", expected=" << expected << ")"
                << std::endl;
      ++g_failures;
    }
}

int main()
{
  // 1) Basic string sanity checks.
  GUTF8String text("abc");
  check_true(text.length() == 3, "GUTF8String length should be 3");
  check_true(text.contains("b") == 1, "contains(\"b\") should return index 1");
  check_true(text.rcontains("b") == 1, "rcontains(\"b\") should return index 1");
  check_true(text == "abc", "operator== with const char* should work");

  // 2) URL parsing and local/remote classification checks.
  GURL::UTF8 local_url("file:///tmp/sample.djvu");
  check_true(local_url.protocol() == "file", "file URL protocol should be 'file'");
  check_true(local_url.is_local_file_url(), "file URL should be local");

  GURL::UTF8 remote_url("https://example.com/sample.djvu");
  check_true(remote_url.protocol() == "https", "https URL protocol should be 'https'");
  check_true(!remote_url.is_local_file_url(), "https URL should not be local");

  GURL::UTF8 escaped_name_url("http://example.com/file%201.djvu");
  check_true(escaped_name_url.name() == "file%201.djvu", "name() should keep escapes");
  check_true(escaped_name_url.fname() == "file 1.djvu", "fname() should decode escapes");
  check_true(escaped_name_url.extension() == "djvu", "extension() should be djvu");
  check_true(escaped_name_url.base().get_string() == "http://example.com/",
             "base() should trim document name");
  check_true(escaped_name_url.pathname() == "/file%201.djvu",
             "pathname() should include absolute path without host");

  // 3) ByteStream memory and primitive I/O checks.
  GP<ByteStream> bs = ByteStream::create();
  check_true(bs != 0, "ByteStream::create() should return valid stream");
  check_eq_long(bs->tell(), 0, "new memory stream position should be 0");

  const char payload[] = "djvu";
  size_t written = bs->write(payload, sizeof(payload) - 1);
  check_true(written == sizeof(payload) - 1, "write() should write full payload");
  check_eq_long(bs->tell(), 4, "position after writing 4 bytes should be 4");
  check_eq_long(bs->size(), 4, "size() after writing 4 bytes should be 4");

  check_true(bs->seek(0, SEEK_SET) == 0, "seek(0, SEEK_SET) should succeed");
  char buffer[8] = {0};
  size_t read = bs->readall(buffer, 4);
  check_true(read == 4, "readall() should read 4 bytes");
  check_true(buffer[0] == 'd' && buffer[1] == 'j' && buffer[2] == 'v' && buffer[3] == 'u',
             "payload roundtrip should preserve bytes");

  check_true(bs->seek(0, SEEK_END) == 0, "seek(0, SEEK_END) should succeed");
  bs->write32(0x01020304U);
  check_eq_long(bs->size(), 8, "size() should include appended 32-bit value");
  check_true(bs->seek(4, SEEK_SET) == 0, "seek to integer offset should succeed");
  check_true(bs->read32() == 0x01020304U, "read32() should match write32() value");

  GUTF8String as_text = bs->getAsUTF8();
  check_true(as_text.length() >= 4, "getAsUTF8() should expose stream content");

  if (g_failures != 0)
    {
      std::cerr << "libdjvu_smoke_test: " << g_failures << " check(s) failed" << std::endl;
      return 1;
    }

  std::cout << "libdjvu_smoke_test: OK" << std::endl;
  return 0;
}
