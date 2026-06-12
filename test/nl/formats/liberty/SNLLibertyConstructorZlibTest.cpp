// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <zlib.h>

#include "NLUniverse.h"
#include "SNLLibertyConstructor.h"
#include "SNLLibertyConstructorException.h"

using namespace naja::NL;

#ifndef SNL_LIBERTY_BENCHMARKS
#define SNL_LIBERTY_BENCHMARKS "Undefined"
#endif

namespace {

class TempFileGuard {
  public:
    explicit TempFileGuard(std::filesystem::path path): path_(std::move(path)) {}
    ~TempFileGuard() {
      std::error_code ec;
      std::filesystem::remove(path_, ec);
    }
    const std::filesystem::path& path() const { return path_; }
  private:
    std::filesystem::path path_;
};

std::string readFile(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

std::filesystem::path makeTempPath(const std::string& suffix) {
  auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path()
    / ("naja_liberty_" + std::to_string(stamp) + suffix);
}

std::filesystem::path makeTempGzipPath() {
  return makeTempPath(".lib.gz");
}

void writeGzipFile(const std::filesystem::path& path, const std::string& contents) {
  gzFile out = gzopen(path.string().c_str(), "wb");
  ASSERT_NE(nullptr, out);
  int written = gzwrite(out, contents.data(), static_cast<unsigned int>(contents.size()));
  int closeStatus = gzclose(out);

  ASSERT_EQ(static_cast<int>(contents.size()), written);
  ASSERT_EQ(Z_OK, closeStatus);
}

void writeLE16(std::ostream& output, uint16_t value) {
  unsigned char bytes[] = {
    static_cast<unsigned char>(value & 0xFF),
    static_cast<unsigned char>((value >> 8) & 0xFF)
  };
  output.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
}

void writeLE32(std::ostream& output, uint32_t value) {
  unsigned char bytes[] = {
    static_cast<unsigned char>(value & 0xFF),
    static_cast<unsigned char>((value >> 8) & 0xFF),
    static_cast<unsigned char>((value >> 16) & 0xFF),
    static_cast<unsigned char>((value >> 24) & 0xFF)
  };
  output.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
}

std::vector<unsigned char> deflateRaw(const std::string& contents) {
  std::vector<unsigned char> output(compressBound(contents.size()));
  z_stream stream {};
  int initStatus = deflateInit2(
    &stream,
    Z_BEST_COMPRESSION,
    Z_DEFLATED,
    -MAX_WBITS,
    8,
    Z_DEFAULT_STRATEGY);
  if (initStatus != Z_OK) {
    throw std::runtime_error("failed to initialize deflater");
  }

  stream.next_in = const_cast<Bytef*>(
    reinterpret_cast<const Bytef*>(contents.data()));
  stream.avail_in = static_cast<uInt>(contents.size());
  stream.next_out = output.data();
  stream.avail_out = static_cast<uInt>(output.size());

  int deflateStatus = deflate(&stream, Z_FINISH);
  auto producedBytes = stream.total_out;
  deflateEnd(&stream);
  if (deflateStatus != Z_STREAM_END) {
    throw std::runtime_error("failed to deflate payload");
  }
  output.resize(producedBytes);
  return output;
}

void writeDeflatedZipFile(
  const std::filesystem::path& path,
  const std::string& entryName,
  const std::string& contents) {
  constexpr uint16_t deflateMethod = 8;
  auto compressed = deflateRaw(contents);
  auto crc = crc32(0L, Z_NULL, 0);
  crc = crc32(
    crc,
    reinterpret_cast<const Bytef*>(contents.data()),
    static_cast<uInt>(contents.size()));
  auto entryNameSize = static_cast<uint16_t>(entryName.size());
  auto compressedSize = static_cast<uint32_t>(compressed.size());
  auto uncompressedSize = static_cast<uint32_t>(contents.size());

  std::ofstream output(path, std::ios::binary);
  ASSERT_TRUE(output.is_open());

  auto localHeaderOffset =
    static_cast<uint32_t>(static_cast<std::streamoff>(output.tellp()));
  writeLE32(output, 0x04034B50);
  writeLE16(output, 20);
  writeLE16(output, 0);
  writeLE16(output, deflateMethod);
  writeLE16(output, 0);
  writeLE16(output, 0);
  writeLE32(output, crc);
  writeLE32(output, compressedSize);
  writeLE32(output, uncompressedSize);
  writeLE16(output, entryNameSize);
  writeLE16(output, 0);
  output.write(entryName.data(), entryName.size());
  output.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());

  auto centralDirectoryOffset =
    static_cast<uint32_t>(static_cast<std::streamoff>(output.tellp()));
  writeLE32(output, 0x02014B50);
  writeLE16(output, 20);
  writeLE16(output, 20);
  writeLE16(output, 0);
  writeLE16(output, deflateMethod);
  writeLE16(output, 0);
  writeLE16(output, 0);
  writeLE32(output, crc);
  writeLE32(output, compressedSize);
  writeLE32(output, uncompressedSize);
  writeLE16(output, entryNameSize);
  writeLE16(output, 0);
  writeLE16(output, 0);
  writeLE16(output, 0);
  writeLE16(output, 0);
  writeLE32(output, 0);
  writeLE32(output, localHeaderOffset);
  output.write(entryName.data(), entryName.size());

  auto centralDirectoryEnd =
    static_cast<uint32_t>(static_cast<std::streamoff>(output.tellp()));
  auto centralDirectorySize = centralDirectoryEnd - centralDirectoryOffset;
  writeLE32(output, 0x06054B50);
  writeLE16(output, 0);
  writeLE16(output, 0);
  writeLE16(output, 1);
  writeLE16(output, 1);
  writeLE32(output, centralDirectorySize);
  writeLE32(output, centralDirectoryOffset);
  writeLE16(output, 0);
}

}  // namespace

