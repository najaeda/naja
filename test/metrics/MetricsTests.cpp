// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"
#include "tbb/scalable_allocator.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>

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
#include "SNLInstance.h"
#include "SNLInstTerm.h"
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

const LogicCone::Node* findNode(
    const LogicCone& cone,
    SNLDesignObject* object) {
  auto found = std::find_if(
      cone.getNodes().begin(),
      cone.getNodes().end(),
      [object](const auto& node) {
        return node.occurrence.getObject() == object;
      });
  return found == cone.getNodes().end() ? nullptr : &*found;
}

void setEnvVar(const char* name, const std::string& value) {
#if defined(_WIN32)
  _putenv_s(name, value.c_str());
#else
  setenv(name, value.c_str(), 1);
#endif
}

void unsetEnvVar(const char* name) {
#if defined(_WIN32)
  _putenv_s(name, "");
#else
  unsetenv(name);
#endif
}

class ScopedEnvVar {
  public:
    explicit ScopedEnvVar(const char* name):
      name_(name) {
      if (const char* value = std::getenv(name)) {
        previous_ = value;
      }
    }

    ~ScopedEnvVar() {
      if (previous_) {
        setEnvVar(name_.c_str(), *previous_);
      } else {
        unsetEnvVar(name_.c_str());
      }
    }

    void set(const std::string& value) const {
      setEnvVar(name_.c_str(), value);
    }

  private:
    std::string name_;
    std::optional<std::string> previous_;
};

struct SharedBusConeDesign {
  SNLBusTerm* output {nullptr};
  SNLBusTermBit* bit0 {nullptr};
  SNLBusTermBit* bit1 {nullptr};
  SNLScalarTerm* input {nullptr};
  SNLInstance* instance {nullptr};
};

SharedBusConeDesign createSharedBusConeDesign(
    NLUniverse* univ,
    NLLibrary* library,
    NLLibrary* primitives) {
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

  return {
      topOutput,
      topOutput->getBit(0),
      topOutput->getBit(1),
      topInput,
      instance,
  };
}

}  // namespace

TEST_F(MetricsTests, logicConeSequentialAndPortFrontiers) {
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

  LogicCone fanIn(
      SNLOccurrence(downstream->getInstTerm(d)),
      LogicCone::Direction::FanIn);
  ASSERT_EQ(4, fanIn.getNodeCount());
  ASSERT_EQ(2, fanIn.getLeaves().size());
  EXPECT_EQ(
      LogicCone::NodeKind::Root,
      fanIn.getNodes()[fanIn.getRoot()].kind);
  auto gateNode = findNode(fanIn, combinatorial);
  auto upstreamNode = findNode(fanIn, upstream);
  auto inputNode = findNode(fanIn, topInput);
  ASSERT_NE(nullptr, gateNode);
  ASSERT_NE(nullptr, upstreamNode);
  ASSERT_NE(nullptr, inputNode);
  EXPECT_EQ(LogicCone::NodeKind::Internal, gateNode->kind);
  EXPECT_EQ(LogicCone::NodeKind::Flop, upstreamNode->kind);
  EXPECT_EQ(LogicCone::NodeKind::Ports, inputNode->kind);
  EXPECT_EQ(2, gateNode->next.size());
  EXPECT_EQ(1, upstreamNode->prev.size());
  EXPECT_EQ(1, inputNode->prev.size());

  LogicCone fanOut(
      SNLOccurrence(upstream->getInstTerm(q)),
      LogicCone::Direction::FanOut);
  ASSERT_EQ(4, fanOut.getNodeCount());
  ASSERT_EQ(2, fanOut.getLeaves().size());
  auto downstreamNode = findNode(fanOut, downstream);
  auto outputNode = findNode(fanOut, topOutput);
  ASSERT_NE(nullptr, downstreamNode);
  ASSERT_NE(nullptr, outputNode);
  EXPECT_EQ(LogicCone::NodeKind::Flop, downstreamNode->kind);
  EXPECT_EQ(LogicCone::NodeKind::Ports, outputNode->kind);
}

