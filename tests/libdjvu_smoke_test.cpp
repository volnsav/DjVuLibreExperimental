#include <iostream>

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

int main()
{
  // Basic string sanity checks.
  GUTF8String text("abc");
  check_true(text.length() == 3, "GUTF8String length should be 3");
  check_true(text.contains("b") == 1, "contains(\"b\") should return index 1");
  check_true(text.rcontains("b") == 1, "rcontains(\"b\") should return index 1");
  check_true(text == "abc", "operator== with const char* should work");

  // URL protocol/local-file detection sanity checks.
  GURL::UTF8 local_url("file:///tmp/sample.djvu");
  check_true(local_url.protocol() == "file", "file URL protocol should be 'file'");
  check_true(local_url.is_local_file_url(), "file URL should be local");

  GURL::UTF8 remote_url("https://example.com/sample.djvu");
  check_true(remote_url.protocol() == "https", "https URL protocol should be 'https'");
  check_true(!remote_url.is_local_file_url(), "https URL should not be local");

  if (g_failures != 0)
    {
      std::cerr << "libdjvu_smoke_test: " << g_failures << " check(s) failed" << std::endl;
      return 1;
    }

  std::cout << "libdjvu_smoke_test: OK" << std::endl;
  return 0;
}
