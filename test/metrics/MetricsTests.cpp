// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "tbb/scalable_allocator.h"

#include "DNL.h"
#include "FanoutComputer.h"
#include "LogicLevelComputer.h"
#include "NLException.h"
#include "NLUniverse.h"
#include "NetlistGraph.h"
#include "SNLOccurrence.h"
#include "SNLDesignModeling.h"
#include "SNLEquipotential.h"
#include "SNLInstTerm.h"
#include "SNLPath.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"

using namespace naja;
using namespace naja::DNL;
using namespace naja::NL;
using namespace naja::NAJA_METRICS;

void executeCommand(const std::string& command) {
  int result = system(command.c_str());
  if (result != 0) {
    std::cerr << "Command execution failed." << std::endl;
  }
}

class MetricsTests : public ::testing::Test {
 protected:
  MetricsTests() {
    // You can do set-up work for each test here
  }
  ~MetricsTests() override {
    // You can do clean-up work that doesn't throw exceptions here
  }
  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }
  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
    // Destroy the SNL
    NLUniverse::get()->destroy();
  }
};

// Building a test like the prvious only with 3 levels of hierarchy
TEST_F(MetricsTests, simpleTest) {
  // Create a simple logic with a single
  // input and output
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  NLLibrary* libraryP = NLLibrary::create(db, NLLibrary::Type::Primitives);
  SNLDesign* top = SNLDesign::create(library, NLName("top"));

  auto topInTerm =
      SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("topIn"));

  auto topOutTerm =
      SNLScalarTerm::create(top, SNLTerm::Direction::Output, NLName("topOut"));

  univ->setTopDesign(top);
  SNLDesign* mod = SNLDesign::create(library, NLName("mod"));

  auto inTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Input, NLName("in"));
  auto outTerm =
      SNLScalarTerm::create(mod, SNLTerm::Direction::Output, NLName("out"));

  SNLDesign* bb =
      SNLDesign::create(libraryP, SNLDesign::Type::Primitive, NLName("bb"));
  auto inTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Input, NLName("in"));
  auto inTermBB1 =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Input, NLName("in1"));
  auto outTermBB =
      SNLScalarTerm::create(bb, SNLTerm::Direction::Output, NLName("out"));
  // set combinatorial dependecies
  SNLDesignModeling::addCombinatorialArcs({inTermBB, inTermBB1}, {outTermBB});

  // create bbSeq
    SNLDesign* bbSeq = SNLDesign::create(
        libraryP, SNLDesign::Type::Primitive, NLName("bbSeq"));
    auto inTermBBseq =
        SNLScalarTerm::create(bbSeq, SNLTerm::Direction::Input, NLName("inseq"));
    auto outTermBBseq = SNLScalarTerm::create(
        bbSeq, SNLTerm::Direction::Output, NLName("outseq"));
    auto clk =
        SNLScalarTerm::create(bbSeq, SNLTerm::Direction::Input, NLName("clk"));
    // set sequential dependecies
    SNLDesignModeling::addInputsToClockArcs({inTermBBseq}, clk);
    SNLDesignModeling::addClockToOutputsArcs(clk, {outTermBBseq});

  SNLDesign* bbNoOutput = SNLDesign::create(
      libraryP, SNLDesign::Type::Primitive, NLName("bbNoOutput"));
  auto inTermBBno = SNLScalarTerm::create(bbNoOutput, SNLTerm::Direction::Input,
                                          NLName("in"));
  SNLDesign* bbNoOutputNoInput = SNLDesign::create(
      libraryP, SNLDesign::Type::Primitive, NLName("bbNoOutputNoInput"));

  SNLInstance* bb1 = SNLInstance::create(mod, bb, NLName("bb1"));
  SNLInstance* bb2 = SNLInstance::create(mod, bbNoOutput, NLName("bb2"));
  SNLInstance* bbnoni =
      SNLInstance::create(mod, bbNoOutputNoInput, NLName("bbnoni"));

  SNLDesign* hier = SNLDesign::create(library, NLName("hier"));
  auto inTermH = SNLScalarTerm::create(
      hier, SNLTerm::Direction::Input, NLName("in"));
  auto outTermH =
      SNLScalarTerm::create(hier, SNLTerm::Direction::Output, NLName("out"));
      

  SNLInstance* bb3 = SNLInstance::create(hier, bb, NLName("bb1"));
  SNLInstance* bb4 = SNLInstance::create(hier, bb, NLName("bb2"));

  auto inNet1 = SNLScalarNet::create(mod, NLName("inNet1"));
  bb1->getInstTerm(inTermBB)->setNet(inNet1);
  bb2->getInstTerm(inTermBBno)->setNet(inNet1);
  inTerm->setNet(inNet1);
  auto inNetHier = SNLScalarNet::create(hier, NLName("inNetHier"));
  auto outNetHier =
      SNLScalarNet::create(hier, NLName("outNetHier"));
  bb3->getInstTerm(inTermBB)->setNet(inNetHier);
  bb4->getInstTerm(inTermBB)->setNet(inNetHier);
  inTermH->setNet(inNetHier);
  bb3->getInstTerm(outTermBB)->setNet(outNetHier);
  bb3->getInstTerm(inTermBB1)->setNet(outNetHier);
  outTermH->setNet(outNetHier);
  SNLInstance* hi1 = SNLInstance::create(mod, hier, NLName("hi1"));
  SNLInstance* hi2 = SNLInstance::create(mod, hier, NLName("hi2"));
  hi1->getInstTerm(inTermH)->setNet(inNet1);
  hi2->getInstTerm(inTermH)->setNet(inNet1);
  SNLInstance* modInst1 = SNLInstance::create(top, mod, NLName("modInst1"));
  SNLInstance* modInst2 = SNLInstance::create(top, mod, NLName("modInst2"));
  SNLInstance* bbSeqInst1 =
      SNLInstance::create(top, bbSeq, NLName("bbSeqInst1"));
  auto outNetMod1 =
      SNLScalarNet::create(mod, NLName("outNetMod"));
  outTerm->setNet(outNetMod1);
  hi1->getInstTerm(outTermH)->setNet(outNetMod1);
  

  SNLNet* topInNet = SNLScalarNet::create(top, NLName("topInNet"));
  SNLNet* topOutNet = SNLScalarNet::create(top, NLName("topOutNet"));
  modInst1->getInstTerm(inTerm)->setNet(topInNet);
  modInst2->getInstTerm(outTerm)->setNet(topOutNet);
  topInTerm->setNet(topInNet);
  topOutTerm->setNet(topOutNet);
  SNLNet* concateInNet =
      SNLScalarNet::create(top, NLName("concateNet"));
  modInst1->getInstTerm(outTerm)->setNet(concateInNet);
  modInst2->getInstTerm(inTerm)->setNet(concateInNet);
  bbSeqInst1->getInstTerm(inTermBBseq)->setNet(concateInNet);
  {
    std::string dotFileName(
        std::string(std::string("./MetricsTest") + std::string(".dot")));
    std::string svgFileName(
        std::string(std::string("./MetricsTest") + std::string(".svg")));
    SnlVisualiser snl(top);
    snl.process();
    snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
    executeCommand(std::string(std::string("dot -Tsvg ") + dotFileName +
                               std::string(" -o ") + svgFileName)
                       .c_str());
  }
  LogicLevelComputer llc;
  llc.process();
  EXPECT_EQ(llc.getMaxLogicLevel(), 4);
  FanoutComputer fc;
  fc.process();
  EXPECT_EQ(fc.getMaxFanout(), 8);
}
