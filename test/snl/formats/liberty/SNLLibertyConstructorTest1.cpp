// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLDesignTruthTable.h"

#include "SNLLibertyConstructor.h"

using namespace naja::SNL;

#ifndef SNL_LIBERTY_BENCHMARKS
#define SNL_LIBERTY_BENCHMARKS "Undefined"
#endif

class SNLLibertyConstructorTest1: public ::testing::Test {
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

TEST_F(SNLLibertyConstructorTest1, testBusses) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bus_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("bus_test"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("ram"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(4, design->getTerms().size());
  EXPECT_EQ(1, design->getScalarTerms().size());
  EXPECT_EQ(3, design->getBusTerms().size());
  auto clk = design->getScalarTerm(SNLName("clk"));
  ASSERT_NE(nullptr, clk);
  EXPECT_EQ(SNLTerm::Direction::Input, clk->getDirection());
  auto rd_out = design->getBusTerm(SNLName("rd_out"));
  ASSERT_NE(nullptr, rd_out);
  EXPECT_EQ(SNLTerm::Direction::Output, rd_out->getDirection());
  EXPECT_EQ(14, rd_out->getMSB());
  EXPECT_EQ(0, rd_out->getLSB());
}

TEST_F(SNLLibertyConstructorTest1, testBufferFunction) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("buffer_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("buffer_test"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("buffer"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i = design->getScalarTerm(SNLName("I"));
  ASSERT_NE(nullptr, i);
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto z = design->getScalarTerm(SNLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(1, tt.size());
  EXPECT_EQ(0b10, tt.bits());
  EXPECT_TRUE(SNLDesignTruthTable::isBuf(design));
}

TEST_F(SNLLibertyConstructorTest1, testInvFunction) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("inv_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("inv_test"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("inv"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i = design->getScalarTerm(SNLName("I"));
  ASSERT_NE(nullptr, i);
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto z = design->getScalarTerm(SNLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(1, tt.size());
  EXPECT_EQ(0b01, tt.bits());
  EXPECT_TRUE(SNLDesignTruthTable::isInv(design));
}

TEST_F(SNLLibertyConstructorTest1, testAnd2Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("and2_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("and2_test"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("and2"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i0 = design->getScalarTerm(SNLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = design->getScalarTerm(SNLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto z = design->getScalarTerm(SNLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(0b1000, tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testOr2Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("or2_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("or2_test"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("or2"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i0 = design->getScalarTerm(SNLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = design->getScalarTerm(SNLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto z = design->getScalarTerm(SNLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(0b1110, tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testXor2Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("xor2_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("xor2_test"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("xor2"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i0 = design->getScalarTerm(SNLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = design->getScalarTerm(SNLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto z = design->getScalarTerm(SNLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(0b0110, tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testBufZFunction) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bufz.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("bufz_test"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("bufz"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i = design->getScalarTerm(SNLName("I"));
  ASSERT_NE(nullptr, i);
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto en = design->getScalarTerm(SNLName("EN"));
  ASSERT_NE(nullptr, en);
  EXPECT_EQ(SNLTerm::Direction::Input, en->getDirection());
  auto z = design->getScalarTerm(SNLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
}