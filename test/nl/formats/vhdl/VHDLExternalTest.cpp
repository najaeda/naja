// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLDB.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLDesign.h"
#include "VHDLConstructor.h"

#include <cstdlib>
#include <filesystem>

using namespace naja::NL;

class VHDLExternalTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto* db = NLDB::create(NLUniverse::create());
    library_ = NLLibrary::create(db, NLLibrary::Type::Standard);
  }
  void TearDown() override { NLUniverse::get()->destroy(); }

  NLLibrary* library_ {};
};

TEST_F(VHDLExternalTest, AESDecryptBenchmark) {
  const auto* directory = std::getenv("VHDL_AES_BENCHMARK");
  ASSERT_NE(directory, nullptr) << "Set VHDL_AES_BENCHMARK to the downloaded AES rtl directory";
  const auto root = std::filesystem::path(directory);
  VHDLConstructor constructor(library_);
  EXPECT_EQ(constructor.constructFile(root / "aes_pkg.vhdl"), nullptr);
  ASSERT_NE(constructor.constructFile(root / "key_expansion.vhdl"), nullptr);
  auto* top = constructor.constructFile(root / "aes_dec.vhdl", "aes_dec");
  ASSERT_NE(top, nullptr);
  EXPECT_EQ(top->getTerms().size(), 10u);
  EXPECT_NE(top->getInstance(NLName("KEXP0")), nullptr);
}
