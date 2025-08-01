
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLUniverse.h"

#include "SNLScalarTerm.h"
#include "SNLDesignTruthTable.h"
using namespace naja::NL;

class SNLDesignTruthTableTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      prims_ = NLLibrary::create(db, NLLibrary::Type::Primitives);
      {
        auto logic0 = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("logic0"));
        auto o = SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, NLName("O"));
        SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0, 0b0));
      }
      {
        auto logic1 = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("logic1"));
        auto o = SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, NLName("O"));
        SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0, 0b1));
      }
      {
        auto inv = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("inv"));
        auto i = SNLScalarTerm::create(inv, SNLTerm::Direction::Input, NLName("I"));
        auto o = SNLScalarTerm::create(inv, SNLTerm::Direction::Output, NLName("O"));
        SNLDesignTruthTable::setTruthTable(inv, SNLTruthTable(1, 0b01));
      }
      {
        auto buf = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("buf"));
        auto i = SNLScalarTerm::create(buf, SNLTerm::Direction::Input, NLName("I"));
        auto o = SNLScalarTerm::create(buf, SNLTerm::Direction::Output, NLName("O"));
        SNLDesignTruthTable::setTruthTable(buf, SNLTruthTable(1, 0b10));
      }
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
    NLLibrary* prims_;
};

