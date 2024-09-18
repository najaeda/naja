// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"

#include "SNLLibertyConstructor.h"

#include "YosysLibertyException.h"
#include "SNLLibertyConstructorException.h"

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
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("asap7_excerpt")
      / std::filesystem::path("test0.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("asap7sc7p5t_AO_LVT_FF_ccs_201020"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 2);
  auto design = library_->getDesign(SNLName("A2O1A1Ixp33_ASAP7_75t_L"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(5, design->getTerms().size());
  EXPECT_EQ(5, design->getScalarTerms().size());
  auto a1 = design->getScalarTerm(SNLName("A1"));
  ASSERT_NE(nullptr, a1);
  EXPECT_EQ(SNLTerm::Direction::Input, a1->getDirection());
  auto a2 = design->getScalarTerm(SNLName("A2"));
  ASSERT_NE(nullptr, a2);
  EXPECT_EQ(SNLTerm::Direction::Input, a2->getDirection());
  auto b = design->getScalarTerm(SNLName("B"));
  ASSERT_NE(nullptr, b);
  EXPECT_EQ(SNLTerm::Direction::Input, b->getDirection());
  auto c = design->getScalarTerm(SNLName("C"));
  ASSERT_NE(nullptr, c);
  EXPECT_EQ(SNLTerm::Direction::Input, c->getDirection());
  auto y = design->getScalarTerm(SNLName("Y"));
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
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("iocell"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(1, design->getTerms().size());
  EXPECT_EQ(1, design->getScalarTerms().size());
  auto z = design->getScalarTerm(SNLName("Z"));
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
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("internal_pin_test"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  auto i = design->getScalarTerm(SNLName("I"));
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto z = design->getScalarTerm(SNLName("Z"));
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