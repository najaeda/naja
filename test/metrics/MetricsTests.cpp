// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "tbb/scalable_allocator.h"

#include <functional>
#include <map>
#include <set>

#include "DNL.h"
#include "FanoutComputer.h"
#include "LogicCone.h"
#include "LogicLevelComputer.h"
#include "NLDB0.h"
#include "NLException.h"
#include "NLUniverse.h"
#include "NetlistGraph.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLOccurrence.h"
#include "SNLDesignModeling.h"
#include "SNLEquipotential.h"
#include "SNLInstTerm.h"
#include "SNLLogicalCone.h"
#include "SNLPath.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"

using namespace naja;
using namespace naja::DNL;
using namespace naja::NL;
using namespace naja::NAJA_METRICS;

void executeCommand(const std::string& command) {
#ifdef NAJA_ENABLE_TEST_DOT
  int result = system(command.c_str());
  if (result != 0) {
    std::cerr << "Command execution failed." << std::endl;
  }
#else
  (void) command;
#endif
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
    naja::DNL::destroy();
    NLUniverse::get()->destroy();
  }
};

namespace {

template <typename Cone>
std::map<SNLDesignObject*, typename Cone::NodeKind> collectNodeKinds(
    const Cone& cone) {
  std::map<SNLDesignObject*, typename Cone::NodeKind> result;
  for (const auto& node : cone.getNodes()) {
    result.emplace(node.occurrence.getObject(), node.kind);
  }
  return result;
}

template <typename Cone>
std::set<std::pair<SNLDesignObject*, SNLDesignObject*>> collectEdges(
    const Cone& cone) {
  std::set<std::pair<SNLDesignObject*, SNLDesignObject*>> result;
  const auto& nodes = cone.getNodes();
  for (const auto& node : nodes) {
    for (auto next : node.next) {
      result.emplace(
          node.occurrence.getObject(),
          nodes[next].occurrence.getObject());
    }
  }
  return result;
}

template <typename Cone>
std::set<SNLDesignObject*> collectLeaves(const Cone& cone) {
  std::set<SNLDesignObject*> result;
  for (auto leaf : cone.getLeaves()) {
    result.insert(cone.getNodes()[leaf].occurrence.getObject());
  }
  return result;
}

void expectSameCone(const SNLLogicalCone& expected, const LogicCone& actual) {
  ASSERT_EQ(expected.getNodeCount(), actual.getNodeCount());
  EXPECT_EQ(
      expected.getNodes()[expected.getRoot()].occurrence.getObject(),
      actual.getNodes()[actual.getRoot()].occurrence.getObject());
  EXPECT_EQ(collectNodeKinds(expected), collectNodeKinds(actual));
  EXPECT_EQ(collectLeaves(expected), collectLeaves(actual));
  EXPECT_EQ(collectEdges(expected), collectEdges(actual));
}

}  // namespace

TEST_F(MetricsTests, logicConeMatchesSNLLogicalCone) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  NLLibrary* primitives =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));

  auto flop = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("DFF"));
  auto d = SNLScalarTerm::create(
      flop, SNLTerm::Direction::Input, NLName("D"));
  auto clock = SNLScalarTerm::create(
      flop, SNLTerm::Direction::Input, NLName("CK"));
  auto q = SNLScalarTerm::create(
      flop, SNLTerm::Direction::Output, NLName("Q"));
  SNLDesignModeling::addInputsToClockArcs({d}, clock);
  SNLDesignModeling::addClockToOutputsArcs(clock, {q});

  auto gate = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("AND2"));
  auto i0 = SNLScalarTerm::create(
      gate, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(
      gate, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(
      gate, SNLTerm::Direction::Output, NLName("O"));
  SNLDesignModeling::addCombinatorialArcs({i0, i1}, {o});

  auto top = SNLDesign::create(library, NLName("TOP"));
  univ->setTopDesign(top);
  auto topInput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Input, NLName("IN"));
  auto topOutput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Output, NLName("OUT"));
  auto upstream = SNLInstance::create(top, flop, NLName("upstream"));
  auto combinatorial = SNLInstance::create(top, gate, NLName("gate"));
  auto downstream = SNLInstance::create(top, flop, NLName("downstream"));

  auto qNet = SNLScalarNet::create(top, NLName("q"));
  upstream->getInstTerm(q)->setNet(qNet);
  combinatorial->getInstTerm(i0)->setNet(qNet);
  auto inputNet = SNLScalarNet::create(top, NLName("input"));
  topInput->setNet(inputNet);
  combinatorial->getInstTerm(i1)->setNet(inputNet);
  auto resultNet = SNLScalarNet::create(top, NLName("result"));
  combinatorial->getInstTerm(o)->setNet(resultNet);
  downstream->getInstTerm(d)->setNet(resultNet);
  topOutput->setNet(resultNet);

  SNLLogicalCone snlFanIn(
      SNLOccurrence(downstream->getInstTerm(d)),
      SNLLogicalCone::Direction::FanIn);
  LogicCone dnlFanIn(
      SNLOccurrence(downstream->getInstTerm(d)),
      LogicCone::Direction::FanIn);
  expectSameCone(snlFanIn, dnlFanIn);

  SNLLogicalCone snlFanOut(
      SNLOccurrence(upstream->getInstTerm(q)),
      SNLLogicalCone::Direction::FanOut);
  LogicCone dnlFanOut(
      SNLOccurrence(upstream->getInstTerm(q)),
      LogicCone::Direction::FanOut);
  expectSameCone(snlFanOut, dnlFanOut);
}