TEST_F(MetricsTests, logicConeBusOccurrenceBuildsSharedCone) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  NLLibrary* primitives =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));
  auto design = createSharedBusConeDesign(univ, library, primitives);

  LogicCone cone(SNLOccurrence(design.output), LogicCone::Direction::FanIn);

  ASSERT_EQ(5, cone.getNodeCount());
  EXPECT_EQ(
      design.output,
      cone.getNodes()[cone.getRoot()].occurrence.getObject());
  EXPECT_EQ(LogicCone::NodeKind::Root, cone.getNodes()[cone.getRoot()].kind);

  std::map<SNLDesignObject*, LogicCone::NodeKind> expectedKinds {
      {design.output, LogicCone::NodeKind::Root},
      {design.bit0, LogicCone::NodeKind::Root},
      {design.bit1, LogicCone::NodeKind::Root},
      {design.instance, LogicCone::NodeKind::Internal},
      {design.input, LogicCone::NodeKind::Ports},
  };
  EXPECT_EQ(expectedKinds, collectNodeKinds(cone));

  std::set<SNLDesignObject*> expectedLeaves {design.input};
  EXPECT_EQ(expectedLeaves, collectLeaves(cone));

  std::set<std::pair<SNLDesignObject*, SNLDesignObject*>> expectedEdges {
      {design.output, design.bit0},
      {design.output, design.bit1},
      {design.bit0, design.instance},
      {design.bit1, design.instance},
      {design.instance, design.input},
  };
  EXPECT_EQ(expectedEdges, collectEdges(cone));
}

TEST_F(MetricsTests, logicConeBusOccurrenceBuildsSharedConeNonMT) {
  const ScopedEnvVar nonMT("NON_MT");
  nonMT.set("1");

  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  NLLibrary* primitives =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));
  auto design = createSharedBusConeDesign(univ, library, primitives);

  LogicCone cone(SNLOccurrence(design.output), LogicCone::Direction::FanIn);

  EXPECT_EQ(5, cone.getNodeCount());
  EXPECT_EQ(
      design.output,
      cone.getNodes()[cone.getRoot()].occurrence.getObject());
  EXPECT_EQ(
      std::set<SNLDesignObject*>({design.input}),
      collectLeaves(cone));
}

TEST_F(MetricsTests, logicConeBusRootPromotionRemovesPortLeaf) {
  const ScopedEnvVar nonMT("NON_MT");
  nonMT.set("1");

  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  auto top = SNLDesign::create(library, NLName("TOP"));
  univ->setTopDesign(top);
  auto bus = SNLBusTerm::create(
      top, SNLTerm::Direction::InOut, 1, 0, NLName("BUS"));
  auto sharedNet = SNLScalarNet::create(top, NLName("shared"));
  bus->getBit(0)->setNet(sharedNet);
  bus->getBit(1)->setNet(sharedNet);

  LogicCone cone(SNLOccurrence(bus), LogicCone::Direction::FanIn);

  ASSERT_EQ(3, cone.getNodeCount());
  const std::map<SNLDesignObject*, LogicCone::NodeKind> expectedKinds {
      {bus, LogicCone::NodeKind::Root},
      {bus->getBit(0), LogicCone::NodeKind::Root},
      {bus->getBit(1), LogicCone::NodeKind::Root},
  };
  EXPECT_EQ(expectedKinds, collectNodeKinds(cone));
  EXPECT_TRUE(cone.getLeaves().empty());
}

