// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include <filesystem>

#include "SNLDumpManifest.h"
#include "SNLException.h"
using namespace naja::SNL;

#ifndef SNL_CAPNP_TEST_PATH
#define SNL_CAPNP_TEST_PATH "Undefined"
#endif

namespace {

std::filesystem::path createManifestsDir() {
  std::filesystem::path manifestsPath(SNL_CAPNP_TEST_PATH);
  manifestsPath /= "dumps";
  if (not std::filesystem::exists(manifestsPath)) {
    std::filesystem::create_directory(manifestsPath);
  }
  manifestsPath /= "manifests";
  if (not std::filesystem::exists(manifestsPath)) {
    std::filesystem::create_directory(manifestsPath);
  }
  return manifestsPath;
}

}

class SNLDumpManifestTest: public ::testing::Test {
  protected:
    void SetUp() override {
      manifestsPath_ = createManifestsDir();
    }
    void TearDown() override {
    }
    std::filesystem::path manifestsPath_;
};

TEST_F(SNLDumpManifestTest, test0) {
  std::filesystem::path test0Path(manifestsPath_);
  test0Path /= "test0";
  if (std::filesystem::exists(test0Path)) {
    std::filesystem::remove_all(test0Path);
  }
  std::filesystem::create_directory(test0Path);
  SNLDumpManifest::dump(test0Path);
  ASSERT_TRUE(std::filesystem::exists(test0Path/SNLDumpManifest::ManifestFileName));

  //Reload
  auto manifest = SNLDumpManifest::load(test0Path);
  EXPECT_EQ(SNLDump::getVersion().getMajor(), manifest.getVersion().getMajor());
  EXPECT_EQ(SNLDump::getVersion().getMinor(), manifest.getVersion().getMinor());
  EXPECT_EQ(SNLDump::getVersion().getRevision(), manifest.getVersion().getRevision());
  EXPECT_EQ(SNLDump::getVersion(), manifest.getVersion());
}

TEST_F(SNLDumpManifestTest, testErrors) {
  std::filesystem::path errorPath("/error");
  EXPECT_THROW(SNLDumpManifest::dump(errorPath), SNLException);
  EXPECT_THROW(SNLDumpManifest::load(errorPath), SNLException);
}