TEST_F(MetricsTests, logicConeBusOccurrenceBuildsSharedCone) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  NLLibrary* primitives =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));

  auto buffer = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("BUF"));
  auto i = SNLScalarTerm::create(
      buffer, SNLTerm::Direction::Input, NLName("I"));
  auto o = SNLScalarTerm::create(
      buffer, SNLTerm::Direction::Output, NLName("O"));
  SNLDesignModeling::addCombinatorialArcs({i}, {o});

  auto top = SNLDesign::create(library, NLName("TOP"));
  univ->setTopDesign(top);
  auto topInput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Input, NLName("IN"));
  auto topOutput = SNLBusTerm::create(
      top, SNLTerm::Direction::Output, 1, 0, NLName("OUT"));
  auto instance = SNLInstance::create(top, buffer, NLName("buffer"));

  auto inputNet = SNLScalarNet::create(top, NLName("input"));
  topInput->setNet(inputNet);
  instance->getInstTerm(i)->setNet(inputNet);

  auto outputNet = SNLScalarNet::create(top, NLName("output"));
  instance->getInstTerm(o)->setNet(outputNet);
  topOutput->getBit(0)->setNet(outputNet);
  topOutput->getBit(1)->setNet(outputNet);

  LogicCone cone(SNLOccurrence(topOutput), LogicCone::Direction::FanIn);
  auto bit0 = topOutput->getBit(0);
  auto bit1 = topOutput->getBit(1);

  ASSERT_EQ(5, cone.getNodeCount());
  EXPECT_EQ(topOutput, cone.getNodes()[cone.getRoot()].occurrence.getObject());
  EXPECT_EQ(LogicCone::NodeKind::Root, cone.getNodes()[cone.getRoot()].kind);

  std::map<SNLDesignObject*, LogicCone::NodeKind> expectedKinds {
      {topOutput, LogicCone::NodeKind::Root},
      {bit0, LogicCone::NodeKind::Root},
      {bit1, LogicCone::NodeKind::Root},
      {instance, LogicCone::NodeKind::Internal},
      {topInput, LogicCone::NodeKind::Ports},
  };
  EXPECT_EQ(expectedKinds, collectNodeKinds(cone));

  std::set<SNLDesignObject*> expectedLeaves {topInput};
  EXPECT_EQ(expectedLeaves, collectLeaves(cone));

  std::set<std::pair<SNLDesignObject*, SNLDesignObject*>> expectedEdges {
      {topOutput, bit0},
      {topOutput, bit1},
      {bit0, instance},
      {bit1, instance},
      {instance, topInput},
  };
  EXPECT_EQ(expectedEdges, collectEdges(cone));
}

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

TEST_F(MetricsTests, logicLevelDB0OrGateTraversalStress) {
  constexpr size_t gateCount = 128;

  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("MYLIB"));
  SNLDesign* top = SNLDesign::create(library, NLName("top"));
  univ->setTopDesign(top);

  SNLDesign* or2 = NLDB0::getOrCreateNInputGate(NLDB0::GateType::Or, 2);
  ASSERT_NE(nullptr, or2);
  auto orOut = NLDB0::getGateSingleTerm(or2);
  ASSERT_NE(nullptr, orOut);
  auto orInputs = NLDB0::getGateNTerms(or2);
  ASSERT_NE(nullptr, orInputs);

  std::vector<SNLBitTerm*> inputBits;
  for (auto bit: orInputs->getBits()) {
    inputBits.push_back(bit);
  }
  ASSERT_EQ(2, inputBits.size());

  for (size_t index = 0; index < gateCount; ++index) {
    auto inputNet0 =
        SNLScalarNet::create(top, NLName("input_net0_" + std::to_string(index)));
    auto inputNet1 =
        SNLScalarNet::create(top, NLName("input_net1_" + std::to_string(index)));
    auto outputNet =
        SNLScalarNet::create(top, NLName("output_net_" + std::to_string(index)));

    auto topIn0 = SNLScalarTerm::create(
        top, SNLTerm::Direction::Input,
        NLName("input0_" + std::to_string(index)));
    auto topIn1 = SNLScalarTerm::create(
        top, SNLTerm::Direction::Input,
        NLName("input1_" + std::to_string(index)));
    auto topOut = SNLScalarTerm::create(
        top, SNLTerm::Direction::Output,
        NLName("output_" + std::to_string(index)));

    topIn0->setNet(inputNet0);
    topIn1->setNet(inputNet1);
    topOut->setNet(outputNet);

    auto instance = SNLInstance::create(
        top, or2, NLName("or2_" + std::to_string(index)));
    instance->getInstTerm(inputBits[0])->setNet(inputNet0);
    instance->getInstTerm(inputBits[1])->setNet(inputNet1);
    instance->getInstTerm(orOut)->setNet(outputNet);
  }

  LogicLevelComputer llc;
  llc.process();
  EXPECT_EQ(1, llc.getMaxLogicLevel());
  EXPECT_GE(llc.getMaxLogicLevelPaths().size(), 1);
}
