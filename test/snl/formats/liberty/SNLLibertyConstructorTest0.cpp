// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLLibertyConstructor.h"
#include "SNLUniverse.h"
#include "SNLScalarTerm.h"

using namespace naja::SNL;

#ifndef SNL_LIBERTY_BENCHMARKS
#define SNL_LIBERTY_BENCHMARKS "Undefined"
#endif

class SNLLibertyConstructorTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse* universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      library_ = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("MYLIB"));
    }
    void TearDown() override {
      SNLUniverse::get()->destroy();
      library_ = nullptr;
    }
  protected:
    SNLLibrary*      library_;
};

TEST_F(SNLLibertyConstructorTest0, test0) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path test0Path(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("test0.lib"));
  constructor.construct(test0Path);
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("AND2_X1"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  auto a1 = design->getScalarTerm(SNLName("A1"));
  ASSERT_NE(nullptr, a1);
  EXPECT_EQ(SNLTerm::Direction::Input, a1->getDirection());
  auto a2 = design->getScalarTerm(SNLName("A2"));
  ASSERT_NE(nullptr, a2);
  EXPECT_EQ(SNLTerm::Direction::Input, a2->getDirection());
  auto zn = design->getScalarTerm(SNLName("ZN"));
  ASSERT_NE(nullptr, zn);
  EXPECT_EQ(SNLTerm::Direction::Output, zn->getDirection());
}