TEST_F(MetricsTests, logicConeHandlesBlackboxLeaf) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  NLLibrary* primitives =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));

  auto blackbox = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("BB"));
  auto input = SNLScalarTerm::create(
      blackbox, SNLTerm::Direction::Input, NLName("I"));
  auto output = SNLScalarTerm::create(
      blackbox, SNLTerm::Direction::Output, NLName("O"));

  auto top = SNLDesign::create(library, NLName("TOP"));
  univ->setTopDesign(top);
  auto topInput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Input, NLName("IN"));
  auto topOutput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Output, NLName("OUT"));
  auto instance = SNLInstance::create(top, blackbox, NLName("blackbox"));

  auto inputNet = SNLScalarNet::create(top, NLName("input"));
  topInput->setNet(inputNet);
  instance->getInstTerm(input)->setNet(inputNet);
  auto outputNet = SNLScalarNet::create(top, NLName("output"));
  instance->getInstTerm(output)->setNet(outputNet);
  topOutput->setNet(outputNet);

  LogicCone cone(SNLOccurrence(topOutput), LogicCone::Direction::FanIn);

  ASSERT_EQ(2, cone.getNodeCount());
  const std::map<SNLDesignObject*, LogicCone::NodeKind> expectedKinds {
      {topOutput, LogicCone::NodeKind::Root},
      {instance, LogicCone::NodeKind::Blackbox},
  };
  EXPECT_EQ(expectedKinds, collectNodeKinds(cone));
  EXPECT_EQ(
      std::set<SNLDesignObject*>({instance}),
      collectLeaves(cone));
}

TEST_F(MetricsTests, logicConeUsesPreciseMultiOutputDependencies) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  NLLibrary* primitives =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));

  auto multi = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("MULTI"));
  auto input0 = SNLScalarTerm::create(
      multi, SNLTerm::Direction::Input, NLName("I0"));
  auto input1 = SNLScalarTerm::create(
      multi, SNLTerm::Direction::Input, NLName("I1"));
  auto output0 = SNLScalarTerm::create(
      multi, SNLTerm::Direction::Output, NLName("O0"));
  auto output1 = SNLScalarTerm::create(
      multi, SNLTerm::Direction::Output, NLName("O1"));
  SNLDesignModeling::addCombinatorialArcs({input0}, {output0});
  SNLDesignModeling::addCombinatorialArcs({input1}, {output1});

  auto top = SNLDesign::create(library, NLName("TOP"));
  univ->setTopDesign(top);
  auto topInput0 = SNLScalarTerm::create(
      top, SNLTerm::Direction::Input, NLName("I0"));
  auto topInput1 = SNLScalarTerm::create(
      top, SNLTerm::Direction::Input, NLName("I1"));
  auto topOutput0 = SNLScalarTerm::create(
      top, SNLTerm::Direction::Output, NLName("O0"));
  auto instance = SNLInstance::create(top, multi, NLName("multi"));

  auto inputNet0 = SNLScalarNet::create(top);
  topInput0->setNet(inputNet0);
  instance->getInstTerm(input0)->setNet(inputNet0);
  auto inputNet1 = SNLScalarNet::create(top);
  topInput1->setNet(inputNet1);
  instance->getInstTerm(input1)->setNet(inputNet1);
  auto outputNet0 = SNLScalarNet::create(top);
  instance->getInstTerm(output0)->setNet(outputNet0);
  topOutput0->setNet(outputNet0);

  LogicCone cone(SNLOccurrence(topOutput0), LogicCone::Direction::FanIn);

  EXPECT_NE(nullptr, findNode(cone, topInput0));
  EXPECT_EQ(nullptr, findNode(cone, topInput1));
}

