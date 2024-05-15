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
  ASSERT_EQ(11, library->getDesigns().size());
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
  ASSERT_EQ(11, library->getDesigns().size());
  auto truthTables = SNLLibraryTruthTables::construct(library);

  auto logic0 = library->getDesign(SNLName("LOGIC0"));
  auto logic1 = library->getDesign(SNLName("LOGIC1"));

  auto buf = library->getDesign(SNLName("BUF"));
  ASSERT_NE(nullptr, buf);
  auto bufTruthTable = SNLDesignModeling::getTruthTable(buf);
  ASSERT_TRUE(bufTruthTable.isInitialized());
  auto tt = bufTruthTable.getReducedWithConstant(0, 0);
  auto design = SNLLibraryTruthTables::getDesignForTruthTable(truthTables, tt);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic0);
  tt = bufTruthTable.getReducedWithConstant(0, 1);
  design = SNLLibraryTruthTables::getDesignForTruthTable(truthTables, tt);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic1);

  auto inv = library->getDesign(SNLName("INV"));
  ASSERT_NE(nullptr, inv);
  auto invTruthTable = SNLDesignModeling::getTruthTable(inv);
  ASSERT_TRUE(invTruthTable.isInitialized());
  tt = invTruthTable.getReducedWithConstant(0, 0);
  design = SNLLibraryTruthTables::getDesignForTruthTable(truthTables, tt);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic1);
  tt = invTruthTable.getReducedWithConstant(0, 1);
  design = SNLLibraryTruthTables::getDesignForTruthTable(truthTables, tt);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic0);

  auto and2 = library->getDesign(SNLName("AND2"));
  ASSERT_NE(nullptr, and2);
  auto and2TruthTable = SNLDesignModeling::getTruthTable(and2);
  ASSERT_TRUE(and2TruthTable.isInitialized());
  tt = and2TruthTable.getReducedWithConstant(0, 0);
  design = SNLLibraryTruthTables::getDesignForTruthTable(truthTables, tt);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic0);

  auto or4 = library->getDesign(SNLName("OR4"));
  ASSERT_NE(nullptr, or4);
  auto or4TruthTable = SNLDesignModeling::getTruthTable(or4);
  ASSERT_TRUE(or4TruthTable.isInitialized());
  tt = or4TruthTable.getReducedWithConstant(0, 1);
  design = SNLLibraryTruthTables::getDesignForTruthTable(truthTables, tt);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic1);
  tt = or4TruthTable.getReducedWithConstant(0, 0);
  design = SNLLibraryTruthTables::getDesignForTruthTable(truthTables, tt);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getDesign(SNLName("OR3")));

  auto xor2 = library->getDesign(SNLName("XOR2"));
  ASSERT_NE(nullptr, xor2);
  auto xor2TruthTable = SNLDesignModeling::getTruthTable(xor2);
  ASSERT_TRUE(xor2TruthTable.isInitialized());
  tt = xor2TruthTable.getReducedWithConstant(0, 0);
  design = SNLLibraryTruthTables::getDesignForTruthTable(truthTables, tt);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getDesign(SNLName("BUF")));

  tt = xor2TruthTable.getReducedWithConstant(0, 1);
  design = SNLLibraryTruthTables::getDesignForTruthTable(truthTables, tt);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getDesign(SNLName("INV")));

  auto xnor2 = library->getDesign(SNLName("XNOR2"));
  ASSERT_NE(nullptr, xnor2);
  auto xnor2TruthTable = SNLDesignModeling::getTruthTable(xnor2);
  ASSERT_TRUE(xnor2TruthTable.isInitialized());
  tt = xnor2TruthTable.getReducedWithConstant(0, 0);
  design = SNLLibraryTruthTables::getDesignForTruthTable(truthTables, tt);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getDesign(SNLName("INV")));

  tt = xnor2TruthTable.getReducedWithConstant(0, 1);
  design = SNLLibraryTruthTables::getDesignForTruthTable(truthTables, tt);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getDesign(SNLName("BUF")));

  auto oai21 = library->getDesign(SNLName("OAI21"));
  ASSERT_NE(nullptr, oai21);
  auto oai21TruthTable = SNLDesignModeling::getTruthTable(oai21);
  ASSERT_TRUE(oai21TruthTable.isInitialized());
  tt = oai21TruthTable.getReducedWithConstant(2, 0);
  std::cout << tt.getString() << std::endl;
  design = SNLLibraryTruthTables::getDesignForTruthTable(truthTables, tt);
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic1);
}