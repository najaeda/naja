
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLDesignTruthTable.h"
using namespace naja::SNL;

class SNLDesignTruthTableTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = SNLUniverse::create();
      auto db = SNLDB::create(universe);
      prims_ = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
      {
        auto logic0 = SNLDesign::create(prims_, SNLDesign::Type::Primitive, SNLName("logic0"));
        auto o = SNLScalarTerm::create(logic0, SNLTerm::Direction::Output, SNLName("O"));
        SNLDesignTruthTable::setTruthTable(logic0, SNLTruthTable(0, 0b0));
      }
      {
        auto logic1 = SNLDesign::create(prims_, SNLDesign::Type::Primitive, SNLName("logic1"));
        auto o = SNLScalarTerm::create(logic1, SNLTerm::Direction::Output, SNLName("O"));
        SNLDesignTruthTable::setTruthTable(logic1, SNLTruthTable(0, 0b1));
      }
      {
        auto inv = SNLDesign::create(prims_, SNLDesign::Type::Primitive, SNLName("inv"));
        auto i = SNLScalarTerm::create(inv, SNLTerm::Direction::Input, SNLName("I"));
        auto o = SNLScalarTerm::create(inv, SNLTerm::Direction::Output, SNLName("O"));
        SNLDesignTruthTable::setTruthTable(inv, SNLTruthTable(1, 0b01));
      }
      {
        auto buf = SNLDesign::create(prims_, SNLDesign::Type::Primitive, SNLName("buf"));
        auto i = SNLScalarTerm::create(buf, SNLTerm::Direction::Input, SNLName("I"));
        auto o = SNLScalarTerm::create(buf, SNLTerm::Direction::Output, SNLName("O"));
        SNLDesignTruthTable::setTruthTable(buf, SNLTruthTable(1, 0b10));
      }
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
    SNLLibrary* prims_;
};

TEST_F(SNLDesignTruthTableTest1, testStandardGates) {
  EXPECT_EQ(4, prims_->getSNLDesigns().size());
  using Prims = std::vector<SNLDesign*>;
  Prims prims(prims_->getSNLDesigns().begin(), prims_->getSNLDesigns().end());
  //logic0
  EXPECT_EQ(prims[0]->getName().getString(), "logic0");
  EXPECT_TRUE(SNLDesignTruthTable::isConst0(prims[0]));
  //logic1
  EXPECT_EQ(prims[1]->getName().getString(), "logic1");
  EXPECT_TRUE(SNLDesignTruthTable::isConst1(prims[1]));
  //inv
  EXPECT_EQ(prims[2]->getName().getString(), "inv");
  EXPECT_TRUE(SNLDesignTruthTable::isInv(prims[2]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst0(prims[2]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst1(prims[2]));
  EXPECT_FALSE(SNLDesignTruthTable::isBuf(prims[2]));
  //buf
  EXPECT_EQ(prims[3]->getName().getString(), "buf");
  EXPECT_TRUE(SNLDesignTruthTable::isBuf(prims[3]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst0(prims[3]));
  EXPECT_FALSE(SNLDesignTruthTable::isConst1(prims[3]));
  EXPECT_FALSE(SNLDesignTruthTable::isInv(prims[3]));

  //return false for primitive with no truth table
  auto design = SNLDesign::create(prims_, SNLDesign::Type::Primitive, SNLName("prim"));
  EXPECT_FALSE(SNLDesignTruthTable::isConst0(design));
  EXPECT_FALSE(SNLDesignTruthTable::isConst1(design));
  EXPECT_FALSE(SNLDesignTruthTable::isInv(design));
  EXPECT_FALSE(SNLDesignTruthTable::isBuf(design));

  //error when setting truth table on non-primitive
  auto designs = SNLLibrary::create(prims_->getDB());
  auto non_prim = SNLDesign::create(designs, SNLName("non_prim"));
  EXPECT_THROW(SNLDesignTruthTable::setTruthTable(non_prim, SNLTruthTable(1, 0b1)), SNLException);

  //error with more than one output
  auto error = SNLDesign::create(prims_, SNLDesign::Type::Primitive, SNLName("error"));
  auto i = SNLScalarTerm::create(error, SNLTerm::Direction::Input, SNLName("I"));
  auto o0 = SNLScalarTerm::create(error, SNLTerm::Direction::Output, SNLName("O0"));
  auto o1 = SNLScalarTerm::create(error, SNLTerm::Direction::Output, SNLName("O1"));
  EXPECT_THROW(SNLDesignTruthTable::setTruthTable(error, SNLTruthTable(1, 0b10)), SNLException);
} 