// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
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

constexpr uint16_t kZipStoredMethod = 0;
constexpr uint16_t kZipDeflateMethod = 8;

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

void putLE16(std::vector<unsigned char>& bytes, size_t offset, uint16_t value) {
  ASSERT_LE(offset + 2, bytes.size());
  bytes[offset] = static_cast<unsigned char>(value & 0xFF);
  bytes[offset + 1] = static_cast<unsigned char>((value >> 8) & 0xFF);
}

void putLE32(std::vector<unsigned char>& bytes, size_t offset, uint32_t value) {
  ASSERT_LE(offset + 4, bytes.size());
  bytes[offset] = static_cast<unsigned char>(value & 0xFF);
  bytes[offset + 1] = static_cast<unsigned char>((value >> 8) & 0xFF);
  bytes[offset + 2] = static_cast<unsigned char>((value >> 16) & 0xFF);
  bytes[offset + 3] = static_cast<unsigned char>((value >> 24) & 0xFF);
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

struct ZipTestEntry {
  std::string name {};
  std::string contents {};
  uint16_t method {kZipDeflateMethod};
  uint16_t flags {0};
  std::vector<unsigned char> payloadOverride {};
  std::optional<uint32_t> compressedSizeOverride {};
  std::optional<uint32_t> uncompressedSizeOverride {};
  std::optional<uint32_t> localHeaderOffsetOverride {};
};

std::vector<unsigned char> zipPayload(const ZipTestEntry& entry) {
  if (not entry.payloadOverride.empty()) {
    return entry.payloadOverride;
  }
  if (entry.method == kZipDeflateMethod) {
    return deflateRaw(entry.contents);
  }
  return std::vector<unsigned char>(entry.contents.begin(), entry.contents.end());
}

void writeZipFile(
  const std::filesystem::path& path,
  const std::vector<ZipTestEntry>& entries) {
  struct WrittenEntry {
    const ZipTestEntry* entry {nullptr};
    std::vector<unsigned char> payload {};
    uint32_t crc {0};
    uint32_t localHeaderOffset {0};
    uint32_t compressedSize {0};
    uint32_t uncompressedSize {0};
  };

  ASSERT_LE(entries.size(), static_cast<size_t>(UINT16_MAX));

  std::ofstream output(path, std::ios::binary);
  ASSERT_TRUE(output.is_open());

  std::vector<WrittenEntry> writtenEntries;
  writtenEntries.reserve(entries.size());
  for (const auto& entry: entries) {
    auto payload = zipPayload(entry);
    auto crc = crc32(0L, Z_NULL, 0);
    crc = crc32(
      crc,
      reinterpret_cast<const Bytef*>(entry.contents.data()),
      static_cast<uInt>(entry.contents.size()));
    auto entryNameSize = static_cast<uint16_t>(entry.name.size());
    auto compressedSize = entry.compressedSizeOverride.value_or(
      static_cast<uint32_t>(payload.size()));
    auto uncompressedSize = entry.uncompressedSizeOverride.value_or(
      static_cast<uint32_t>(entry.contents.size()));

    auto localHeaderOffset =
      static_cast<uint32_t>(static_cast<std::streamoff>(output.tellp()));
    writeLE32(output, 0x04034B50);
    writeLE16(output, 20);
    writeLE16(output, entry.flags);
    writeLE16(output, entry.method);
    writeLE16(output, 0);
    writeLE16(output, 0);
    writeLE32(output, crc);
    writeLE32(output, compressedSize);
    writeLE32(output, uncompressedSize);
    writeLE16(output, entryNameSize);
    writeLE16(output, 0);
    output.write(entry.name.data(), entry.name.size());
    output.write(reinterpret_cast<const char*>(payload.data()), payload.size());

    writtenEntries.push_back(WrittenEntry {
      &entry,
      std::move(payload),
      static_cast<uint32_t>(crc),
      localHeaderOffset,
      compressedSize,
      uncompressedSize});
  }

  auto centralDirectoryOffset =
    static_cast<uint32_t>(static_cast<std::streamoff>(output.tellp()));
  for (const auto& writtenEntry: writtenEntries) {
    const auto& entry = *writtenEntry.entry;
    auto entryNameSize = static_cast<uint16_t>(entry.name.size());
    writeLE32(output, 0x02014B50);
    writeLE16(output, 20);
    writeLE16(output, 20);
    writeLE16(output, entry.flags);
    writeLE16(output, entry.method);
    writeLE16(output, 0);
    writeLE16(output, 0);
    writeLE32(output, writtenEntry.crc);
    writeLE32(output, writtenEntry.compressedSize);
    writeLE32(output, writtenEntry.uncompressedSize);
    writeLE16(output, entryNameSize);
    writeLE16(output, 0);
    writeLE16(output, 0);
    writeLE16(output, 0);
    writeLE16(output, 0);
    writeLE32(output, 0);
    writeLE32(
      output,
      entry.localHeaderOffsetOverride.value_or(writtenEntry.localHeaderOffset));
    output.write(entry.name.data(), entry.name.size());
  }

  auto centralDirectoryEnd =
    static_cast<uint32_t>(static_cast<std::streamoff>(output.tellp()));
  auto centralDirectorySize = centralDirectoryEnd - centralDirectoryOffset;
  writeLE32(output, 0x06054B50);
  writeLE16(output, 0);
  writeLE16(output, 0);
  writeLE16(output, static_cast<uint16_t>(entries.size()));
  writeLE16(output, static_cast<uint16_t>(entries.size()));
  writeLE32(output, centralDirectorySize);
  writeLE32(output, centralDirectoryOffset);
  writeLE16(output, 0);
}

void writeDeflatedZipFile(
  const std::filesystem::path& path,
  const std::string& entryName,
  const std::string& contents) {
  ZipTestEntry entry;
  entry.name = entryName;
  entry.contents = contents;
  entry.method = kZipDeflateMethod;
  writeZipFile(path, {entry});
}

void writeBinaryFile(
  const std::filesystem::path& path,
  const std::vector<unsigned char>& bytes) {
  std::ofstream output(path, std::ios::binary);
  ASSERT_TRUE(output.is_open());
  output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

void writeZipEndOfCentralDirectory(
  std::ostream& output,
  uint16_t entriesCount,
  uint32_t centralDirectorySize,
  uint32_t centralDirectoryOffset) {
  writeLE32(output, 0x06054B50);
  writeLE16(output, 0);
  writeLE16(output, 0);
  writeLE16(output, entriesCount);
  writeLE16(output, entriesCount);
  writeLE32(output, centralDirectorySize);
  writeLE32(output, centralDirectoryOffset);
  writeLE16(output, 0);
}

void writeZipWithEndOfCentralDirectory(
  const std::filesystem::path& path,
  uint16_t entriesCount,
  uint32_t centralDirectorySize,
  uint32_t centralDirectoryOffset) {
  std::ofstream output(path, std::ios::binary);
  ASSERT_TRUE(output.is_open());
  writeLE32(output, 0x04034B50);
  writeZipEndOfCentralDirectory(
    output,
    entriesCount,
    centralDirectorySize,
    centralDirectoryOffset);
}

void writeZipWithCentralDirectory(
  const std::filesystem::path& path,
  const std::vector<unsigned char>& centralDirectory) {
  std::ofstream output(path, std::ios::binary);
  ASSERT_TRUE(output.is_open());
  writeLE32(output, 0x04034B50);
  auto centralDirectoryOffset =
    static_cast<uint32_t>(static_cast<std::streamoff>(output.tellp()));
  output.write(
    reinterpret_cast<const char*>(centralDirectory.data()),
    centralDirectory.size());
  writeZipEndOfCentralDirectory(
    output,
    1,
    static_cast<uint32_t>(centralDirectory.size()),
    centralDirectoryOffset);
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

TEST_F(SNLLibertyConstructorZlibTest, testStoredZipParsingWithSingleNonLibEntry) {
  std::filesystem::path sourcePath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("small.lib"));

  auto zipPath = makeTempPath(".memory");
  TempFileGuard guard(zipPath);

  ZipTestEntry entry;
  entry.name = "payload.txt";
  entry.contents = readFile(sourcePath);
  ASSERT_FALSE(entry.contents.empty());
  entry.method = kZipStoredMethod;
  writeZipFile(zipPath, {entry});

  SNLLibertyConstructor constructor(library_);
  constructor.construct(zipPath);

  EXPECT_EQ(NLName("small_lib"), library_->getName());
  EXPECT_EQ(2, library_->getSNLDesigns().size());
  EXPECT_NE(nullptr, library_->getSNLDesign(NLName("and2")));
}

TEST_F(SNLLibertyConstructorZlibTest, testZipDirectoryOnlyIsAmbiguous) {
  auto zipPath = makeTempPath(".zip");
  TempFileGuard guard(zipPath);

  ZipTestEntry directory;
  directory.name = "cells/";
  directory.method = kZipStoredMethod;
  writeZipFile(zipPath, {directory});

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(zipPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorZlibTest, testZipEmptyDeflatedLibertyEntry) {
  auto zipPath = makeTempPath(".zip");
  TempFileGuard guard(zipPath);

  ZipTestEntry entry;
  entry.name = "empty.lib";
  entry.method = kZipDeflateMethod;
  writeZipFile(zipPath, {entry});

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(zipPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorZlibTest, testZipMalformedMissingEndOfCentralDirectory) {
  auto zipPath = makeTempPath(".zip");
  TempFileGuard guard(zipPath);

  std::vector<unsigned char> bytes(70000, 0);
  putLE32(bytes, 0, 0x04034B50);
  writeBinaryFile(zipPath, bytes);

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(zipPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorZlibTest, testZip64ArchiveIsRejected) {
  auto zipPath = makeTempPath(".zip");
  TempFileGuard guard(zipPath);

  writeZipWithEndOfCentralDirectory(zipPath, UINT16_MAX, 0, 0);

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(zipPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorZlibTest, testZipCentralDirectoryRangeIsRejected) {
  auto zipPath = makeTempPath(".zip");
  TempFileGuard guard(zipPath);

  writeZipWithEndOfCentralDirectory(zipPath, 1, 46, 10000);

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(zipPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorZlibTest, testZipCentralDirectorySignatureIsRejected) {
  auto zipPath = makeTempPath(".zip");
  TempFileGuard guard(zipPath);

  writeZipWithCentralDirectory(zipPath, std::vector<unsigned char>(46, 0));

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(zipPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorZlibTest, testZipCentralDirectoryEntryBoundsAreRejected) {
  auto zipPath = makeTempPath(".zip");
  TempFileGuard guard(zipPath);

  std::vector<unsigned char> centralDirectory(46, 0);
  putLE32(centralDirectory, 0, 0x02014B50);
  putLE16(centralDirectory, 28, 1);
  writeZipWithCentralDirectory(zipPath, centralDirectory);

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(zipPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorZlibTest, testZipCorruptedDeflatedEntryIsRejected) {
  auto zipPath = makeTempPath(".zip");
  TempFileGuard guard(zipPath);

  ZipTestEntry entry;
  entry.name = "bad.lib";
  entry.method = kZipDeflateMethod;
  entry.payloadOverride = {0x00};
  entry.uncompressedSizeOverride = 1;
  writeZipFile(zipPath, {entry});

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(zipPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorZlibTest, testZipEncryptedEntryIsRejected) {
  auto zipPath = makeTempPath(".zip");
  TempFileGuard guard(zipPath);

  ZipTestEntry entry;
  entry.name = "encrypted.lib";
  entry.contents = "library(encrypted) {}";
  entry.flags = 1;
  writeZipFile(zipPath, {entry});

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(zipPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorZlibTest, testZipMalformedLocalHeaderIsRejected) {
  auto zipPath = makeTempPath(".zip");
  TempFileGuard guard(zipPath);

  ZipTestEntry entry;
  entry.name = "bad-local.lib";
  entry.contents = "library(bad_local) {}";
  entry.localHeaderOffsetOverride = 1;
  writeZipFile(zipPath, {entry});

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(zipPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorZlibTest, testZipTruncatedEntryDataIsRejected) {
  auto zipPath = makeTempPath(".zip");
  TempFileGuard guard(zipPath);

  ZipTestEntry entry;
  entry.name = "truncated.lib";
  entry.contents = "library(truncated) {}";
  entry.method = kZipStoredMethod;
  entry.compressedSizeOverride = 10000;
  entry.uncompressedSizeOverride = 10000;
  writeZipFile(zipPath, {entry});

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(zipPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorZlibTest, testZipMalformedStoredEntryIsRejected) {
  auto zipPath = makeTempPath(".zip");
  TempFileGuard guard(zipPath);

  ZipTestEntry entry;
  entry.name = "bad-stored.lib";
  entry.contents = "x";
  entry.method = kZipStoredMethod;
  entry.uncompressedSizeOverride = 2;
  writeZipFile(zipPath, {entry});

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(zipPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorZlibTest, testZipUnsupportedCompressionMethodIsRejected) {
  auto zipPath = makeTempPath(".zip");
  TempFileGuard guard(zipPath);

  ZipTestEntry entry;
  entry.name = "unsupported.lib";
  entry.contents = "library(unsupported) {}";
  entry.method = 99;
  writeZipFile(zipPath, {entry});

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(zipPath), SNLLibertyConstructorException);
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
