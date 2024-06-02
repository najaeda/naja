// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gmock/gmock.h"
#include "gtest/gtest.h"
using ::testing::ElementsAre;

#include "SNLDesignTruthTable.h"
#include "SNLException.h"
#include "SNLLibraryTruthTables.h"
#include "SNLPyLoader.h"
#include "SNLScalarTerm.h"
#include "SNLUniverse.h"
using namespace naja::SNL;

#include "ConstantPropagation.h"
#include "DNL.h"
#include "NetlistGraph.h"
#include "Reduction.h"
#include "RemoveLoadlessLogic.h"
#include "SNLBitNetOccurrence.h"
#include "SNLBitTermOccurrence.h"
#include "SNLEquipotential.h"
#include "SNLException.h"
#include "SNLInstTerm.h"
#include "SNLPath.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLUniverse.h"
#include "gtest/gtest.h"
#include "tbb/scalable_allocator.h"
#ifndef SNL_PRIMITIVES_TEST_PATH
#define SNL_PRIMITIVES_TEST_PATH
#endif

using namespace naja;
using namespace naja::DNL;
using namespace naja::SNL;
using namespace naja::NAJA_OPT;

class ReductionOptTests : public ::testing::Test {
 protected:
  void SetUp() override { SNLUniverse::create(); }
  void TearDown() override {
    if (SNLUniverse::get()) {
      SNLUniverse::get()->destroy();
    }
  }
};

TEST_F(ReductionOptTests, test) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto library =
      SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "../snl/python/pyloader/scripts/";
  primitives0Path /= "primitives1.py";
  SNLPyLoader::loadPrimitives(library, primitives0Path);
  ASSERT_EQ(13, library->getDesigns().size());
  auto logic0 = library->getDesign(SNLName("LOGIC0"));
  EXPECT_NE(nullptr, logic0);
  EXPECT_TRUE(logic0->isPrimitive());
  auto logic0TruthTable = SNLDesignTruthTable::getTruthTable(logic0);
  EXPECT_TRUE(logic0TruthTable.isInitialized());
  EXPECT_EQ(0, logic0TruthTable.size());
  EXPECT_TRUE(logic0TruthTable.is0());

  auto logic1 = library->getDesign(SNLName("LOGIC1"));
  EXPECT_NE(nullptr, logic1);
  EXPECT_TRUE(logic1->isPrimitive());
  auto logic1TruthTable = SNLDesignTruthTable::getTruthTable(logic1);
  EXPECT_TRUE(logic1TruthTable.isInitialized());
  EXPECT_EQ(0, logic1TruthTable.size());
  EXPECT_TRUE(logic1TruthTable.is1());

  auto and2 = library->getDesign(SNLName("AND2"));
  EXPECT_NE(nullptr, and2);
  EXPECT_TRUE(and2->isPrimitive());
  auto and2TruthTable = SNLDesignTruthTable::getTruthTable(and2);
  EXPECT_TRUE(and2TruthTable.isInitialized());
  EXPECT_EQ(2, and2TruthTable.size());
  EXPECT_EQ(SNLTruthTable(2, 0x8), and2TruthTable);
}

