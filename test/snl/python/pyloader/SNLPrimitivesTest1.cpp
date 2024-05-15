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
#include "SNLLibraryTruthTables.h"
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
}

TEST_F(SNLPrimitivesTest1, testTruthTablesMap) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto library = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "scripts";
  primitives0Path /= "primitives1.py";
  SNLPyLoader::loadPrimitives(library, primitives0Path);
  ASSERT_EQ(3, library->getDesigns().size());
  auto truthTables = SNLLibraryTruthTables::construct(library);

  auto logic0 = library->getDesign(SNLName("LOGIC0"));
  auto and2 = library->getDesign(SNLName("AND2"));
  ASSERT_NE(nullptr, and2);
  auto and2TruthTable = SNLDesignModeling::getTruthTable(and2);
  ASSERT_TRUE(and2TruthTable.isInitialized());
  auto tt = and2TruthTable.getReducedWithConstant(0, 0);
  auto design = SNLLibraryTruthTables::getDesignForTruthTable(truthTables, tt);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic0);
}