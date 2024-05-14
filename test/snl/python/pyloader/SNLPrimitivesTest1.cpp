// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLDesignModeling.h"
#include "SNLPyLoader.h"
#include "SNLException.h"
using namespace naja::SNL;

#ifndef SNL_PRIMITIVES_TEST_PATH
#define SNL_PRIMITIVES_TEST_PATH "Undefined"
#endif

class SNLPrimitivesTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      SNLUniverse::create();
    }
    void TearDown() override {
      if (SNLUniverse::get()) {
        SNLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLPrimitivesTest1, test) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "scripts";
  primitives0Path /= "primitives1.py";
  SNLPyLoader::loadPrimitives(library, primitives0Path);
  ASSERT_EQ(3, library->getDesigns().size());
  auto logic0 = library->getDesign(SNLName("LOGIC0"));
  EXPECT_NE(nullptr, logic0);
  EXPECT_TRUE(logic0->isPrimitive());
  auto logic0TruthTable = SNLDesignModeling::getTruthTable(logic0);
  EXPECT_TRUE(logic0TruthTable.isInitialized());
  EXPECT_EQ(0, logic0TruthTable.size());
  EXPECT_TRUE(logic0TruthTable.is0());

  auto logic1 = library->getDesign(SNLName("LOGIC1"));
  EXPECT_NE(nullptr, logic1);
  EXPECT_TRUE(logic1->isPrimitive());
  auto logic1TruthTable = SNLDesignModeling::getTruthTable(logic1);
  EXPECT_TRUE(logic1TruthTable.isInitialized());
  EXPECT_EQ(0, logic1TruthTable.size());
  EXPECT_TRUE(logic1TruthTable.is1());

  auto and2 = library->getDesign(SNLName("AND2"));
  EXPECT_NE(nullptr, and2);
  EXPECT_TRUE(and2->isPrimitive());
  auto and2TruthTable = SNLDesignModeling::getTruthTable(and2);
  EXPECT_TRUE(and2TruthTable.isInitialized());
  EXPECT_EQ(2, and2TruthTable.size());
  EXPECT_EQ(SNLTruthTable(2, 0x8), and2TruthTable);

#if 0 
  EXPECT_TRUE(lut4->isPrimitive());
  ASSERT_NE(nullptr, lut4);
  ASSERT_EQ(5, lut4->getScalarTerms().size());
  using Terms = std::vector<SNLScalarTerm*>;
  Terms terms(lut4->getScalarTerms().begin(), lut4->getScalarTerms().end()); 
  ASSERT_EQ(5, terms.size());
  EXPECT_EQ("I0", terms[0]->getName().getString());
  EXPECT_EQ("I1", terms[1]->getName().getString());
  EXPECT_EQ("I2", terms[2]->getName().getString());
  EXPECT_EQ("I3", terms[3]->getName().getString());
  EXPECT_EQ("O", terms[4]->getName().getString());
  EXPECT_EQ(SNLTerm::Direction::Input, terms[0]->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, terms[1]->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, terms[2]->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Input, terms[3]->getDirection());
  EXPECT_EQ(SNLTerm::Direction::Output,  terms[4]->getDirection());
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialOutputs(terms[4]).empty());
  EXPECT_EQ(4, SNLDesignModeling::getCombinatorialInputs(terms[4]).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialInputs(terms[4]).begin(),
      SNLDesignModeling::getCombinatorialInputs(terms[4]).end()),
    ElementsAre(terms[0], terms[1], terms[2], terms[3]));
  EXPECT_TRUE(SNLDesignModeling::getCombinatorialInputs(terms[0]).empty());
  EXPECT_EQ(1, SNLDesignModeling::getCombinatorialOutputs(terms[0]).size());
  EXPECT_THAT(
    std::vector(
      SNLDesignModeling::getCombinatorialOutputs(terms[0]).begin(),
      SNLDesignModeling::getCombinatorialOutputs(terms[0]).end()),
    ElementsAre(terms[4]));
#endif
}