TEST_F(ReductionOptTests, testTruthTablesMap) {
  auto db = SNLDB::create(SNLUniverse::get());
  auto library =
      SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("nangate45"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "../snl/python/pyloader/scripts/";
  primitives0Path /= "primitives1.py";
  SNLPyLoader::loadPrimitives(library, primitives0Path);
  ASSERT_EQ(13, library->getDesigns().size());

  auto truthTables = SNLLibraryTruthTables::getTruthTables(library);

  auto logic0 = library->getDesign(SNLName("LOGIC0"));
  auto logic1 = library->getDesign(SNLName("LOGIC1"));

  auto buf = library->getDesign(SNLName("BUF"));
  ASSERT_NE(nullptr, buf);
  auto bufTruthTable = SNLDesignTruthTable::getTruthTable(buf);
  ASSERT_TRUE(bufTruthTable.isInitialized());
  auto tt = bufTruthTable.getReducedWithConstant(0, 0);
  auto result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  ASSERT_NE(nullptr, result.first);
  EXPECT_EQ(result.first, logic0);
  tt = bufTruthTable.getReducedWithConstant(0, 1);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  auto design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic1);

  auto inv = library->getDesign(SNLName("INV"));
  ASSERT_NE(nullptr, inv);
  auto invTruthTable = SNLDesignTruthTable::getTruthTable(inv);
  ASSERT_TRUE(invTruthTable.isInitialized());
  tt = invTruthTable.getReducedWithConstant(0, 0);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic1);
  tt = invTruthTable.getReducedWithConstant(0, 1);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic0);

  auto and2 = library->getDesign(SNLName("AND2"));
  ASSERT_NE(nullptr, and2);
  auto and2TruthTable = SNLDesignTruthTable::getTruthTable(and2);
  ASSERT_TRUE(and2TruthTable.isInitialized());
  tt = and2TruthTable.getReducedWithConstant(0, 0);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic0);

  auto or4 = library->getDesign(SNLName("OR4"));
  ASSERT_NE(nullptr, or4);
  auto or4TruthTable = SNLDesignTruthTable::getTruthTable(or4);
  ASSERT_TRUE(or4TruthTable.isInitialized());
  tt = or4TruthTable.getReducedWithConstant(0, 1);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic1);
  tt = or4TruthTable.getReducedWithConstant(0, 0);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getDesign(SNLName("OR3")));

  auto xor2 = library->getDesign(SNLName("XOR2"));
  ASSERT_NE(nullptr, xor2);
  auto xor2TruthTable = SNLDesignTruthTable::getTruthTable(xor2);
  ASSERT_TRUE(xor2TruthTable.isInitialized());
  tt = xor2TruthTable.getReducedWithConstant(0, 0);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getDesign(SNLName("BUF")));

  tt = xor2TruthTable.getReducedWithConstant(0, 1);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getDesign(SNLName("INV")));

  auto xnor2 = library->getDesign(SNLName("XNOR2"));
  ASSERT_NE(nullptr, xnor2);
  auto xnor2TruthTable = SNLDesignTruthTable::getTruthTable(xnor2);
  ASSERT_TRUE(xnor2TruthTable.isInitialized());
  tt = xnor2TruthTable.getReducedWithConstant(0, 0);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getDesign(SNLName("INV")));

  tt = xnor2TruthTable.getReducedWithConstant(0, 1);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, library->getDesign(SNLName("BUF")));

  auto oai21 = library->getDesign(SNLName("OAI21"));
  ASSERT_NE(nullptr, oai21);
  auto oai21TruthTable = SNLDesignTruthTable::getTruthTable(oai21);
  ASSERT_TRUE(oai21TruthTable.isInitialized());
  tt = oai21TruthTable.getReducedWithConstant(0, 0);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, logic1);

  tt = oai21TruthTable.getReducedWithConstant(0, 1);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);

  auto mux2 = library->getDesign(SNLName("MUX2"));
  ASSERT_NE(nullptr, mux2);
  // 0: A, 1: B, 2: S
  auto mux2TruthTable = SNLDesignTruthTable::getTruthTable(mux2);
  ASSERT_TRUE(mux2TruthTable.isInitialized());
  // A=0
  tt = mux2TruthTable.getReducedWithConstant(0, 0);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, and2);

  // A=1
  tt = mux2TruthTable.getReducedWithConstant(0, 1);
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(0xB, tt.bits());
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  EXPECT_EQ(nullptr, design);  // no design for or2 with one inversed input

  // if S=0, then A, if S=1, then B
  tt = mux2TruthTable.getReducedWithConstant(2, 0);
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(0xA, tt.bits());
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, buf);
  auto indexes = result.second;
  EXPECT_EQ(1, indexes.size());
  EXPECT_EQ(1, indexes[0]);

  tt = mux2TruthTable.getReducedWithConstant(2, 1);
  result = SNLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, buf);
  indexes = result.second;
  EXPECT_EQ(1, indexes.size());
  EXPECT_EQ(0, indexes[0]);

  {
    SNLLibrary* library = SNLLibrary::create(db, SNLName("MYLIB"));
    // 2. Create a top model with one output
    SNLDesign* top = SNLDesign::create(library, SNLName("top"));
    SNLUniverse* univ = SNLUniverse::get();
    SNLDB* db = SNLDB::create(univ);
    univ->setTopDesign(top);
    auto topOut =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out"));
    auto topOut2 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out2"));
    auto topOut3 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("out3"));
    auto topIn =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, SNLName("in"));
    // 8. create a mux instance in top
    SNLInstance* muxInst = SNLInstance::create(top, mux2, SNLName("mux"));
    SNLInstance* muxInst2 = SNLInstance::create(top, mux2, SNLName("mux2"));
    SNLInstance* muxInst3 = SNLInstance::create(top, mux2, SNLName("mux3"));
    SNLInstance* logic0Inst =
        SNLInstance::create(top, logic0, SNLName("logic0"));
    SNLInstance* logic1Inst =
        SNLInstance::create(top, logic1, SNLName("logic1"));
    // 9. connect all instances inputs
    // SNLNet* net1 = SNLScalarNet::create(top, SNLName("logic_0_net"));
    SNLNet* net2 = SNLScalarNet::create(top, SNLName("constant_0_net"));
    SNLNet* net3 = SNLScalarNet::create(top, SNLName("mux_output_net"));
    SNLNet* net4 = SNLScalarNet::create(top, SNLName("input_net"));
    SNLNet* net5 = SNLScalarNet::create(top, SNLName("constant_1_net"));
    SNLNet* net6 = SNLScalarNet::create(top, SNLName("mux_output_net2"));
    SNLNet* net7 = SNLScalarNet::create(top, SNLName("mux_output_net3"));
    topIn->setNet(net4);
    // connect logic0 to mux
    muxInst->getInstTerm(mux2->getScalarTerm(SNLName("A")))->setNet(net2);
    // connect logic1 to mux
    // net2->setType(naja::SNL::SNLNet::Type::Assign1);
    (*logic0Inst->getInstTerms().begin())->setNet(net2);
    muxInst->getInstTerm(mux2->getScalarTerm(SNLName("B")))->setNet(net4);
    muxInst->getInstTerm(mux2->getScalarTerm(SNLName("S")))->setNet(net2);
    // connect the mux instance output to the top output
    muxInst->getInstTerm(mux2->getScalarTerm(SNLName("Z")))->setNet(net3);
    topOut->setNet(net3);
    muxInst2->getInstTerm(mux2->getScalarTerm(SNLName("A")))->setNet(net2);
    muxInst2->getInstTerm(mux2->getScalarTerm(SNLName("B")))->setNet(net5);
    muxInst2->getInstTerm(mux2->getScalarTerm(SNLName("S")))->setNet(net5);
    muxInst2->getInstTerm(mux2->getScalarTerm(SNLName("Z")))->setNet(net6);
    topOut2->setNet(net6);
    muxInst3->getInstTerm(mux2->getScalarTerm(SNLName("A")))->setNet(net4);
    muxInst3->getInstTerm(mux2->getScalarTerm(SNLName("B")))->setNet(net4);
    muxInst3->getInstTerm(mux2->getScalarTerm(SNLName("S")))->setNet(net4);
    muxInst3->getInstTerm(mux2->getScalarTerm(SNLName("Z")))->setNet(net7);
    topOut3->setNet(net7);
    ConstantPropagation cp;
    cp.setTruthTableEngine(true);
    cp.run();
    printf("partial constant readers: %lu\n", cp.getPartialConstantReaders().size());
    ReductionOptimization reductionOpt(cp.getPartialConstantReaders());
    reductionOpt.run();
  }
}