TEST_F(MetricsTests, logicConeSharesReconvergentLogic) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  NLLibrary* primitives =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));

  auto buffer = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("BUF"));
  auto bufferInput = SNLScalarTerm::create(
      buffer, SNLTerm::Direction::Input, NLName("I"));
  auto bufferOutput = SNLScalarTerm::create(
      buffer, SNLTerm::Direction::Output, NLName("O"));
  SNLDesignModeling::addCombinatorialArcs({bufferInput}, {bufferOutput});

  auto join = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("JOIN"));
  auto join0 = SNLScalarTerm::create(
      join, SNLTerm::Direction::Input, NLName("I0"));
  auto join1 = SNLScalarTerm::create(
      join, SNLTerm::Direction::Input, NLName("I1"));
  auto joinOutput = SNLScalarTerm::create(
      join, SNLTerm::Direction::Output, NLName("O"));
  SNLDesignModeling::addCombinatorialArcs({join0, join1}, {joinOutput});

  auto top = SNLDesign::create(library, NLName("TOP"));
  univ->setTopDesign(top);
  auto input = SNLScalarTerm::create(
      top, SNLTerm::Direction::Input, NLName("IN"));
  auto output = SNLScalarTerm::create(
      top, SNLTerm::Direction::Output, NLName("OUT"));
  auto source = SNLInstance::create(top, buffer, NLName("source"));
  auto branch0 = SNLInstance::create(top, buffer, NLName("branch0"));
  auto branch1 = SNLInstance::create(top, buffer, NLName("branch1"));
  auto joinInstance = SNLInstance::create(top, join, NLName("join"));

  auto inputNet = SNLScalarNet::create(top);
  input->setNet(inputNet);
  source->getInstTerm(bufferInput)->setNet(inputNet);
  auto sourceNet = SNLScalarNet::create(top);
  source->getInstTerm(bufferOutput)->setNet(sourceNet);
  branch0->getInstTerm(bufferInput)->setNet(sourceNet);
  branch1->getInstTerm(bufferInput)->setNet(sourceNet);
  auto branchNet0 = SNLScalarNet::create(top);
  branch0->getInstTerm(bufferOutput)->setNet(branchNet0);
  joinInstance->getInstTerm(join0)->setNet(branchNet0);
  auto branchNet1 = SNLScalarNet::create(top);
  branch1->getInstTerm(bufferOutput)->setNet(branchNet1);
  joinInstance->getInstTerm(join1)->setNet(branchNet1);
  auto outputNet = SNLScalarNet::create(top);
  joinInstance->getInstTerm(joinOutput)->setNet(outputNet);
  output->setNet(outputNet);

  LogicCone cone(SNLOccurrence(output), LogicCone::Direction::FanIn);

  ASSERT_EQ(6, cone.getNodeCount());
  auto sourceNode = findNode(cone, source);
  ASSERT_NE(nullptr, sourceNode);
  EXPECT_EQ(2, sourceNode->prev.size());
  EXPECT_EQ(1, sourceNode->next.size());
  ASSERT_EQ(1, cone.getLeaves().size());
  EXPECT_EQ(input, cone.getNodes()[cone.getLeaves()[0]].occurrence.getObject());
}

TEST_F(MetricsTests, logicConeSuppressesDuplicateEdges) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  NLLibrary* primitives =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));

  auto multi = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("MULTI"));
  auto input = SNLScalarTerm::create(
      multi, SNLTerm::Direction::Input, NLName("I"));
  auto output0 = SNLScalarTerm::create(
      multi, SNLTerm::Direction::Output, NLName("O0"));
  auto output1 = SNLScalarTerm::create(
      multi, SNLTerm::Direction::Output, NLName("O1"));
  SNLDesignModeling::addCombinatorialArcs({input}, {output0, output1});

  auto top = SNLDesign::create(library, NLName("TOP"));
  univ->setTopDesign(top);
  auto topInput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Input, NLName("IN"));
  auto topOutput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Output, NLName("OUT"));
  auto instance = SNLInstance::create(top, multi, NLName("multi"));

  auto inputNet = SNLScalarNet::create(top, NLName("input"));
  topInput->setNet(inputNet);
  instance->getInstTerm(input)->setNet(inputNet);
  auto outputNet = SNLScalarNet::create(top, NLName("output"));
  instance->getInstTerm(output0)->setNet(outputNet);
  instance->getInstTerm(output1)->setNet(outputNet);
  topOutput->setNet(outputNet);

  LogicCone cone(SNLOccurrence(topOutput), LogicCone::Direction::FanIn);
  const auto& nodes = cone.getNodes();

  ASSERT_EQ(3, cone.getNodeCount());
  ASSERT_EQ(1, nodes[cone.getRoot()].next.size());
  auto instanceNode = nodes[cone.getRoot()].next.front();
  EXPECT_EQ(instance, nodes[instanceNode].occurrence.getObject());
  EXPECT_EQ(1, nodes[instanceNode].next.size());
  EXPECT_EQ(
      topInput,
      nodes[nodes[instanceNode].next.front()].occurrence.getObject());
}