class SNLLibertyConstructorZlibTest: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("MYLIB"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    NLLibrary* library_ {nullptr};
};

TEST_F(SNLLibertyConstructorZlibTest, testLibertyPathRecognition) {
  EXPECT_TRUE(SNLLibertyConstructor::hasLibertyExtension("small.lib"));
  EXPECT_TRUE(SNLLibertyConstructor::hasLibertyExtension("small.lib_tt"));
  EXPECT_TRUE(SNLLibertyConstructor::hasLibertyExtension("small.lib.gz"));
  EXPECT_FALSE(SNLLibertyConstructor::hasLibertyExtension("small.txt"));

  EXPECT_TRUE(SNLLibertyConstructor::isLibertyPath("small.lib_tt"));
  EXPECT_TRUE(SNLLibertyConstructor::isLibertyPath("small.gz"));
  EXPECT_TRUE(SNLLibertyConstructor::isLibertyPath("small.zip"));
  EXPECT_FALSE(SNLLibertyConstructor::isLibertyPath("small.txt"));
}

TEST_F(SNLLibertyConstructorZlibTest, testGzipParsing) {
  std::filesystem::path sourcePath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("small.lib"));

  auto gzPath = makeTempGzipPath();
  TempFileGuard guard(gzPath);

  auto contents = readFile(sourcePath);
  ASSERT_FALSE(contents.empty());

  writeGzipFile(gzPath, contents);
  EXPECT_TRUE(SNLLibertyConstructor::hasLibertyExtension(gzPath));
  EXPECT_TRUE(SNLLibertyConstructor::hasCompressedLibertySignature(gzPath));
  EXPECT_TRUE(SNLLibertyConstructor::isLibertyPath(gzPath));

  SNLLibertyConstructor constructor(library_);
  constructor.construct(gzPath);

  EXPECT_EQ(NLName("small_lib"), library_->getName());
  EXPECT_EQ(2, library_->getSNLDesigns().size());
  auto and2 = library_->getSNLDesign(NLName("and2"));
  EXPECT_NE(nullptr, and2);
}

