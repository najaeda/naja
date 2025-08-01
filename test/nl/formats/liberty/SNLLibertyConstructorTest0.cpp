// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"

#include "SNLScalarTerm.h"

#include "SNLLibertyConstructor.h"

#include "YosysLibertyException.h"
#include "SNLLibertyConstructorException.h"

using namespace naja::NL;

#ifndef SNL_LIBERTY_BENCHMARKS
#define SNL_LIBERTY_BENCHMARKS "Undefined"
#endif

class SNLLibertyConstructorTest0: public ::testing::Test {
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
    NLLibrary*  library_;
};

TEST_F(SNLLibertyConstructorTest0, test0) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("asap7_excerpt")
      / std::filesystem::path("test0.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("asap7sc7p5t_AO_LVT_FF_ccs_201020"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 2);
  auto design = library_->getSNLDesign(NLName("A2O1A1Ixp33_ASAP7_75t_L"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(5, design->getTerms().size());
  EXPECT_EQ(5, design->getScalarTerms().size());
  auto a1 = design->getScalarTerm(NLName("A1"));
  ASSERT_NE(nullptr, a1);
  EXPECT_EQ(SNLTerm::Direction::Input, a1->getDirection());
  auto a2 = design->getScalarTerm(NLName("A2"));
  ASSERT_NE(nullptr, a2);
  EXPECT_EQ(SNLTerm::Direction::Input, a2->getDirection());
  auto b = design->getScalarTerm(NLName("B"));
  ASSERT_NE(nullptr, b);
  EXPECT_EQ(SNLTerm::Direction::Input, b->getDirection());
  auto c = design->getScalarTerm(NLName("C"));
  ASSERT_NE(nullptr, c);
  EXPECT_EQ(SNLTerm::Direction::Input, c->getDirection());
  auto y = design->getScalarTerm(NLName("Y"));
  ASSERT_NE(nullptr, y);
  EXPECT_EQ(SNLTerm::Direction::Output, y->getDirection());
}

TEST_F(SNLLibertyConstructorTest0, testInOut) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("inout_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("iocell"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(1, design->getTerms().size());
  EXPECT_EQ(1, design->getScalarTerms().size());
  auto z = design->getScalarTerm(NLName("Z"));
  EXPECT_EQ(SNLTerm::Direction::InOut, z->getDirection());
}

TEST_F(SNLLibertyConstructorTest0, testInternalPin) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("internal_pin_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("internal_pin_test"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  auto i = design->getScalarTerm(NLName("I"));
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
}

TEST_F(SNLLibertyConstructorTest0, testNonExistingFile) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("missing.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testWrongFileType) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("not_a_file.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testWrongSyntaxFile) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("syntax_error.lib"));
  EXPECT_THROW(constructor.construct(testPath), naja::liberty::YosysLibertyException);
}

TEST_F(SNLLibertyConstructorTest0, testWrongDirection) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("direction_error.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testMissingDirection) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("missing_direction_error.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}

TEST_F(SNLLibertyConstructorTest0, testMissingBusType) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("errors")
      / std::filesystem::path("missing_bus_type_error.lib"));
  EXPECT_THROW(constructor.construct(testPath), SNLLibertyConstructorException);
}
