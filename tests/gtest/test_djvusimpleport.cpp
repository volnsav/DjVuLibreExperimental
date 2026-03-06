#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "ByteStream.h"
#include "DataPool.h"
#include "DjVuPort.h"
#include "GURL.h"

namespace {

std::filesystem::path MakeTempFile(const std::string &payload)
{
  const auto base = std::filesystem::temp_directory_path();
  const auto path = base / "djvu_gtest_simple_port_tmp.bin";
  std::ofstream os(path, std::ios::binary | std::ios::trunc);
  os.write(payload.data(), static_cast<std::streamsize>(payload.size()));
  os.close();
  return path;
}

}  // namespace

TEST(DjVuSimplePortTest, RequestDataForLocalFileReturnsPool)
{
  const std::string payload = "local-simple-port-data";
  const auto path = MakeTempFile(payload);

  GP<DjVuSimplePort> port = new DjVuSimplePort();
  const GURL url = GURL::Filename::UTF8(path.string().c_str());

  GP<DataPool> pool = port->request_data(nullptr, url);
  ASSERT_TRUE(pool != 0);

  GP<ByteStream> bs = pool->get_stream();
  ASSERT_TRUE(bs != 0);
  bs->seek(0, SEEK_SET);

  char out[64] = {0};
  ASSERT_EQ(payload.size(), bs->readall(out, payload.size()));
  EXPECT_EQ(payload, std::string(out, payload.size()));

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(DjVuSimplePortTest, RequestDataForRemoteUrlReturnsNull)
{
  GP<DjVuSimplePort> port = new DjVuSimplePort();
  const GURL url = GURL::UTF8("https://example.com/test.djvu");

  GP<DataPool> pool = port->request_data(nullptr, url);
  EXPECT_TRUE(pool == 0);
}
