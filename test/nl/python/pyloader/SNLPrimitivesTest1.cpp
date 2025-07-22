// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using ::testing::ElementsAre;

#include "NLUniverse.h"
#include "NLException.h"

#include "SNLScalarTerm.h"
#include "SNLDesignTruthTable.h"
#include "SNLPyLoader.h"
#include "NLLibraryTruthTables.h"
using namespace naja::NL;

#ifndef SNL_PRIMITIVES_TEST_PATH
#define SNL_PRIMITIVES_TEST_PATH "Undefined"
#endif

class SNLPrimitivesTest1: public ::testing::Test {
  protected:
    void SetUp() override {
      NLUniverse::create();
    }
    void TearDown() override {
      if (NLUniverse::get()) {
        NLUniverse::get()->destroy();
      }
    }
};

TEST_F(SNLPrimitivesTest1, test) {
  auto db = NLDB::create(NLUniverse::get());
  auto library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMS"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "scripts";
  primitives0Path /= "primitives1.py";
  SNLPyLoader::loadPrimitives(library, primitives0Path);
  ASSERT_EQ(13, library->getSNLDesigns().size());
  auto logic0 = library->getSNLDesign(NLName("LOGIC0"));
  EXPECT_NE(nullptr, logic0);
  EXPECT_TRUE(logic0->isPrimitive());
  auto logic0TruthTable = SNLDesignTruthTable::getTruthTable(logic0);
  EXPECT_TRUE(logic0TruthTable.isInitialized());
  EXPECT_EQ(0, logic0TruthTable.size());
  EXPECT_TRUE(logic0TruthTable.all0());

  auto logic1 = library->getSNLDesign(NLName("LOGIC1"));
  EXPECT_NE(nullptr, logic1);
  EXPECT_TRUE(logic1->isPrimitive());
  auto logic1TruthTable = SNLDesignTruthTable::getTruthTable(logic1);
  EXPECT_TRUE(logic1TruthTable.isInitialized());
  EXPECT_EQ(0, logic1TruthTable.size());
  EXPECT_TRUE(logic1TruthTable.all1());

  auto and2 = library->getSNLDesign(NLName("AND2"));
  EXPECT_NE(nullptr, and2);
  EXPECT_TRUE(and2->isPrimitive());
  auto and2TruthTable = SNLDesignTruthTable::getTruthTable(and2);
  EXPECT_TRUE(and2TruthTable.isInitialized());
  EXPECT_EQ(2, and2TruthTable.size());
  EXPECT_EQ(SNLTruthTable(2, 0x8), and2TruthTable);
}

TEST_F(SNLPrimitivesTest1, testTruthTablesMap) {
  auto db = NLDB::create(NLUniverse::get());
  auto library = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMS"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "scripts";
  primitives0Path /= "primitives1.py";
  SNLPyLoader::loadPrimitives(library, primitives0Path);
  ASSERT_EQ(13, library->getSNLDesigns().size());

  auto truthTables = NLLibraryTruthTables::getTruthTables(library);

  auto logic0 = library->getSNLDesign(NLName("LOGIC0"));
  auto logic1 = library->getSNLDesign(NLName("LOGIC1"));

  auto buf = library->getSNLDesign(NLName("BUF"));
  ASSERT_NE(nullptr, buf);
  auto bufTruthTable = SNLDesignTruthTable::getTruthTable(buf);
  ASSERT_TRUE(bufTruthTable.isInitialized());
  auto tt = bufTruthTable.getReducedWithConstant(0, 0);
  auto result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  ASSERT_NE(nullptr, result.first);
  EXPECT_EQ(result.first, logic0);
  tt = bufTruthTable.getReducedWithConstant(0, 1);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  auto design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic1);

  auto inv = library->getSNLDesign(NLName("INV"));
  ASSERT_NE(nullptr, inv);
  auto invTruthTable = SNLDesignTruthTable::getTruthTable(inv);
  ASSERT_TRUE(invTruthTable.isInitialized());
  tt = invTruthTable.getReducedWithConstant(0, 0);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic1);
  tt = invTruthTable.getReducedWithConstant(0, 1);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic0);

  auto and2 = library->getSNLDesign(NLName("AND2"));
  ASSERT_NE(nullptr, and2);
  auto and2TruthTable = SNLDesignTruthTable::getTruthTable(and2);
  ASSERT_TRUE(and2TruthTable.isInitialized());
  tt = and2TruthTable.getReducedWithConstant(0, 0);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic0);

  auto or4 = library->getSNLDesign(NLName("OR4"));
  ASSERT_NE(nullptr, or4);
  auto or4TruthTable = SNLDesignTruthTable::getTruthTable(or4);
  ASSERT_TRUE(or4TruthTable.isInitialized());
  tt = or4TruthTable.getReducedWithConstant(0, 1);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic1);
  tt = or4TruthTable.getReducedWithConstant(0, 0);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getSNLDesign(NLName("OR3")));

  auto xor2 = library->getSNLDesign(NLName("XOR2"));
  ASSERT_NE(nullptr, xor2);
  auto xor2TruthTable = SNLDesignTruthTable::getTruthTable(xor2);
  ASSERT_TRUE(xor2TruthTable.isInitialized());
  tt = xor2TruthTable.getReducedWithConstant(0, 0);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getSNLDesign(NLName("BUF")));

  tt = xor2TruthTable.getReducedWithConstant(0, 1);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getSNLDesign(NLName("INV")));

  auto xnor2 = library->getSNLDesign(NLName("XNOR2"));
  ASSERT_NE(nullptr, xnor2);
  auto xnor2TruthTable = SNLDesignTruthTable::getTruthTable(xnor2);
  ASSERT_TRUE(xnor2TruthTable.isInitialized());
  tt = xnor2TruthTable.getReducedWithConstant(0, 0);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getSNLDesign(NLName("INV")));

  tt = xnor2TruthTable.getReducedWithConstant(0, 1);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getSNLDesign(NLName("BUF")));

  auto oai21 = library->getSNLDesign(NLName("OAI21"));
  ASSERT_NE(nullptr, oai21);
  auto oai21TruthTable = SNLDesignTruthTable::getTruthTable(oai21);
  ASSERT_TRUE(oai21TruthTable.isInitialized());
  tt = oai21TruthTable.getReducedWithConstant(0, 0);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic1);

  tt = oai21TruthTable.getReducedWithConstant(0, 1);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);

  auto mux2 = library->getSNLDesign(NLName("MUX2"));
  ASSERT_NE(nullptr, mux2);
  //0: A, 1: B, 2: S
  auto mux2TruthTable = SNLDesignTruthTable::getTruthTable(mux2);
  ASSERT_TRUE(mux2TruthTable.isInitialized());
  //A=0
  tt = mux2TruthTable.getReducedWithConstant(0, 0);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, and2);

  //A=1
  tt = mux2TruthTable.getReducedWithConstant(0, 1);
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(NLBitVecDynamic(0xB, 4), tt.bits());
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  EXPECT_EQ(nullptr, design); //no design for or2 with one inversed input

  //if S=0, then A, if S=1, then B
  tt = mux2TruthTable.getReducedWithConstant(2, 0);
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(NLBitVecDynamic(0xA, 4), tt.bits());
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, buf);
  auto indexes = result.second;
  EXPECT_EQ(1, indexes.size());
  EXPECT_EQ(1, indexes[0]);

  tt = mux2TruthTable.getReducedWithConstant(2, 1);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, buf);
  indexes = result.second;
  EXPECT_EQ(1, indexes.size());
  EXPECT_EQ(0, indexes[0]);
}
