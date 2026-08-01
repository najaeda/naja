// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>

#include "NajaVersion.h"
#include "SNLDumpManifest.h"
#include "SNLDumpException.h"
#include "NLException.h"
using namespace naja::NL;

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
  EXPECT_EQ(SNLDump::getVersion(), manifest.getSchemaVersion());
  EXPECT_EQ(naja::NAJA_VERSION, manifest.getProducerVersion());
  EXPECT_EQ(naja::NAJA_GIT_HASH, manifest.getProducerGitHash());
}

TEST_F(SNLDumpManifestTest, testErrors) {
  std::filesystem::path errorPath("/error");
  EXPECT_THROW(SNLDumpManifest::dump(errorPath), NLException);
  EXPECT_THROW(SNLDumpManifest::load(errorPath), NLException);
}

TEST_F(SNLDumpManifestTest, malformedRecordsExplainHowToRepairManifest) {
  const auto manifestDir = manifestsPath_ / "malformed";
  std::filesystem::remove_all(manifestDir);
  std::filesystem::create_directory(manifestDir);
  const auto manifestPath = manifestDir / SNLDumpManifest::ManifestFileName;

  {
    std::ofstream manifest(manifestPath);
    manifest << "# SNL manifest\n"
             << "V 1 two 3\n"
             << "P producer hash\n";
  }

  try {
    static_cast<void>(SNLDumpManifest::load(manifestDir));
    FAIL() << "Expected malformed manifest to throw";
  } catch (const SNLDumpException& e) {
    const std::string reason = e.getReason();
    EXPECT_NE(reason.find(manifestPath.string()), std::string::npos);
    EXPECT_NE(reason.find("line 2"), std::string::npos);
    EXPECT_NE(reason.find("got `two`"), std::string::npos);
    EXPECT_NE(reason.find("V <major> <minor> <revision>"), std::string::npos);
  }
}

TEST_F(SNLDumpManifestTest, unknownAndMissingRecordsAreActionable) {
  const auto manifestDir = manifestsPath_ / "unknown_record";
  std::filesystem::remove_all(manifestDir);
  std::filesystem::create_directory(manifestDir);
  const auto manifestPath = manifestDir / SNLDumpManifest::ManifestFileName;

  {
    std::ofstream manifest(manifestPath);
    manifest << "X unsupported\n";
  }
  try {
    static_cast<void>(SNLDumpManifest::load(manifestDir));
    FAIL() << "Expected unknown manifest record to throw";
  } catch (const SNLDumpException& e) {
    const std::string reason = e.getReason();
    EXPECT_NE(reason.find("unknown record type `X`"), std::string::npos);
    EXPECT_NE(reason.find("line 1"), std::string::npos);
  }

  {
    std::ofstream manifest(manifestPath);
    manifest << "# producer-only legacy manifest\n"
             << "P producer hash\n";
  }
  try {
    static_cast<void>(SNLDumpManifest::load(manifestDir));
    FAIL() << "Expected missing schema record to throw";
  } catch (const SNLDumpException& e) {
    const std::string reason = e.getReason();
    EXPECT_NE(reason.find(manifestPath.string()), std::string::npos);
    EXPECT_NE(reason.find("missing required schema version record"), std::string::npos);
    EXPECT_NE(reason.find("V <major> <minor> <revision>"), std::string::npos);
  }
}
