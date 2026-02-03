// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

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

std::filesystem::path makeTempGzipPath() {
  auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path()
    / ("naja_liberty_" + std::to_string(stamp) + ".lib.gz");
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

  gzFile out = gzopen(gzPath.string().c_str(), "wb");
  ASSERT_NE(nullptr, out);
  int written = gzwrite(out, contents.data(), static_cast<unsigned int>(contents.size()));
  int closeStatus = gzclose(out);

  ASSERT_EQ(static_cast<int>(contents.size()), written);
  ASSERT_EQ(Z_OK, closeStatus);

  SNLLibertyConstructor constructor(library_);
  constructor.construct(gzPath);

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
  std::error_code ec;
  std::filesystem::create_directory(gzPath, ec);
  if (ec) {
    GTEST_SKIP() << "Failed to create temp directory";
  }

  SNLLibertyConstructor constructor(library_);
  EXPECT_THROW(constructor.construct(gzPath), SNLLibertyConstructorException);
  std::filesystem::remove_all(gzPath, ec);
}
