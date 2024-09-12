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

TEST_F(SNLLibertyConstructorTest1, testGate0Function) {
  //function: "!(A | (B1 & B2))";
  //order 0: A 1: B1 2: B2
  //Expected Truth Table: tt(3, 0x15);

  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("gates_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("gates_test"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 2);
  auto gate0 = library_->getDesign(SNLName("gate0"));
  ASSERT_NE(nullptr, gate0);
  EXPECT_EQ(4, gate0->getTerms().size());
  EXPECT_EQ(4, gate0->getScalarTerms().size());
  EXPECT_TRUE(gate0->getBusTerms().empty());
  auto a = gate0->getScalarTerm(SNLName("A"));
  ASSERT_NE(nullptr, a);
  EXPECT_EQ(SNLTerm::Direction::Input, a->getDirection());
  auto b1 = gate0->getScalarTerm(SNLName("B1"));
  ASSERT_NE(nullptr, b1);
  EXPECT_EQ(SNLTerm::Direction::Input, b1->getDirection());
  auto b2 = gate0->getScalarTerm(SNLName("B2"));
  ASSERT_NE(nullptr, b2);
  EXPECT_EQ(SNLTerm::Direction::Input, b2->getDirection());
  auto z = gate0->getScalarTerm(SNLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(gate0);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(3, tt.size());
  EXPECT_EQ(0x15, tt.bits());

  auto gate1 = library_->getDesign(SNLName("gate0"));
  ASSERT_NE(nullptr, gate1);
  EXPECT_EQ(4, gate1->getTerms().size());
  EXPECT_EQ(4, gate1->getScalarTerms().size());
  EXPECT_TRUE(gate1->getBusTerms().empty());
  a = gate1->getScalarTerm(SNLName("A"));
  ASSERT_NE(nullptr, a);
  EXPECT_EQ(SNLTerm::Direction::Input, a->getDirection());
  b1 = gate1->getScalarTerm(SNLName("B1"));
  ASSERT_NE(nullptr, b1);
  EXPECT_EQ(SNLTerm::Direction::Input, b1->getDirection());
  b2 = gate1->getScalarTerm(SNLName("B2"));
  ASSERT_NE(nullptr, b2);
  EXPECT_EQ(SNLTerm::Direction::Input, b2->getDirection());
  z = gate1->getScalarTerm(SNLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  tt = SNLDesignTruthTable::getTruthTable(gate1);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(3, tt.size());
  EXPECT_EQ(0x15, tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testLogic01Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("logic01_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("logic01_test"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 2);
  auto logic0 = library_->getDesign(SNLName("logic0"));
  ASSERT_NE(nullptr, logic0);
  EXPECT_EQ(1, logic0->getTerms().size());
  EXPECT_EQ(1, logic0->getScalarTerms().size());
  EXPECT_TRUE(logic0->getBusTerms().empty());
  auto z = logic0->getScalarTerm(SNLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(logic0);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(0, tt.size());
  EXPECT_EQ(0b0, tt.bits());
  EXPECT_TRUE(SNLDesignTruthTable::isConst0(logic0));

  auto logic1 = library_->getDesign(SNLName("logic1"));
  ASSERT_NE(nullptr, logic1);
  EXPECT_EQ(1, logic1->getTerms().size());
  EXPECT_EQ(1, logic1->getScalarTerms().size());
  EXPECT_TRUE(logic1->getBusTerms().empty());
  z = logic1->getScalarTerm(SNLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  tt = SNLDesignTruthTable::getTruthTable(logic1);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(0, tt.size());
  EXPECT_EQ(0b1, tt.bits());
  EXPECT_TRUE(SNLDesignTruthTable::isConst1(logic1));
}

TEST_F(SNLLibertyConstructorTest1, testBufZFunction) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bufz_test.lib"));
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

TEST_F(SNLLibertyConstructorTest1, testOAI222Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("OAI222_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(SNLName("OAI222_test"), library_->getName());
  EXPECT_EQ(library_->getDesigns().size(), 1);
  auto design = library_->getDesign(SNLName("OAI222"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(7, design->getTerms().size());
  EXPECT_EQ(7, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto a1 = design->getScalarTerm(SNLName("A1"));
  ASSERT_NE(nullptr, a1);
  EXPECT_EQ(SNLTerm::Direction::Input, a1->getDirection());
  auto a2 = design->getScalarTerm(SNLName("A2"));
  ASSERT_NE(nullptr, a2);
  EXPECT_EQ(SNLTerm::Direction::Input, a2->getDirection());
  auto b1 = design->getScalarTerm(SNLName("B1"));
  ASSERT_NE(nullptr, b1);
  EXPECT_EQ(SNLTerm::Direction::Input, b1->getDirection());
  auto b2 = design->getScalarTerm(SNLName("B2"));
  ASSERT_NE(nullptr, b2);
  EXPECT_EQ(SNLTerm::Direction::Input, b2->getDirection());
  auto c1 = design->getScalarTerm(SNLName("C1"));
  ASSERT_NE(nullptr, c1);
  EXPECT_EQ(SNLTerm::Direction::Input, c1->getDirection());
  auto c2 = design->getScalarTerm(SNLName("C2"));
  ASSERT_NE(nullptr, c2);
  EXPECT_EQ(SNLTerm::Direction::Input, c2->getDirection());
  auto zn = design->getScalarTerm(SNLName("ZN"));
  ASSERT_NE(nullptr, zn);
  EXPECT_EQ(SNLTerm::Direction::Output, zn->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(6, tt.size());
  EXPECT_EQ(0x111f111f111fffff, tt.bits());
}