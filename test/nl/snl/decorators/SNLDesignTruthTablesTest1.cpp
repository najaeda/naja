
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
  EXPECT_EQ(4, prims_->getDesigns().size());
  using Prims = std::vector<SNLDesign*>;
  Prims prims(prims_->getDesigns().begin(), prims_->getDesigns().end());
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
} 
