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

using namespace naja::NL;

#ifndef SNL_VRL_BENCHMARKS_PATH
#define SNL_VRL_BENCHMARKS_PATH "Undefined"
#endif

class SNLVRLConstructorTestConflictingNameModules: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse* universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      library_ = NLLibrary::create(db, NLName("MYLIB"));
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
      library_ = nullptr;
    }
  protected:
    NLLibrary*      library_;
};

TEST_F(SNLVRLConstructorTestConflictingNameModules, testForbidPolicy) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  constructor.config_.conflictingDesignNamePolicy_ =
    SNLVRLConstructor::Config::ConflictingDesignNamePolicy::Forbid;

  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  EXPECT_THROW(
    constructor.construct(benchmarksPath/"conflicting_name_designs.v"),
    SNLVRLConstructorException);
}

TEST_F(SNLVRLConstructorTestConflictingNameModules, testFirstOnePolicy) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  constructor.config_.conflictingDesignNamePolicy_ =
    SNLVRLConstructor::Config::ConflictingDesignNamePolicy::FirstOne;

  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"conflicting_name_designs.v");

  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("clash"));
  ASSERT_NE(design, nullptr);
  ASSERT_EQ(design->getScalarTerms().size(), 1);
  EXPECT_NE(design->getScalarTerm(NLName("A")), nullptr);
}

TEST_F(SNLVRLConstructorTestConflictingNameModules, testLastOnePolicy) {
  auto db = NLDB::create(NLUniverse::get());
  SNLVRLConstructor constructor(library_);
  constructor.config_.conflictingDesignNamePolicy_ =
    SNLVRLConstructor::Config::ConflictingDesignNamePolicy::LastOne;
  std::filesystem::path benchmarksPath(SNL_VRL_BENCHMARKS_PATH);
  constructor.construct(benchmarksPath/"conflicting_name_designs.v");

  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("clash"));
  ASSERT_NE(design, nullptr);
  ASSERT_EQ(design->getScalarTerms().size(), 1);
  EXPECT_NE(design->getScalarTerm(NLName("D")), nullptr);
}