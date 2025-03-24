// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gmock/gmock.h"
#include "gtest/gtest.h"
using ::testing::ElementsAre;

#include "SNLDesignTruthTable.h"
#include "NLUniverse.h"
#include "NLLibraryTruthTables.h"
#include "SNLPyLoader.h"
#include "SNLScalarTerm.h"
#include "Utils.h"
#include "ConstantPropagation.h"
#include "DNL.h"
#include "NetlistGraph.h"
#include "Reduction.h"
#include "RemoveLoadlessLogic.h"
#include "SNLInstTerm.h"
#include "SNLPath.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"

#ifndef SNL_PRIMITIVES_TEST_PATH
#define SNL_PRIMITIVES_TEST_PATH
#endif

namespace {

void executeCommand(const std::string& command) {
  int result = system(command.c_str());
  if (result != 0) {
    std::cerr << "Command execution failed." << std::endl;
  }
}

}

using namespace naja;
using namespace naja::DNL;
using namespace naja::NL;
using namespace naja::NAJA_OPT;

class ReductionOptTests : public ::testing::Test {
 protected:
  void SetUp() override { NLUniverse::create(); }
  void TearDown() override {
    if (NLUniverse::get()) {
      NLUniverse::get()->destroy();
    }
  }
};

TEST_F(ReductionOptTests, test) {
  auto db = NLDB::create(NLUniverse::get());
  auto library = NLLibrary::create(db, NLLibrary::Type::Primitives,
                                    NLName("nangate45"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "../nl/python/pyloader/scripts/";
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

TEST_F(ReductionOptTests, testTruthTablesMap) {
  auto db = NLDB::create(NLUniverse::get());
  auto library = NLLibrary::create(db, NLLibrary::Type::Primitives,
                                    NLName("nangate45"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "../nl/python/pyloader/scripts/";
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
  // 0: A, 1: B, 2: S
  auto mux2TruthTable = SNLDesignTruthTable::getTruthTable(mux2);
  ASSERT_TRUE(mux2TruthTable.isInitialized());
  // A=0
  tt = mux2TruthTable.getReducedWithConstant(0, 0);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, and2);

  // A=1
  tt = mux2TruthTable.getReducedWithConstant(0, 1);
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(0xB, tt.bits());
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  EXPECT_EQ(nullptr, design);  // no design for or2 with one inversed input

  // if S=0, then A, if S=1, then B
  tt = mux2TruthTable.getReducedWithConstant(2, 0);
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(0xA, tt.bits());
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

  {
    NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
    // 2. Create a top model with one output
    SNLDesign* top = SNLDesign::create(library, NLName("top"));
    NLUniverse* univ = NLUniverse::get();
    NLDB* db = NLDB::create(univ);
    univ->setTopDesign(top);
    auto topOut =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
    auto topOut2 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
    auto topOut3 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out3"));
    auto topOut4 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out4"));
    auto topOut5 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out5"));
    auto topOut6 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out6"));
    auto topOut7 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out7"));
    auto topOut8 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out8"));
    auto topOut9 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out9"));
    auto topOut10 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out10"));
    auto topOut11 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out11"));
    auto topOut12 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out12"));
    auto topIn =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
    SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
    auto modOut =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
    auto modOut2 =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out2"));
    auto modOut3 =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out3"));
    auto modOut4 =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out4"));
    auto modOut5 =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out5"));
    auto modOut6 =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out6"));
    auto modIn =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("in"));
    // 8. create a mux instance in top
    SNLInstance* modInst = SNLInstance::create(top, mod, NLName("mod"));
    SNLInstance* modInst2 = SNLInstance::create(top, mod, NLName("mod2"));
    SNLInstance* muxInst = SNLInstance::create(mod, mux2, NLName("mux"));
    SNLInstance* muxInst2 = SNLInstance::create(mod, mux2, NLName("mux2"));
    SNLInstance* muxInst3 = SNLInstance::create(mod, mux2, NLName("mux3"));
    SNLInstance* muxInst4 = SNLInstance::create(mod, mux2, NLName("mux4"));
    SNLInstance* muxInst5 = SNLInstance::create(mod, mux2, NLName("mux5"));
    SNLInstance* muxInst6 = SNLInstance::create(mod, mux2, NLName("mux6"));
    SNLInstance* logic0Inst =
        SNLInstance::create(mod, logic0, NLName("logic0"));
    SNLInstance* logic1Inst =
        SNLInstance::create(mod, logic1, NLName("logic1"));
    // 9. connect all instances inputs
    // SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
    //SNLNet* net2 = SNLScalarNet::create(mod, NLName("constant_0_net"));
    SNLNet* net3 = SNLScalarNet::create(top, NLName("mux_output_net"));
    SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
    SNLNet* net5 = SNLScalarNet::create(top, NLName("constant_1_net"));
    SNLNet* net6 = SNLScalarNet::create(top, NLName("mux_output_net2"));
    SNLNet* net7 = SNLScalarNet::create(top, NLName("mux_output_net3"));
    SNLNet* net8 = SNLScalarNet::create(top, NLName("mux_output_net4"));
    SNLNet* net9 = SNLScalarNet::create(top, NLName("mux_output_net5"));
    SNLNet* net10 = SNLScalarNet::create(top, NLName("mux_output_net6"));
    SNLNet* net11 = SNLScalarNet::create(top, NLName("mod2out1"));
    SNLNet* net12 = SNLScalarNet::create(top, NLName("mod2out2"));
    SNLNet* net13 = SNLScalarNet::create(top, NLName("mod2out3"));
    SNLNet* net14 = SNLScalarNet::create(top, NLName("mod2out4"));
    SNLNet* net15 = SNLScalarNet::create(top, NLName("mod2out5"));
    SNLNet* net16 = SNLScalarNet::create(top, NLName("mod2out6"));
    SNLNet* net2mod = SNLScalarNet::create(mod, NLName("constant_0_net"));
    SNLNet* net3mod = SNLScalarNet::create(mod, NLName("mux_output_net"));
    SNLNet* net4mod = SNLScalarNet::create(mod, NLName("input_net"));
    SNLNet* net5mod = SNLScalarNet::create(mod, NLName("constant_1_net"));
    SNLNet* net6mod = SNLScalarNet::create(mod, NLName("mux_output_net2"));
    SNLNet* net7mod = SNLScalarNet::create(mod, NLName("mux_output_net3"));
    SNLNet* net8mod = SNLScalarNet::create(mod, NLName("mux_output_net4"));
    SNLNet* net9mod = SNLScalarNet::create(mod, NLName("mux_output_net5"));
    SNLNet* net10mod = SNLScalarNet::create(mod, NLName("mux_output_net6"));

    //Connect all instances inside mod
    
    // connect logic0 to mux
    
    // connect logic1 to mux
    // net2->setType(naja::SNL::SNLNet::Type::Assign1);
    (*logic0Inst->getInstTerms().begin())->setNet(net2mod);
    (*logic1Inst->getInstTerms().begin())->setNet(net5mod);

    modIn->setNet(net6mod);

    //Twins muxes 1 and 4
    muxInst->getInstTerm(mux2->getScalarTerm(NLName("A")))->setNet(net2mod);
    muxInst4->getInstTerm(mux2->getScalarTerm(NLName("A")))->setNet(net2mod);
    muxInst->getInstTerm(mux2->getScalarTerm(NLName("B")))->setNet(net4mod);
    muxInst4->getInstTerm(mux2->getScalarTerm(NLName("B")))->setNet(net4mod);
    muxInst->getInstTerm(mux2->getScalarTerm(NLName("S")))->setNet(net2mod);
    muxInst4->getInstTerm(mux2->getScalarTerm(NLName("S")))->setNet(net2mod);
    // connect the mux instance output to the top output
    muxInst->getInstTerm(mux2->getScalarTerm(NLName("Z")))->setNet(net3mod);
    modOut->setNet(net3mod);
    muxInst4->getInstTerm(mux2->getScalarTerm(NLName("Z")))->setNet(net7mod);
    modOut4->setNet(net7mod);

   //Twins muxes 2 and 5
    muxInst2->getInstTerm(mux2->getScalarTerm(NLName("A")))->setNet(net2mod);
    muxInst5->getInstTerm(mux2->getScalarTerm(NLName("A")))->setNet(net2mod);
    muxInst2->getInstTerm(mux2->getScalarTerm(NLName("B")))->setNet(net5mod);
    muxInst5->getInstTerm(mux2->getScalarTerm(NLName("B")))->setNet(net5mod);
    muxInst2->getInstTerm(mux2->getScalarTerm(NLName("S")))->setNet(net5mod);
    muxInst5->getInstTerm(mux2->getScalarTerm(NLName("S")))->setNet(net5mod);
    muxInst2->getInstTerm(mux2->getScalarTerm(NLName("Z")))->setNet(net6mod);
    modOut2->setNet(net6mod);
    muxInst5->getInstTerm(mux2->getScalarTerm(NLName("Z")))->setNet(net9mod);
    modOut5->setNet(net9mod);
    
    //Twins muxes 3 and 6
    muxInst3->getInstTerm(mux2->getScalarTerm(NLName("A")))->setNet(net4mod);
    muxInst6->getInstTerm(mux2->getScalarTerm(NLName("A")))->setNet(net4mod);
    muxInst3->getInstTerm(mux2->getScalarTerm(NLName("B")))->setNet(net4mod);
    muxInst6->getInstTerm(mux2->getScalarTerm(NLName("B")))->setNet(net4mod);
    muxInst3->getInstTerm(mux2->getScalarTerm(NLName("S")))->setNet(net5mod);
    muxInst6->getInstTerm(mux2->getScalarTerm(NLName("S")))->setNet(net5mod);
    muxInst3->getInstTerm(mux2->getScalarTerm(NLName("Z")))->setNet(net7mod);
    muxInst6->getInstTerm(mux2->getScalarTerm(NLName("Z")))->setNet(net10mod);
    modOut3->setNet(net7mod);
    modOut6->setNet(net10mod);


    //Connect all instances under top
    topIn->setNet(net4);
    
    
    modInst->getInstTerm(modIn)->setNet(net4);
    modInst->getInstTerm(modOut)->setNet(net3);
    topOut->setNet(net3);
    modInst->getInstTerm(modOut2)->setNet(net6);
    topOut2->setNet(net6);
    modInst->getInstTerm(modOut3)->setNet(net7);
    topOut3->setNet(net7);
    modInst->getInstTerm(modOut4)->setNet(net8);
    topOut4->setNet(net8);
    modInst->getInstTerm(modOut5)->setNet(net9);
    topOut5->setNet(net9);
    modInst->getInstTerm(modOut6)->setNet(net10);
    topOut6->setNet(net10);

    modInst2->getInstTerm(modIn)->setNet(net4);
    modInst2->getInstTerm(modOut)->setNet(net11);
    topOut7->setNet(net11);
    modInst2->getInstTerm(modOut2)->setNet(net12);
    topOut8->setNet(net12);
    modInst2->getInstTerm(modOut3)->setNet(net13);
    topOut9->setNet(net13);
    modInst2->getInstTerm(modOut4)->setNet(net14);
    topOut10->setNet(net14);
    modInst2->getInstTerm(modOut5)->setNet(net15);
    topOut11->setNet(net15);
    modInst2->getInstTerm(modOut6)->setNet(net16);
    topOut12->setNet(net16);

    ConstantPropagation cp;
    cp.setTruthTableEngine(true);
    cp.setNormalizedUniquification(false);
    cp.run();
    {
      std::string dotFileName(
          std::string(std::string("./afterCP") + std::string(".dot")));
      std::string svgFileName(
          std::string(std::string("./afterCP") + std::string(".svg")));
      SnlVisualiser snl(top);
      snl.process();
      snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
      executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                         std::string(" -o ") + svgFileName)
                 .c_str());
    }
    printf("partial constant readers: %lu\n",
           cp.getPartialConstantReaders().size());
    ReductionOptimization reductionOpt(cp.getPartialConstantReaders());
    reductionOpt.setNormalizedUniquification(false);
    reductionOpt.run();
    reductionOpt.collectStatistics(); 
  }
  NetlistStatistics netlistStats(*get());
  netlistStats.process();
  destroy();
  printf("Netlist statistics: %s\n", netlistStats.getReport().c_str());
}



TEST_F(ReductionOptTests, test_bne) {
  auto db = NLDB::create(NLUniverse::get());
  auto library = NLLibrary::create(db, NLLibrary::Type::Primitives,
                                    NLName("nangate45"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "../nl/python/pyloader/scripts/";
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

TEST_F(ReductionOptTests, testTruthTablesMap_bne) {
  auto db = NLDB::create(NLUniverse::get());
  auto library = NLLibrary::create(db, NLLibrary::Type::Primitives,
                                    NLName("nangate45"));
  auto primitives0Path = std::filesystem::path(SNL_PRIMITIVES_TEST_PATH);
  primitives0Path /= "../nl/python/pyloader/scripts/";
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
  // 0: A, 1: B, 2: S
  auto mux2TruthTable = SNLDesignTruthTable::getTruthTable(mux2);
  ASSERT_TRUE(mux2TruthTable.isInitialized());
  // A=0
  tt = mux2TruthTable.getReducedWithConstant(0, 0);
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  ASSERT_NE(nullptr, design);
  EXPECT_EQ(design, and2);

  // A=1
  tt = mux2TruthTable.getReducedWithConstant(0, 1);
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(0xB, tt.bits());
  result = NLLibraryTruthTables::getDesignForTruthTable(library, tt);
  design = result.first;
  EXPECT_EQ(nullptr, design);  // no design for or2 with one inversed input

  // if S=0, then A, if S=1, then B
  tt = mux2TruthTable.getReducedWithConstant(2, 0);
  EXPECT_EQ(2, tt.size());
  EXPECT_EQ(0xA, tt.bits());
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

  {
    NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
    // 2. Create a top model with one output
    SNLDesign* top = SNLDesign::create(library, NLName("top"));
    NLUniverse* univ = NLUniverse::get();
    NLDB* db = NLDB::create(univ);
    univ->setTopDesign(top);
    auto topOut =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out"));
    auto topOut2 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out2"));
    auto topOut3 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out3"));
    auto topOut4 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out4"));
    auto topOut5 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out5"));
    auto topOut6 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out6"));
    auto topOut7 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out7"));
    auto topOut8 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out8"));
    auto topOut9 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out9"));
    auto topOut10 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out10"));
    auto topOut11 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out11"));
    auto topOut12 =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("out12"));
    auto topIn =
        SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("in"));
    SNLDesign* mod = SNLDesign::create(library, NLName("mod"));
    auto modOut =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));
    auto modOut2 =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out2"));
    auto modOut3 =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out3"));
    auto modOut4 =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out4"));
    auto modOut5 =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out5"));
    auto modOut6 =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out6"));
    auto modIn =
        SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("in"));
    // 8. create a mux instance in top
    SNLInstance* modInst = SNLInstance::create(top, mod, NLName("mod"));
    SNLInstance* modInst2 = SNLInstance::create(top, mod, NLName("mod2"));
    SNLInstance* muxInst = SNLInstance::create(mod, mux2, NLName("mux"));
    SNLInstance* muxInst2 = SNLInstance::create(mod, mux2, NLName("mux2"));
    SNLInstance* muxInst3 = SNLInstance::create(mod, mux2, NLName("mux3"));
    SNLInstance* muxInst4 = SNLInstance::create(mod, mux2, NLName("mux4"));
    SNLInstance* muxInst5 = SNLInstance::create(mod, mux2, NLName("mux5"));
    SNLInstance* muxInst6 = SNLInstance::create(mod, mux2, NLName("mux6"));
    SNLInstance* logic0Inst =
        SNLInstance::create(mod, logic0, NLName("logic0"));
    SNLInstance* logic1Inst =
        SNLInstance::create(mod, logic1, NLName("logic1"));
    // 9. connect all instances inputs
    // SNLNet* net1 = SNLScalarNet::create(top, NLName("logic_0_net"));
    //SNLNet* net2 = SNLScalarNet::create(mod, NLName("constant_0_net"));
    SNLNet* net3 = SNLScalarNet::create(top, NLName("mux_output_net"));
    SNLNet* net4 = SNLScalarNet::create(top, NLName("input_net"));
    SNLNet* net5 = SNLScalarNet::create(top, NLName("constant_1_net"));
    SNLNet* net6 = SNLScalarNet::create(top, NLName("mux_output_net2"));
    SNLNet* net7 = SNLScalarNet::create(top, NLName("mux_output_net3"));
    SNLNet* net8 = SNLScalarNet::create(top, NLName("mux_output_net4"));
    SNLNet* net9 = SNLScalarNet::create(top, NLName("mux_output_net5"));
    SNLNet* net10 = SNLScalarNet::create(top, NLName("mux_output_net6"));
    SNLNet* net11 = SNLScalarNet::create(top, NLName("mod2out1"));
    SNLNet* net12 = SNLScalarNet::create(top, NLName("mod2out2"));
    SNLNet* net13 = SNLScalarNet::create(top, NLName("mod2out3"));
    SNLNet* net14 = SNLScalarNet::create(top, NLName("mod2out4"));
    SNLNet* net15 = SNLScalarNet::create(top, NLName("mod2out5"));
    SNLNet* net16 = SNLScalarNet::create(top, NLName("mod2out6"));
    SNLNet* net2mod = SNLScalarNet::create(mod, NLName("constant_0_net"));
    SNLNet* net3mod = SNLScalarNet::create(mod, NLName("mux_output_net"));
    SNLNet* net4mod = SNLScalarNet::create(mod, NLName("input_net"));
    SNLNet* net5mod = SNLScalarNet::create(mod, NLName("constant_1_net"));
    SNLNet* net6mod = SNLScalarNet::create(mod, NLName("mux_output_net2"));
    SNLNet* net7mod = SNLScalarNet::create(mod, NLName("mux_output_net3"));
    SNLNet* net8mod = SNLScalarNet::create(mod, NLName("mux_output_net4"));
    SNLNet* net9mod = SNLScalarNet::create(mod, NLName("mux_output_net5"));
    SNLNet* net10mod = SNLScalarNet::create(mod, NLName("mux_output_net6"));

    //Connect all instances inside mod
    
    // connect logic0 to mux
    
    // connect logic1 to mux
    // net2->setType(naja::SNL::SNLNet::Type::Assign1);
    (*logic0Inst->getInstTerms().begin())->setNet(net2mod);
    (*logic1Inst->getInstTerms().begin())->setNet(net5mod);

    modIn->setNet(net6mod);

    //Twins muxes 1 and 4
    muxInst->getInstTerm(mux2->getScalarTerm(NLName("A")))->setNet(net2mod);
    muxInst4->getInstTerm(mux2->getScalarTerm(NLName("A")))->setNet(net2mod);
    muxInst->getInstTerm(mux2->getScalarTerm(NLName("B")))->setNet(net4mod);
    muxInst4->getInstTerm(mux2->getScalarTerm(NLName("B")))->setNet(net4mod);
    muxInst->getInstTerm(mux2->getScalarTerm(NLName("S")))->setNet(net2mod);
    muxInst4->getInstTerm(mux2->getScalarTerm(NLName("S")))->setNet(net2mod);
    // connect the mux instance output to the top output
    muxInst->getInstTerm(mux2->getScalarTerm(NLName("Z")))->setNet(net3mod);
    modOut->setNet(net3mod);
    muxInst4->getInstTerm(mux2->getScalarTerm(NLName("Z")))->setNet(net7mod);
    modOut4->setNet(net7mod);

   //Twins muxes 2 and 5
    muxInst2->getInstTerm(mux2->getScalarTerm(NLName("A")))->setNet(net2mod);
    muxInst5->getInstTerm(mux2->getScalarTerm(NLName("A")))->setNet(net2mod);
    muxInst2->getInstTerm(mux2->getScalarTerm(NLName("B")))->setNet(net5mod);
    muxInst5->getInstTerm(mux2->getScalarTerm(NLName("B")))->setNet(net5mod);
    muxInst2->getInstTerm(mux2->getScalarTerm(NLName("S")))->setNet(net5mod);
    muxInst5->getInstTerm(mux2->getScalarTerm(NLName("S")))->setNet(net5mod);
    muxInst2->getInstTerm(mux2->getScalarTerm(NLName("Z")))->setNet(net6mod);
    modOut2->setNet(net6mod);
    muxInst5->getInstTerm(mux2->getScalarTerm(NLName("Z")))->setNet(net9mod);
    modOut5->setNet(net9mod);
    
    //Twins muxes 3 and 6
    muxInst3->getInstTerm(mux2->getScalarTerm(NLName("A")))->setNet(net4mod);
    muxInst6->getInstTerm(mux2->getScalarTerm(NLName("A")))->setNet(net4mod);
    muxInst3->getInstTerm(mux2->getScalarTerm(NLName("B")))->setNet(net4mod);
    muxInst6->getInstTerm(mux2->getScalarTerm(NLName("B")))->setNet(net4mod);
    muxInst3->getInstTerm(mux2->getScalarTerm(NLName("S")))->setNet(net5mod);
    muxInst6->getInstTerm(mux2->getScalarTerm(NLName("S")))->setNet(net5mod);
    muxInst3->getInstTerm(mux2->getScalarTerm(NLName("Z")))->setNet(net7mod);
    muxInst6->getInstTerm(mux2->getScalarTerm(NLName("Z")))->setNet(net10mod);
    modOut3->setNet(net7mod);
    modOut6->setNet(net10mod);


    //Connect all instances under top
    topIn->setNet(net4);
    
    
    modInst->getInstTerm(modIn)->setNet(net4);
    modInst->getInstTerm(modOut)->setNet(net3);
    topOut->setNet(net3);
    modInst->getInstTerm(modOut2)->setNet(net6);
    topOut2->setNet(net6);
    modInst->getInstTerm(modOut3)->setNet(net7);
    topOut3->setNet(net7);
    modInst->getInstTerm(modOut4)->setNet(net8);
    topOut4->setNet(net8);
    modInst->getInstTerm(modOut5)->setNet(net9);
    topOut5->setNet(net9);
    modInst->getInstTerm(modOut6)->setNet(net10);
    topOut6->setNet(net10);

    modInst2->getInstTerm(modIn)->setNet(net4);
    modInst2->getInstTerm(modOut)->setNet(net11);
    topOut7->setNet(net11);
    modInst2->getInstTerm(modOut2)->setNet(net12);
    topOut8->setNet(net12);
    modInst2->getInstTerm(modOut3)->setNet(net13);
    topOut9->setNet(net13);
    modInst2->getInstTerm(modOut4)->setNet(net14);
    topOut10->setNet(net14);
    modInst2->getInstTerm(modOut5)->setNet(net15);
    topOut11->setNet(net15);
    modInst2->getInstTerm(modOut6)->setNet(net16);
    topOut12->setNet(net16);

    ConstantPropagation cp;
    cp.setTruthTableEngine(true);
    cp.run();
    {
      std::string dotFileName(
          std::string(std::string("./afterCP") + std::string(".dot")));
      std::string svgFileName(
          std::string(std::string("./afterCP") + std::string(".svg")));
      SnlVisualiser snl(top);
      snl.process();
      snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
      executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                         std::string(" -o ") + svgFileName)
                 .c_str());
    }
    printf("partial constant readers: %lu\n",
           cp.getPartialConstantReaders().size());
    ReductionOptimization reductionOpt(cp.getPartialConstantReaders());
    reductionOpt.setNormalizedUniquification(true);
    reductionOpt.run();
    reductionOpt.collectStatistics(); 
  }
  NetlistStatistics netlistStats(*get());
  netlistStats.process();
  printf("Netlist statistics: %s\n", netlistStats.getReport().c_str());
}