TEST_F(MetricsTests, logicConeAvoidsCombinationalCycles) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  NLLibrary* primitives =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));

  auto buffer = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("BUF"));
  auto input = SNLScalarTerm::create(
      buffer, SNLTerm::Direction::Input, NLName("I"));
  auto output = SNLScalarTerm::create(
      buffer, SNLTerm::Direction::Output, NLName("O"));
  SNLDesignModeling::addCombinatorialArcs({input}, {output});

  auto top = SNLDesign::create(library, NLName("TOP"));
  univ->setTopDesign(top);
  auto topOutput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Output, NLName("OUT"));
  auto instance = SNLInstance::create(top, buffer, NLName("loop"));

  auto loopNet = SNLScalarNet::create(top, NLName("loop"));
  instance->getInstTerm(input)->setNet(loopNet);
  instance->getInstTerm(output)->setNet(loopNet);
  topOutput->setNet(loopNet);

  LogicCone cone(SNLOccurrence(topOutput), LogicCone::Direction::FanIn);
  const auto& nodes = cone.getNodes();

  ASSERT_EQ(2, cone.getNodeCount());
  auto instanceNode = nodes[cone.getRoot()].next.front();
  EXPECT_EQ(instance, nodes[instanceNode].occurrence.getObject());
  EXPECT_TRUE(nodes[instanceNode].next.empty());
}

TEST_F(MetricsTests, logicConeAvoidsIndirectCombinationalCycles) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  NLLibrary* primitives =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));

  auto buffer = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("BUF"));
  auto input = SNLScalarTerm::create(
      buffer, SNLTerm::Direction::Input, NLName("I"));
  auto output = SNLScalarTerm::create(
      buffer, SNLTerm::Direction::Output, NLName("O"));
  SNLDesignModeling::addCombinatorialArcs({input}, {output});

  auto top = SNLDesign::create(library, NLName("TOP"));
  univ->setTopDesign(top);
  auto topOutput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Output, NLName("OUT"));
  auto first = SNLInstance::create(top, buffer, NLName("first"));
  auto second = SNLInstance::create(top, buffer, NLName("second"));

  auto firstOutputNet = SNLScalarNet::create(top, NLName("first_output"));
  first->getInstTerm(output)->setNet(firstOutputNet);
  second->getInstTerm(input)->setNet(firstOutputNet);
  topOutput->setNet(firstOutputNet);

  auto secondOutputNet = SNLScalarNet::create(top, NLName("second_output"));
  second->getInstTerm(output)->setNet(secondOutputNet);
  first->getInstTerm(input)->setNet(secondOutputNet);

  LogicCone cone(SNLOccurrence(topOutput), LogicCone::Direction::FanIn);
  const auto& nodes = cone.getNodes();

  ASSERT_EQ(3, cone.getNodeCount());
  auto firstNode = nodes[cone.getRoot()].next.front();
  ASSERT_EQ(first, nodes[firstNode].occurrence.getObject());
  ASSERT_EQ(1, nodes[firstNode].next.size());
  auto secondNode = nodes[firstNode].next.front();
  EXPECT_EQ(second, nodes[secondNode].occurrence.getObject());
  EXPECT_TRUE(nodes[secondNode].next.empty());
}

TEST_F(MetricsTests, logicConeHandlesUnconnectedRoot) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  auto top = SNLDesign::create(library, NLName("TOP"));
  univ->setTopDesign(top);
  auto output = SNLScalarTerm::create(
      top, SNLTerm::Direction::Output, NLName("OUT"));

  LogicCone cone(SNLOccurrence(output), LogicCone::Direction::FanIn);

  ASSERT_EQ(1, cone.getNodeCount());
  EXPECT_EQ(output, cone.getNodes()[cone.getRoot()].occurrence.getObject());
  EXPECT_TRUE(cone.getLeaves().empty());
}

TEST_F(MetricsTests, logicConeRejectsInvalidRoot) {
  NLUniverse::create();
  EXPECT_THROW(
      LogicCone(SNLOccurrence(), LogicCone::Direction::FanIn),
      NLException);
}