TEST_F(SNLLibertyConstructorZlibTest, testGzipParsingWithoutGzipExtension) {
  std::filesystem::path sourcePath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("small.lib"));

  auto gzPath = makeTempPath(".memory");
  TempFileGuard guard(gzPath);

  auto contents = readFile(sourcePath);
  ASSERT_FALSE(contents.empty());

  writeGzipFile(gzPath, contents);

  SNLLibertyConstructor constructor(library_);
  constructor.construct(gzPath);

  EXPECT_EQ(NLName("small_lib"), library_->getName());
  EXPECT_EQ(2, library_->getSNLDesigns().size());
  auto and2 = library_->getSNLDesign(NLName("and2"));
  EXPECT_NE(nullptr, and2);
}

TEST_F(SNLLibertyConstructorZlibTest, testZipParsingWithoutZipExtension) {
  std::filesystem::path sourcePath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("small.lib"));

  auto zipPath = makeTempPath(".memory");
  TempFileGuard guard(zipPath);

  auto contents = readFile(sourcePath);
  ASSERT_FALSE(contents.empty());
  ASSERT_NO_THROW(writeDeflatedZipFile(zipPath, "memories/small.lib_tt", contents));
  EXPECT_FALSE(SNLLibertyConstructor::hasLibertyExtension(zipPath));
  EXPECT_TRUE(SNLLibertyConstructor::hasCompressedLibertySignature(zipPath));
  EXPECT_TRUE(SNLLibertyConstructor::isLibertyPath(zipPath));

  SNLLibertyConstructor constructor(library_);
  constructor.construct(zipPath);

  EXPECT_EQ(NLName("small_lib"), library_->getName());
  EXPECT_EQ(2, library_->getSNLDesigns().size());
  auto and2 = library_->getSNLDesign(NLName("and2"));
  EXPECT_NE(nullptr, and2);
}

TEST_F(SNLLibertyConstructorZlibTest, testGzipReadError) {
  auto gzPath = makeTempGzipPath();
  TempFileGuard guard(gzPath);

  {
    std::ofstream output(gzPath, std::ios::binary);
    const unsigned char badHeader[] = {
      0x1f, 0x8b, 0x08, 0xe0, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x03
    };
    output.write(reinterpret_cast<const char*>(badHeader), sizeof(badHeader));
  }

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(gzPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorZlibTest, testGzipEof) {
  auto gzPath = makeTempGzipPath();
  TempFileGuard guard(gzPath);

  gzFile out = gzopen(gzPath.string().c_str(), "wb");
  ASSERT_NE(nullptr, out);
  int closeStatus = gzclose(out);
  ASSERT_EQ(Z_OK, closeStatus);

  SNLLibertyConstructor constructor(library_);
  EXPECT_ANY_THROW(constructor.construct(gzPath));
}

TEST_F(SNLLibertyConstructorZlibTest, testGzipOpenFailure) {
  auto gzPath = makeTempGzipPath();
  {
    gzFile out = gzopen(gzPath.string().c_str(), "wb");
    ASSERT_NE(nullptr, out);
    int closeStatus = gzclose(out);
    ASSERT_EQ(Z_OK, closeStatus);
  }

  std::error_code ec;
  std::filesystem::permissions(gzPath, std::filesystem::perms::none, ec);
  if (ec) {
    std::filesystem::remove(gzPath, ec);
    GTEST_SKIP() << "Permissions not supported on this platform";
  }

  {
    std::ifstream probe(gzPath, std::ios::binary);
    if (probe.is_open()) {
      std::filesystem::permissions(gzPath, std::filesystem::perms::owner_all, ec);
      std::filesystem::remove(gzPath, ec);
      GTEST_SKIP() << "Unreadable gzip file still readable (likely running as root)";
    }
  }

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(gzPath), SNLLibertyConstructorException);
  std::filesystem::permissions(gzPath, std::filesystem::perms::owner_all, ec);
  std::filesystem::remove(gzPath, ec);
}
