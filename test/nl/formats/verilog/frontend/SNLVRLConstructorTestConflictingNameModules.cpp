// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>

#include "NLUniverse.h"
#include "NLException.h"

#include "SNLVRLConstructor.h"
#include "SNLVRLConstructorException.h"
#include "SNLUtils.h"

using namespace naja::NL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestConflictingNameModules: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      db_ = NLDB::create(universe);
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
    protected:
      NLDB* db_ = nullptr;
};

TEST_F(SNLVRLConstructorTestConflictingNameModules, testForbidPolicy) {
  auto library = NLLibrary::create(db_, NLName("MYLIB"));
  SNLVRLConstructor constructor(library);
  constructor.config_.conflictingDesignNamePolicy_ =
    SNLVRLConstructor::Config::ConflictingDesignNamePolicy::Forbid;

  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"conflicting_name_designs.v"),
    SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestConflictingNameModules, testFirstOnePolicy) {
  auto library = NLLibrary::create(db_, NLName("MYLIB"));
  SNLVRLConstructor constructor(library);
  constructor.config_.conflictingDesignNamePolicy_ =
    SNLVRLConstructor::Config::ConflictingDesignNamePolicy::FirstOne;
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"conflicting_name_designs.v");

  EXPECT_EQ(library->getSNLDesigns().size(), 1);
  auto design = library->getSNLDesign(NLName("clash"));
  ASSERT_NE(design, nullptr);
  ASSERT_EQ(design->getScalarTerms().size(), 1);
  EXPECT_NE(design->getScalarTerm(NLName("A")), nullptr);

  auto top = SNLUtils::findTop(library);
  EXPECT_EQ(top, design);
}

TEST_F(SNLVRLConstructorTestConflictingNameModules, testLastOnePolicy) {
  auto library = NLLibrary::create(db_, NLName("MYLIB"));
  SNLVRLConstructor constructor(library);
  constructor.config_.conflictingDesignNamePolicy_ =
    SNLVRLConstructor::Config::ConflictingDesignNamePolicy::LastOne;
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"conflicting_name_designs.v");

  EXPECT_EQ(library->getSNLDesigns().size(), 1);
  auto design = library->getSNLDesign(NLName("clash"));
  ASSERT_NE(design, nullptr);
  ASSERT_EQ(design->getScalarTerms().size(), 1);
  EXPECT_NE(design->getScalarTerm(NLName("D")), nullptr);
  
  auto top = SNLUtils::findTop(library);
  EXPECT_EQ(top, design);
}