TEST_F(SNLDesignTruthTableTest1, testStandardGates) {
  EXPECT_EQ(4, prims_->getSNLDesigns().size());
  using Prims = std::vector<SNLDesign*>;
  Prims prims(prims_->getSNLDesigns().begin(), prims_->getSNLDesigns().end());
  //logic0
  EXPECT_EQ(prims[0]->getName().getString(), "logic0");
  EXPECT_TRUE(SNLDesignTruthTable::isConst0(prims[0]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst1(prims[0]));
  EXPECT_TRUE(SNLDesignTruthTable::isConst(prims[0]));
  //logic1
  EXPECT_EQ(prims[1]->getName().getString(), "logic1");
  EXPECT_TRUE(SNLDesignTruthTable::isConst1(prims[1]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst0(prims[1]));
  EXPECT_TRUE(SNLDesignTruthTable::isConst(prims[1]));
  //inv
  EXPECT_EQ(prims[2]->getName().getString(), "inv");
  EXPECT_TRUE(SNLDesignTruthTable::isInv(prims[2]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst0(prims[2]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst1(prims[2]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst(prims[2]));
  EXPECT_FALSE(SNLDesignTruthTable::isBuf(prims[2]));
  //buf
  EXPECT_EQ(prims[3]->getName().getString(), "buf");
  EXPECT_TRUE(SNLDesignTruthTable::isBuf(prims[3]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst0(prims[3]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst1(prims[3]));
  EXPECT_FALSE(SNLDesignTruthTable::isInv(prims[3]));

  //return false for primitive with no truth table
  auto design = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("prim"));
  EXPECT_FALSE(SNLDesignTruthTable::isConst0(design));
  EXPECT_FALSE(SNLDesignTruthTable::isConst1(design));
  EXPECT_FALSE(SNLDesignTruthTable::isInv(design));
  EXPECT_FALSE(SNLDesignTruthTable::isBuf(design));

  //error when setting truth table on non-primitive
  auto designs = NLLibrary::create(prims_->getDB());
  auto non_prim = SNLDesign::create(designs, NLName("non_prim"));
  EXPECT_THROW(SNLDesignTruthTable::setTruthTable(non_prim, SNLTruthTable(1, 0b1)), NLException);

  //error with more than one output
  auto error = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("error"));
  auto i = SNLScalarTerm::create(error, SNLTerm::Direction::Input, NLName("I"));
  auto o0 = SNLScalarTerm::create(error, SNLTerm::Direction::Output, NLName("O0"));
  auto o1 = SNLScalarTerm::create(error, SNLTerm::Direction::Output, NLName("O1"));
  EXPECT_THROW(SNLDesignTruthTable::setTruthTable(error, SNLTruthTable(1, 0b10)), NLException);

  // Check for trutn table for each output
  auto multiple_outputs = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("multiple_outputs"));
  auto i0mo = SNLScalarTerm::create(multiple_outputs, SNLTerm::Direction::Input, NLName("I0"));
  auto i1mo = SNLScalarTerm::create(multiple_outputs, SNLTerm::Direction::Input, NLName("I1"));
  auto o0mo = SNLScalarTerm::create(multiple_outputs, SNLTerm::Direction::Output, NLName("O0"));
  auto o1mo = SNLScalarTerm::create(multiple_outputs, SNLTerm::Direction::Output, NLName("O1"));
  SNLTruthTable tt0(2, 0b10);
  SNLTruthTable tt1(2, 0b01);
  SNLDesignTruthTable::setTruthTables(multiple_outputs, {tt0, tt1});
  EXPECT_THROW(SNLDesignTruthTable::getTruthTable(multiple_outputs), NLException);
  auto tt0get = SNLDesignTruthTable::getTruthTable(multiple_outputs, 0);
  EXPECT_TRUE(tt0get.isInitialized());
  EXPECT_EQ(tt0, tt0get);
  auto tt1get = SNLDesignTruthTable::getTruthTable(multiple_outputs, 1);
  EXPECT_TRUE(tt1get.isInitialized());
  EXPECT_EQ(tt1, tt1get);

  // Like last test but with truth tables that are of more than 64 bits
  auto multiple_outputs2 = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("multiple_outputs2"));
  auto i0mo2 = SNLScalarTerm::create(multiple_outputs2, SNLTerm::Direction::Input, NLName("I0"));
  auto i1mo2 = SNLScalarTerm::create(multiple_outputs2, SNLTerm::Direction::Input, NLName("I1"));
  auto o0mo2 = SNLScalarTerm::create(multiple_outputs2, SNLTerm::Direction::Output, NLName("O0"));
  auto o1mo2 = SNLScalarTerm::create(multiple_outputs2, SNLTerm::Direction::Output, NLName("O1"));
  // Create a vector of 128
  std::vector<bool> bitVect(128, false);
  for (size_t i = 0; i < bitVect.size(); i++) {
    bitVect[i] = i % 2 ? true : false;
  }
  SNLTruthTable tt0Big(7, bitVect);
  SNLTruthTable tt1Big(7, bitVect);
  SNLDesignTruthTable::setTruthTables(multiple_outputs2, {tt0Big, tt1Big});
  EXPECT_THROW(SNLDesignTruthTable::getTruthTable(multiple_outputs2), NLException);
  auto tt0getBig = SNLDesignTruthTable::getTruthTable(multiple_outputs2, 0);
  EXPECT_TRUE(tt0Big.isInitialized());
  EXPECT_TRUE(tt0Big == tt0getBig);
  auto tt1getBig = SNLDesignTruthTable::getTruthTable(multiple_outputs2, 1);
  EXPECT_TRUE(tt1getBig.isInitialized());
  EXPECT_TRUE(tt1Big == tt1getBig);

  // Add an hybrid example of mask, vector, mask, vector tts
  auto hybrid = SNLDesign::create(prims_, SNLDesign::Type::Primitive, NLName("hybrid"));
  auto i0hybrid = SNLScalarTerm::create(hybrid, SNLTerm::Direction::Input, NLName("I0"));
  auto i1hybrid = SNLScalarTerm::create(hybrid, SNLTerm::Direction::Input, NLName("I1"));
  // 4 outpus
  auto o0hybrid = SNLScalarTerm::create(hybrid, SNLTerm::Direction::Output, NLName("O0"));
  auto o1hybrid = SNLScalarTerm::create(hybrid, SNLTerm::Direction::Output, NLName("O1"));
  auto o2hybrid = SNLScalarTerm::create(hybrid, SNLTerm::Direction::Output, NLName("O2"));
  auto o3hybrid = SNLScalarTerm::create(hybrid, SNLTerm::Direction::Output, NLName("O3"));
  SNLTruthTable tt0hybrid(2, 0b10);
  SNLTruthTable tt1hybrid(2, 0b01);
  std::vector<bool> bitVect2(128, false);
  for (size_t i = 0; i < bitVect2.size(); i++) {
    bitVect2[i] = i % 2 ? true : false;
  }
  SNLTruthTable tt2hybrid(7, bitVect2);
  SNLTruthTable tt3hybrid(2, 0b01);
  SNLDesignTruthTable::setTruthTables(hybrid, {tt0hybrid, tt1hybrid, tt2hybrid, tt3hybrid});
  EXPECT_THROW(SNLDesignTruthTable::getTruthTable(hybrid), NLException);
  auto tt0getHybrid = SNLDesignTruthTable::getTruthTable(hybrid, 0);
  EXPECT_TRUE(tt0hybrid.isInitialized());
  EXPECT_TRUE(tt0hybrid == tt0getHybrid);
  auto tt1getHybrid = SNLDesignTruthTable::getTruthTable(hybrid, 1);
  EXPECT_TRUE(tt1hybrid.isInitialized());
  EXPECT_TRUE(tt1hybrid == tt1getHybrid);
  auto tt2getHybrid = SNLDesignTruthTable::getTruthTable(hybrid, 2);
  EXPECT_TRUE(tt2hybrid.isInitialized());
  EXPECT_TRUE(tt2hybrid == tt2getHybrid);
  auto tt3getHybrid = SNLDesignTruthTable::getTruthTable(hybrid, 3);
  EXPECT_TRUE(tt3hybrid.isInitialized());
  EXPECT_TRUE(tt3hybrid == tt3getHybrid);
} 
