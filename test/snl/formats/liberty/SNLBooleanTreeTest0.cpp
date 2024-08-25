// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLBooleanTree.h"

using namespace naja::SNL;

class SNLBooleanTreeTest0: public ::testing::Test {
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

TEST_F(SNLBooleanTreeTest0, test00) {
  auto and2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, SNLName("AND2"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, SNLName("A"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, SNLName("B"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Output, SNLName("Y"));
  size_t pos = 0;
  auto a = SNLBooleanTree::parseInput(and2, "A & B", pos);
  EXPECT_EQ(a, and2->getScalarTerm(SNLName("A")));
  EXPECT_EQ(pos, 1);
  pos = 4;
  auto b = SNLBooleanTree::parseInput(and2, "A & B", pos);
  EXPECT_EQ(b, and2->getScalarTerm(SNLName("B")));
  EXPECT_EQ(pos, 5);
}

TEST_F(SNLBooleanTreeTest0, test01) {
  auto and2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, SNLName("AND2"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, SNLName("A"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, SNLName("B"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Output, SNLName("Y"));
  SNLBooleanTree::parse(and2, "A & B");
}

TEST_F(SNLBooleanTreeTest0, test10) {
  auto and2 = SNLDesign::create(library_, SNLDesign::Type::Primitive, SNLName("ANDN2"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, SNLName("A1"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Input, SNLName("A2"));
  SNLScalarTerm::create(and2, SNLTerm::Direction::Output, SNLName("Y"));
  size_t pos = 0;
  auto a1 = SNLBooleanTree::parseInput(and2, "A1 & 'A2", pos);
  EXPECT_EQ(a1, and2->getScalarTerm(SNLName("A1")));
  EXPECT_EQ(pos, 2);
  pos = 6;
  auto a2 = SNLBooleanTree::parseInput(and2, "A1 & 'A2", pos);
  EXPECT_EQ(a2, and2->getScalarTerm(SNLName("A2")));
  EXPECT_EQ(pos, 8);
}