
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLDesignTruthTable.h"
using namespace naja::SNL;

class SNLDesignTruthTableTest: public ::testing::Test {
  protected:
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLDesignTruthTableTest, testTruthTablesError) {
  //Create primitives
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto prims = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, SNLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("I1"));
  auto o0 = SNLScalarTerm::create(design, SNLTerm::Direction::Output, SNLName("O0"));
  //size discrepancy error
  EXPECT_THROW(SNLDesignTruthTable::setTruthTable(design, SNLTruthTable(3, 0x5)), SNLException);
}

TEST_F(SNLDesignTruthTableTest, testTruthTablesConflictError) {
  //Create primitives
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto prims = SNLLibrary::create(db, SNLLibrary::Type::Primitives);
  auto design = SNLDesign::create(prims, SNLDesign::Type::Primitive, SNLName("design"));
  auto i0 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("I0"));
  auto i1 = SNLScalarTerm::create(design, SNLTerm::Direction::Input, SNLName("I1"));
  auto o = SNLScalarTerm::create(design, SNLTerm::Direction::Output, SNLName("O"));
  //set truth table
  SNLDesignTruthTable::setTruthTable(design, SNLTruthTable(2, 0x5));
  EXPECT_THROW(SNLDesignTruthTable::setTruthTable(design, SNLTruthTable(2, 0x1)), SNLException);
}