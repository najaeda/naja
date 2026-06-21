// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <algorithm>
#include <set>

#include "NLDB.h"
#include "NLException.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLBusTerm.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLLogicalCone.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"

using namespace naja::NL;

namespace {

const SNLLogicalCone::Node* findNode(
  const SNLLogicalCone& cone,
  SNLDesignObject* object) {
  auto found = std::find_if(
    cone.getNodes().begin(),
    cone.getNodes().end(),
    [object](const auto& node) {
      return node.occurrence.getObject() == object;
    });
  return found == cone.getNodes().end() ? nullptr : &*found;
}

SNLDesign* createBuffer(
  NLLibrary* primitives,
  const NLName& name,
  SNLScalarTerm*& input,
  SNLScalarTerm*& output) {
  auto design = SNLDesign::create(
    primitives, SNLDesign::Type::Primitive, name);
  input = SNLScalarTerm::create(
    design, SNLTerm::Direction::Input, NLName("I"));
  output = SNLScalarTerm::create(
    design, SNLTerm::Direction::Output, NLName("O"));
  SNLDesignModeling::addCombinatorialArcs({input}, {output});
  return design;
}

}  // namespace

class SNLLogicalConeTest0: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      db_ = NLDB::create(universe);
      library_ = NLLibrary::create(db_, NLName("designs"));
      primitives_ = NLLibrary::create(
        db_, NLLibrary::Type::Primitives, NLName("primitives"));
    }

    void TearDown() override {
      NLUniverse::get()->destroy();
    }

    NLDB* db_ {nullptr};
    NLLibrary* library_ {nullptr};
    NLLibrary* primitives_ {nullptr};
};