TEST_F(MetricsTests, logicConeRejectsNetRoot) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  auto top = SNLDesign::create(library, NLName("TOP"));
  univ->setTopDesign(top);
  auto net = SNLScalarNet::create(top, NLName("net"));

  EXPECT_THROW(
      LogicCone(SNLOccurrence(net), LogicCone::Direction::FanIn),
      NLException);
}

TEST_F(MetricsTests, logicConeHandlesHierarchicalBitTermRoot) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  NLLibrary* primitives =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));

  auto buffer = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("BUF"));
  auto bufferInput = SNLScalarTerm::create(
      buffer, SNLTerm::Direction::Input, NLName("I"));
  auto bufferOutput = SNLScalarTerm::create(
      buffer, SNLTerm::Direction::Output, NLName("O"));
  SNLDesignModeling::addCombinatorialArcs({bufferInput}, {bufferOutput});

  auto child = SNLDesign::create(library, NLName("CHILD"));
  auto childInput = SNLScalarTerm::create(
      child, SNLTerm::Direction::Input, NLName("I"));
  auto childOutput = SNLScalarTerm::create(
      child, SNLTerm::Direction::Output, NLName("O"));
  auto childBuffer = SNLInstance::create(child, buffer, NLName("buffer"));
  auto childInputNet = SNLScalarNet::create(child, NLName("child_input"));
  childInput->setNet(childInputNet);
  childBuffer->getInstTerm(bufferInput)->setNet(childInputNet);
  auto childOutputNet = SNLScalarNet::create(child, NLName("child_output"));
  childBuffer->getInstTerm(bufferOutput)->setNet(childOutputNet);
  childOutput->setNet(childOutputNet);

  auto top = SNLDesign::create(library, NLName("TOP"));
  univ->setTopDesign(top);
  auto topInput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Input, NLName("IN"));
  auto topOutput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Output, NLName("OUT"));
  auto childInstance = SNLInstance::create(top, child, NLName("child"));
  auto topInputNet = SNLScalarNet::create(top, NLName("top_input"));
  topInput->setNet(topInputNet);
  childInstance->getInstTerm(childInput)->setNet(topInputNet);
  auto topOutputNet = SNLScalarNet::create(top, NLName("top_output"));
  childInstance->getInstTerm(childOutput)->setNet(topOutputNet);
  topOutput->setNet(topOutputNet);

  auto start = SNLOccurrence(SNLPath(childInstance), childOutput);
  LogicCone cone(start, LogicCone::Direction::FanIn);

  ASSERT_EQ(3, cone.getNodeCount());
  EXPECT_EQ(start, cone.getNodes()[cone.getRoot()].occurrence);
  EXPECT_NE(nullptr, findNode(cone, childBuffer));
  EXPECT_NE(nullptr, findNode(cone, topInput));
}

