// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"

#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLDesignTruthTable.h"

#include "SNLLibertyConstructor.h"
#include "NLBitVecDynamic.h"

using namespace naja::NL;

#ifndef SNL_LIBERTY_BENCHMARKS
#define SNL_LIBERTY_BENCHMARKS "Undefined"
#endif

class SNLLibertyConstructorTest1: public ::testing::Test {
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

TEST_F(SNLLibertyConstructorTest1, testBusses) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("bus_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("bus_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("ram"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(4, design->getTerms().size());
  EXPECT_EQ(1, design->getScalarTerms().size());
  EXPECT_EQ(3, design->getBusTerms().size());
  auto clk = design->getScalarTerm(NLName("clk"));
  ASSERT_NE(nullptr, clk);
  EXPECT_EQ(SNLTerm::Direction::Input, clk->getDirection());
  auto rd_out = design->getBusTerm(NLName("rd_out"));
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
  EXPECT_EQ(NLName("buffer_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("buffer"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i = design->getScalarTerm(NLName("I"));
  ASSERT_NE(nullptr, i);
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(1, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0b10, 2) == tt.bits());
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
  EXPECT_EQ(NLName("inv_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("inv"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(2, design->getTerms().size());
  EXPECT_EQ(2, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i = design->getScalarTerm(NLName("I"));
  ASSERT_NE(nullptr, i);
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(1, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0b01, 2) == tt.bits());
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
  EXPECT_EQ(NLName("and2_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("and2"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i0 = design->getScalarTerm(NLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = design->getScalarTerm(NLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(2, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0b1000, 4) == tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testAnd4Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("and4_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("and4_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("and4"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(5, design->getTerms().size());
  EXPECT_EQ(5, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i0 = design->getScalarTerm(NLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = design->getScalarTerm(NLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto i2 = design->getScalarTerm(NLName("I2"));
  ASSERT_NE(nullptr, i2);
  EXPECT_EQ(SNLTerm::Direction::Input, i2->getDirection());
  auto i3 = design->getScalarTerm(NLName("I3"));
  ASSERT_NE(nullptr, i3);
  EXPECT_EQ(SNLTerm::Direction::Input, i3->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(4, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0x8000, 16) == tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testOr2Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("or2_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("or2_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("or2"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i0 = design->getScalarTerm(NLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = design->getScalarTerm(NLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(2, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0b1110, 4) == tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testXor2Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("xor2_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("xor2_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("xor2"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i0 = design->getScalarTerm(NLName("I0"));
  ASSERT_NE(nullptr, i0);
  EXPECT_EQ(SNLTerm::Direction::Input, i0->getDirection());
  auto i1 = design->getScalarTerm(NLName("I1"));
  ASSERT_NE(nullptr, i1);
  EXPECT_EQ(SNLTerm::Direction::Input, i1->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(2, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0b0110, 4) == tt.bits());
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
  EXPECT_EQ(NLName("gates_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 2);
  auto gate0 = library_->getSNLDesign(NLName("gate0"));
  ASSERT_NE(nullptr, gate0);
  EXPECT_EQ(4, gate0->getTerms().size());
  EXPECT_EQ(4, gate0->getScalarTerms().size());
  EXPECT_TRUE(gate0->getBusTerms().empty());
  auto a = gate0->getScalarTerm(NLName("A"));
  ASSERT_NE(nullptr, a);
  EXPECT_EQ(SNLTerm::Direction::Input, a->getDirection());
  auto b1 = gate0->getScalarTerm(NLName("B1"));
  ASSERT_NE(nullptr, b1);
  EXPECT_EQ(SNLTerm::Direction::Input, b1->getDirection());
  auto b2 = gate0->getScalarTerm(NLName("B2"));
  ASSERT_NE(nullptr, b2);
  EXPECT_EQ(SNLTerm::Direction::Input, b2->getDirection());
  auto z = gate0->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(gate0);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(3, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0x15, 8) == tt.bits());

  auto gate1 = library_->getSNLDesign(NLName("gate0"));
  ASSERT_NE(nullptr, gate1);
  EXPECT_EQ(4, gate1->getTerms().size());
  EXPECT_EQ(4, gate1->getScalarTerms().size());
  EXPECT_TRUE(gate1->getBusTerms().empty());
  a = gate1->getScalarTerm(NLName("A"));
  ASSERT_NE(nullptr, a);
  EXPECT_EQ(SNLTerm::Direction::Input, a->getDirection());
  b1 = gate1->getScalarTerm(NLName("B1"));
  ASSERT_NE(nullptr, b1);
  EXPECT_EQ(SNLTerm::Direction::Input, b1->getDirection());
  b2 = gate1->getScalarTerm(NLName("B2"));
  ASSERT_NE(nullptr, b2);
  EXPECT_EQ(SNLTerm::Direction::Input, b2->getDirection());
  z = gate1->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  tt = SNLDesignTruthTable::getTruthTable(gate1);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(3, tt.size());
  EXPECT_TRUE(NLBitVecDynamic(0x15, 8) == tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testLogic01Function) {
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("logic01_test.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("logic01_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 2);
  auto logic0 = library_->getSNLDesign(NLName("logic0"));
  ASSERT_NE(nullptr, logic0);
  EXPECT_EQ(1, logic0->getTerms().size());
  EXPECT_EQ(1, logic0->getScalarTerms().size());
  EXPECT_TRUE(logic0->getBusTerms().empty());
  auto z = logic0->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(logic0);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(0, tt.size());
  EXPECT_TRUE(0b0 == tt.bits().operator uint64_t());
  EXPECT_TRUE(SNLDesignTruthTable::isConst0(logic0));

  auto logic1 = library_->getSNLDesign(NLName("logic1"));
  ASSERT_NE(nullptr, logic1);
  EXPECT_EQ(1, logic1->getTerms().size());
  EXPECT_EQ(1, logic1->getScalarTerms().size());
  EXPECT_TRUE(logic1->getBusTerms().empty());
  z = logic1->getScalarTerm(NLName("Z"));
  ASSERT_NE(nullptr, z);
  EXPECT_EQ(SNLTerm::Direction::Output, z->getDirection());
  tt = SNLDesignTruthTable::getTruthTable(logic1);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(0, tt.size());
  EXPECT_EQ(0b1, tt.bits().operator uint64_t());
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
  EXPECT_EQ(NLName("bufz_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("bufz"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(3, design->getTerms().size());
  EXPECT_EQ(3, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto i = design->getScalarTerm(NLName("I"));
  ASSERT_NE(nullptr, i);
  EXPECT_EQ(SNLTerm::Direction::Input, i->getDirection());
  auto en = design->getScalarTerm(NLName("EN"));
  ASSERT_NE(nullptr, en);
  EXPECT_EQ(SNLTerm::Direction::Input, en->getDirection());
  auto z = design->getScalarTerm(NLName("Z"));
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
  EXPECT_EQ(NLName("OAI222_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("OAI222"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(7, design->getTerms().size());
  EXPECT_EQ(7, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto a1 = design->getScalarTerm(NLName("A1"));
  ASSERT_NE(nullptr, a1);
  EXPECT_EQ(SNLTerm::Direction::Input, a1->getDirection());
  auto a2 = design->getScalarTerm(NLName("A2"));
  ASSERT_NE(nullptr, a2);
  EXPECT_EQ(SNLTerm::Direction::Input, a2->getDirection());
  auto b1 = design->getScalarTerm(NLName("B1"));
  ASSERT_NE(nullptr, b1);
  EXPECT_EQ(SNLTerm::Direction::Input, b1->getDirection());
  auto b2 = design->getScalarTerm(NLName("B2"));
  ASSERT_NE(nullptr, b2);
  EXPECT_EQ(SNLTerm::Direction::Input, b2->getDirection());
  auto c1 = design->getScalarTerm(NLName("C1"));
  ASSERT_NE(nullptr, c1);
  EXPECT_EQ(SNLTerm::Direction::Input, c1->getDirection());
  auto c2 = design->getScalarTerm(NLName("C2"));
  ASSERT_NE(nullptr, c2);
  EXPECT_EQ(SNLTerm::Direction::Input, c2->getDirection());
  auto zn = design->getScalarTerm(NLName("ZN"));
  ASSERT_NE(nullptr, zn);
  EXPECT_EQ(SNLTerm::Direction::Output, zn->getDirection());
  auto tt = SNLDesignTruthTable::getTruthTable(design);
  EXPECT_TRUE(tt.isInitialized());
  EXPECT_EQ(6, tt.size());
  uint64_t result = 0x111f111f111fffff;
  EXPECT_TRUE(NLBitVecDynamic(result, 64) == tt.bits());
}

TEST_F(SNLLibertyConstructorTest1, testFA_X1Function) {
  //This test tests a multiple output term primitive.
  //The primitive is a full adder with 3 inputs and 2 outputs.
  
  SNLLibertyConstructor constructor(library_);
  std::filesystem::path testPath(
      std::filesystem::path(SNL_LIBERTY_BENCHMARKS)
      / std::filesystem::path("benchmarks")
      / std::filesystem::path("tests")
      / std::filesystem::path("FA_X1.lib"));
  constructor.construct(testPath);
  EXPECT_EQ(NLName("FA_X1_test"), library_->getName());
  EXPECT_EQ(library_->getSNLDesigns().size(), 1);
  auto design = library_->getSNLDesign(NLName("FA_X1"));
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(5, design->getTerms().size());
  EXPECT_EQ(5, design->getScalarTerms().size());
  EXPECT_TRUE(design->getBusTerms().empty());
  auto a = design->getScalarTerm(NLName("A"));
  ASSERT_NE(nullptr, a);
  EXPECT_EQ(SNLTerm::Direction::Input, a->getDirection());
  auto b = design->getScalarTerm(NLName("B"));
  ASSERT_NE(nullptr, b);
  EXPECT_EQ(SNLTerm::Direction::Input, b->getDirection());
  auto ci = design->getScalarTerm(NLName("CI"));
  ASSERT_NE(nullptr, ci);
  EXPECT_EQ(SNLTerm::Direction::Input, ci->getDirection());
  auto co = design->getScalarTerm(NLName("CO"));
  ASSERT_NE(nullptr, co);
  EXPECT_EQ(SNLTerm::Direction::Output, co->getDirection());
  auto s = design->getScalarTerm(NLName("S"));
  ASSERT_NE(nullptr, s);
  EXPECT_EQ(SNLTerm::Direction::Output, s->getDirection());
  // Check the truth table per output
  auto tt_co = SNLDesignTruthTable::getTruthTable(design, co->getID());
  EXPECT_TRUE(tt_co.isInitialized());
  EXPECT_EQ(3, tt_co.size());
  auto tt_s = SNLDesignTruthTable::getTruthTable(design, s->getID());
  EXPECT_TRUE(tt_s.isInitialized());
  EXPECT_EQ(3, tt_s.size());
  // Check the truth table for the design as a whole
  // Note: The design truth table is not initialized for multiple outputs.
  // This is a limitation of the current implementation.
  // Uncomment the following lines if you want to test the design truth table.
  //EXPECT_TRUE(SNLDesignTruthTable::isInitialized(design));
}