TEST_F(SNLLogicalConeTest0, SequentialAndPortFrontiers) {
  auto flop = SNLDesign::create(
    primitives_, SNLDesign::Type::Primitive, NLName("DFF"));
  auto d = SNLScalarTerm::create(
    flop, SNLTerm::Direction::Input, NLName("D"));
  auto clock = SNLScalarTerm::create(
    flop, SNLTerm::Direction::Input, NLName("CK"));
  auto q = SNLScalarTerm::create(
    flop, SNLTerm::Direction::Output, NLName("Q"));
  SNLDesignModeling::addInputsToClockArcs({d}, clock);
  SNLDesignModeling::addClockToOutputsArcs(clock, {q});

  auto gate = SNLDesign::create(
    primitives_, SNLDesign::Type::Primitive, NLName("AND2"));
  auto i0 = SNLScalarTerm::create(
    gate, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(
    gate, SNLTerm::Direction::Input, NLName("I1"));
  auto o = SNLScalarTerm::create(
    gate, SNLTerm::Direction::Output, NLName("O"));
  SNLDesignModeling::addCombinatorialArcs({i0, i1}, {o});

  auto top = SNLDesign::create(library_, NLName("TOP"));
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

  auto downstreamD = downstream->getInstTerm(d);
  SNLLogicalCone fanIn(
    SNLOccurrence(downstreamD), SNLLogicalCone::Direction::FanIn);
  ASSERT_EQ(4, fanIn.getNodeCount());
  ASSERT_EQ(2, fanIn.getLeaves().size());
  EXPECT_EQ(
    SNLLogicalCone::NodeKind::Root,
    fanIn.getNodes()[fanIn.getRoot()].kind);

  auto gateNode = findNode(fanIn, combinatorial);
  auto upstreamNode = findNode(fanIn, upstream);
  auto inputNode = findNode(fanIn, topInput);
  ASSERT_NE(nullptr, gateNode);
  ASSERT_NE(nullptr, upstreamNode);
  ASSERT_NE(nullptr, inputNode);
  EXPECT_EQ(SNLLogicalCone::NodeKind::Internal, gateNode->kind);
  EXPECT_EQ(SNLLogicalCone::NodeKind::Flop, upstreamNode->kind);
  EXPECT_EQ(SNLLogicalCone::NodeKind::Ports, inputNode->kind);
  EXPECT_EQ(2, gateNode->next.size());
  EXPECT_EQ(1, upstreamNode->prev.size());
  EXPECT_EQ(1, inputNode->prev.size());

  SNLLogicalCone fanOut(
    SNLOccurrence(upstream->getInstTerm(q)),
    SNLLogicalCone::Direction::FanOut);
  ASSERT_EQ(4, fanOut.getNodeCount());
  ASSERT_EQ(2, fanOut.getLeaves().size());
  auto downstreamNode = findNode(fanOut, downstream);
  auto outputNode = findNode(fanOut, topOutput);
  ASSERT_NE(nullptr, downstreamNode);
  ASSERT_NE(nullptr, outputNode);
  EXPECT_EQ(SNLLogicalCone::NodeKind::Flop, downstreamNode->kind);
  EXPECT_EQ(SNLLogicalCone::NodeKind::Ports, outputNode->kind);
}

TEST_F(SNLLogicalConeTest0, MultiOutputDependenciesAndBlackboxBarrier) {
  auto multi = SNLDesign::create(
    primitives_, SNLDesign::Type::Primitive, NLName("MULTI"));
  auto i0 = SNLScalarTerm::create(
    multi, SNLTerm::Direction::Input, NLName("I0"));
  auto i1 = SNLScalarTerm::create(
    multi, SNLTerm::Direction::Input, NLName("I1"));
  auto o0 = SNLScalarTerm::create(
    multi, SNLTerm::Direction::Output, NLName("O0"));
  auto o1 = SNLScalarTerm::create(
    multi, SNLTerm::Direction::Output, NLName("O1"));
  SNLDesignModeling::addCombinatorialArcs({i0}, {o0});
  SNLDesignModeling::addCombinatorialArcs({i1}, {o1});

  auto blackbox = SNLDesign::create(
    primitives_, SNLDesign::Type::Primitive, NLName("BLACKBOX"));
  auto blackboxInput = SNLScalarTerm::create(
    blackbox, SNLTerm::Direction::Input, NLName("I"));
  auto blackboxOutput = SNLScalarTerm::create(
    blackbox, SNLTerm::Direction::Output, NLName("O"));

  auto top = SNLDesign::create(library_, NLName("TOP"));
  auto input0 = SNLScalarTerm::create(
    top, SNLTerm::Direction::Input, NLName("I0"));
  auto input1 = SNLScalarTerm::create(
    top, SNLTerm::Direction::Input, NLName("I1"));
  auto output0 = SNLScalarTerm::create(
    top, SNLTerm::Direction::Output, NLName("O0"));
  auto output1 = SNLScalarTerm::create(
    top, SNLTerm::Direction::Output, NLName("O1"));
  auto multiInstance = SNLInstance::create(top, multi, NLName("multi"));
  auto blackboxInstance =
    SNLInstance::create(top, blackbox, NLName("blackbox"));

  auto input0Net = SNLScalarNet::create(top);
  input0->setNet(input0Net);
  multiInstance->getInstTerm(i0)->setNet(input0Net);
  auto input1Net = SNLScalarNet::create(top);
  input1->setNet(input1Net);
  multiInstance->getInstTerm(i1)->setNet(input1Net);
  auto output0Net = SNLScalarNet::create(top);
  multiInstance->getInstTerm(o0)->setNet(output0Net);
  output0->setNet(output0Net);

  auto blackboxInputNet = SNLScalarNet::create(top);
  input1->setNet(blackboxInputNet);
  blackboxInstance->getInstTerm(blackboxInput)->setNet(blackboxInputNet);
  auto blackboxOutputNet = SNLScalarNet::create(top);
  blackboxInstance->getInstTerm(blackboxOutput)->setNet(blackboxOutputNet);
  output1->setNet(blackboxOutputNet);

  SNLLogicalCone precise(
    SNLOccurrence(output0), SNLLogicalCone::Direction::FanIn);
  EXPECT_NE(nullptr, findNode(precise, input0));
  EXPECT_EQ(nullptr, findNode(precise, input1));

  SNLLogicalCone opaque(
    SNLOccurrence(output1), SNLLogicalCone::Direction::FanIn);
  ASSERT_EQ(2, opaque.getNodeCount());
  ASSERT_EQ(1, opaque.getLeaves().size());
  auto blackboxNode = findNode(opaque, blackboxInstance);
  ASSERT_NE(nullptr, blackboxNode);
  EXPECT_EQ(SNLLogicalCone::NodeKind::Blackbox, blackboxNode->kind);
  EXPECT_TRUE(blackboxNode->next.empty());
}

TEST_F(SNLLogicalConeTest0, ReconvergentLogicIsShared) {
  SNLScalarTerm* bufferInput = nullptr;
  SNLScalarTerm* bufferOutput = nullptr;
  auto buffer = createBuffer(
    primitives_, NLName("BUF"), bufferInput, bufferOutput);

  auto join = SNLDesign::create(
    primitives_, SNLDesign::Type::Primitive, NLName("JOIN"));
  auto join0 = SNLScalarTerm::create(
    join, SNLTerm::Direction::Input, NLName("I0"));
  auto join1 = SNLScalarTerm::create(
    join, SNLTerm::Direction::Input, NLName("I1"));
  auto joinOutput = SNLScalarTerm::create(
    join, SNLTerm::Direction::Output, NLName("O"));
  SNLDesignModeling::addCombinatorialArcs({join0, join1}, {joinOutput});

  auto top = SNLDesign::create(library_, NLName("TOP"));
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
  auto branch0Net = SNLScalarNet::create(top);
  branch0->getInstTerm(bufferOutput)->setNet(branch0Net);
  joinInstance->getInstTerm(join0)->setNet(branch0Net);
  auto branch1Net = SNLScalarNet::create(top);
  branch1->getInstTerm(bufferOutput)->setNet(branch1Net);
  joinInstance->getInstTerm(join1)->setNet(branch1Net);
  auto outputNet = SNLScalarNet::create(top);
  joinInstance->getInstTerm(joinOutput)->setNet(outputNet);
  output->setNet(outputNet);

  SNLLogicalCone cone(
    SNLOccurrence(output), SNLLogicalCone::Direction::FanIn);
  ASSERT_EQ(6, cone.getNodeCount());
  auto sourceNode = findNode(cone, source);
  ASSERT_NE(nullptr, sourceNode);
  EXPECT_EQ(2, sourceNode->prev.size());
  EXPECT_EQ(1, sourceNode->next.size());
  EXPECT_EQ(1, cone.getLeaves().size());
  EXPECT_EQ(input, cone.getNodes()[cone.getLeaves()[0]].occurrence.getObject());
}

TEST_F(SNLLogicalConeTest0, DuplicateAndCyclicEdgesAreIgnored) {
  auto multiOutput = SNLDesign::create(
    primitives_, SNLDesign::Type::Primitive, NLName("MULTI_OUTPUT"));
  auto multiInput = SNLScalarTerm::create(
    multiOutput, SNLTerm::Direction::Input, NLName("I"));
  auto multiOutput0 = SNLScalarTerm::create(
    multiOutput, SNLTerm::Direction::Output, NLName("O0"));
  auto multiOutput1 = SNLScalarTerm::create(
    multiOutput, SNLTerm::Direction::Output, NLName("O1"));
  SNLDesignModeling::addCombinatorialArcs(
    {multiInput}, {multiOutput0, multiOutput1});

  SNLScalarTerm* bufferInput = nullptr;
  SNLScalarTerm* bufferOutput = nullptr;
  auto buffer = createBuffer(
    primitives_, NLName("LOOP_BUFFER"), bufferInput, bufferOutput);

  auto top = SNLDesign::create(library_, NLName("TOP"));
  auto input = SNLScalarTerm::create(
    top, SNLTerm::Direction::Input, NLName("IN"));
  auto duplicateOutput = SNLScalarTerm::create(
    top, SNLTerm::Direction::Output, NLName("DUPLICATE_OUT"));
  auto loopOutput = SNLScalarTerm::create(
    top, SNLTerm::Direction::Output, NLName("LOOP_OUT"));
  auto multiInstance =
    SNLInstance::create(top, multiOutput, NLName("multi"));
  auto loopInstance =
    SNLInstance::create(top, buffer, NLName("loop"));

  auto inputNet = SNLScalarNet::create(top);
  input->setNet(inputNet);
  multiInstance->getInstTerm(multiInput)->setNet(inputNet);
  auto duplicateNet = SNLScalarNet::create(top);
  multiInstance->getInstTerm(multiOutput0)->setNet(duplicateNet);
  multiInstance->getInstTerm(multiOutput1)->setNet(duplicateNet);
  duplicateOutput->setNet(duplicateNet);

  SNLLogicalCone duplicateCone(
    SNLOccurrence(duplicateOutput), SNLLogicalCone::Direction::FanIn);
  ASSERT_EQ(3, duplicateCone.getNodeCount());
  auto multiNode = findNode(duplicateCone, multiInstance);
  ASSERT_NE(nullptr, multiNode);
  EXPECT_EQ(1, duplicateCone.getNodes()[duplicateCone.getRoot()].next.size());

  auto loopNet = SNLScalarNet::create(top);
  loopInstance->getInstTerm(bufferInput)->setNet(loopNet);
  loopInstance->getInstTerm(bufferOutput)->setNet(loopNet);
  loopOutput->setNet(loopNet);

  SNLLogicalCone loopCone(
    SNLOccurrence(loopOutput), SNLLogicalCone::Direction::FanIn);
  ASSERT_EQ(2, loopCone.getNodeCount());
  auto loopNode = findNode(loopCone, loopInstance);
  ASSERT_NE(nullptr, loopNode);
  EXPECT_TRUE(loopNode->next.empty());
}

TEST_F(SNLLogicalConeTest0, RejectsWholeBusRoot) {
  auto top = SNLDesign::create(library_, NLName("TOP"));
  auto bus = SNLBusTerm::create(
    top, SNLTerm::Direction::Input, 3, 0, NLName("BUS"));
  EXPECT_THROW(
    SNLLogicalCone(
      SNLOccurrence(bus), SNLLogicalCone::Direction::FanIn),
    NLException);
}