TEST_F(MetricsTests, logicConeHandlesPartialBitTermRoots) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  NLLibrary* primitives =
      NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));

  auto buffer = SNLDesign::create(
      primitives, SNLDesign::Type::Primitive, NLName("BUF"));
  auto bufferInput = SNLScalarTerm::create(
      buffer, SNLTerm::Direction::Input, NLName("I"));
  auto bufferOutput = SNLScalarTerm::create(
      buffer, SNLTerm::Direction::Output, NLName("O"));
  SNLDesignModeling::addCombinatorialArcs({bufferInput}, {bufferOutput});

  auto child = SNLDesign::create(library, NLName("CHILD"));
  auto childInput = SNLScalarTerm::create(
      child, SNLTerm::Direction::Input, NLName("I"));
  auto childOutput = SNLScalarTerm::create(
      child, SNLTerm::Direction::Output, NLName("O"));
  auto childBuffer = SNLInstance::create(child, buffer, NLName("buffer"));
  auto childInputNet = SNLScalarNet::create(child, NLName("child_input"));
  childInput->setNet(childInputNet);
  childBuffer->getInstTerm(bufferInput)->setNet(childInputNet);
  auto childOutputNet = SNLScalarNet::create(child, NLName("child_output"));
  childBuffer->getInstTerm(bufferOutput)->setNet(childOutputNet);
  childOutput->setNet(childOutputNet);

  auto middle = SNLDesign::create(library, NLName("MIDDLE"));
  auto middleInput = SNLScalarTerm::create(
      middle, SNLTerm::Direction::Input, NLName("I"));
  auto middleOutput = SNLScalarTerm::create(
      middle, SNLTerm::Direction::Output, NLName("O"));
  auto childInstance = SNLInstance::create(middle, child, NLName("child"));
  auto middleInputNet = SNLScalarNet::create(middle, NLName("middle_input"));
  middleInput->setNet(middleInputNet);
  childInstance->getInstTerm(childInput)->setNet(middleInputNet);
  auto middleOutputNet = SNLScalarNet::create(middle, NLName("middle_output"));
  childInstance->getInstTerm(childOutput)->setNet(middleOutputNet);
  middleOutput->setNet(middleOutputNet);

  auto top = SNLDesign::create(library, NLName("TOP"));
  univ->setTopDesign(top);
  auto topInput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Input, NLName("IN"));
  auto topOutput = SNLScalarTerm::create(
      top, SNLTerm::Direction::Output, NLName("OUT"));
  auto middleInstance = SNLInstance::create(top, middle, NLName("middle"));
  auto topInputNet = SNLScalarNet::create(top, NLName("top_input"));
  topInput->setNet(topInputNet);
  middleInstance->getInstTerm(middleInput)->setNet(topInputNet);
  auto topOutputNet = SNLScalarNet::create(top, NLName("top_output"));
  middleInstance->getInstTerm(middleOutput)->setNet(topOutputNet);
  topOutput->setNet(topOutputNet);

  auto localStart = SNLOccurrence(middleOutput);
  LogicCone localCone(localStart, LogicCone::Direction::FanIn);
  ASSERT_EQ(3, localCone.getNodeCount());
  EXPECT_EQ(localStart, localCone.getNodes()[localCone.getRoot()].occurrence);
  EXPECT_NE(nullptr, findNode(localCone, childBuffer));
  EXPECT_NE(nullptr, findNode(localCone, middleInput));

  auto partialPathStart = SNLOccurrence(SNLPath(childInstance), childOutput);
  LogicCone partialCone(partialPathStart, LogicCone::Direction::FanIn);
  ASSERT_EQ(3, partialCone.getNodeCount());
  EXPECT_EQ(
      partialPathStart,
      partialCone.getNodes()[partialCone.getRoot()].occurrence);
  EXPECT_NE(nullptr, findNode(partialCone, childBuffer));
  EXPECT_NE(nullptr, findNode(partialCone, middleInput));
}

TEST_F(MetricsTests, logicConeInfersAndSetsTopDesign) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  auto top = SNLDesign::create(library, NLName("TOP"));
  auto otherTop = SNLDesign::create(library, NLName("OTHER_TOP"));
  auto repeatedModel = SNLDesign::create(library, NLName("REPEATED"));
  SNLInstance::create(otherTop, repeatedModel, NLName("first"));
  SNLInstance::create(otherTop, repeatedModel, NLName("second"));
  auto output = SNLScalarTerm::create(
      top, SNLTerm::Direction::Output, NLName("OUT"));
  univ->setTopDesign(otherTop);

  LogicCone cone(SNLOccurrence(output), LogicCone::Direction::FanIn);

  EXPECT_EQ(top, univ->getTopDesign());
  EXPECT_EQ(output, cone.getNodes()[cone.getRoot()].occurrence.getObject());
}

TEST_F(MetricsTests, logicConeInfersTopDesignWhenUniverseTopUnset) {
  NLUniverse* univ = NLUniverse::create();
  NLDB* db = NLDB::create(univ);
  NLLibrary* library = NLLibrary::create(db, NLName("designs"));
  auto top = SNLDesign::create(library, NLName("TOP"));
  auto output = SNLScalarTerm::create(
      top, SNLTerm::Direction::Output, NLName("OUT"));

  LogicCone cone(SNLOccurrence(output), LogicCone::Direction::FanIn);

  EXPECT_EQ(top, univ->getTopDesign());
  EXPECT_EQ(output, cone.getNodes()[cone.getRoot()].occurrence.getObject());
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
