// SPDX-FileCopyrightText: 2026 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include <stdexcept>

#include "NLDB.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLDesign.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLScalarTerm.h"

#include "TClock.h"
#include "TClockPropagator.h"
#include "TEdge.h"
#include "TGraph.h"
#include "TLoopBreaker.h"
#include "TNode.h"

using namespace naja::NL;
using namespace naja::TG;

class TGraphTests: public ::testing::Test {
  protected:
    void SetUp() override {
      auto universe = NLUniverse::create();
      auto db = NLDB::create(universe);
      auto designsLib = NLLibrary::create(db);
      top_ = SNLDesign::create(designsLib, NLName("TOP"));
      auto model = SNLDesign::create(designsLib, NLName("MODEL"));
      modelClockTerm_ = SNLScalarTerm::create(model, SNLTerm::Direction::Input, NLName("clk"));
      topClockTerm_ = SNLScalarTerm::create(top_, SNLTerm::Direction::Input, NLName("clk"));
      instance_ = SNLInstance::create(top_, model, NLName("instance"));
    }
    void TearDown() override {
      NLUniverse::get()->destroy();
    }

    SNLDesign*      top_             {nullptr};
    SNLInstance*    instance_        {nullptr};
    SNLScalarTerm*  topClockTerm_    {nullptr};
    SNLScalarTerm*  modelClockTerm_  {nullptr};
};

TEST_F(TGraphTests, testNodesAndEdges) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  EXPECT_EQ(2u, graph.getNodesCount());
  auto e0 = graph.createEdge(n0, n1, nullptr);
  EXPECT_EQ(1u, graph.getEdgesCount());
  auto edge = graph.getEdge(e0);
  ASSERT_NE(nullptr, edge);
  EXPECT_EQ(n0, edge->getSource());
  EXPECT_EQ(n1, edge->getTarget());
}

TEST_F(TGraphTests, testClockOnTopAndInstanceTerminal) {
  TGraph graph;
  auto topNode = graph.createNode(nullptr);
  auto instNode = graph.createNode(nullptr);

  //Clock reaches the top terminal directly...
  auto topClock = graph.createClock("clk", topClockTerm_, topNode);
  //...and, through the instance, the instance terminal.
  auto instTerm = instance_->getInstTerm(modelClockTerm_);
  ASSERT_NE(nullptr, instTerm);
  auto instClock = graph.createClock("clk", instTerm, instNode);

  EXPECT_EQ(2u, graph.getClocksCount());
  EXPECT_EQ("clk", graph.getClock(topClock)->getName());
  EXPECT_EQ(topClockTerm_, graph.getClock(topClock)->getTarget());
  EXPECT_EQ(topNode, graph.getClock(topClock)->getNode());
  EXPECT_EQ(instTerm, graph.getClock(instClock)->getTarget());
  EXPECT_EQ(instNode, graph.getClock(instClock)->getNode());
  EXPECT_EQ(nullptr, graph.getClock(graph.getClocksCount()));
}

TEST_F(TGraphTests, testCreateClockRejectsInvalidNode) {
  TGraph graph;
  EXPECT_THROW(graph.createClock("clk", nullptr, 0), std::invalid_argument);
}

TEST_F(TGraphTests, testEdgeClockMasksDetectCrossing) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  auto e0 = graph.createEdge(n0, n1, nullptr);
  auto edge = graph.getEdge(e0);
  //No clocks registered yet: masks are empty.
  EXPECT_EQ(0u, edge->getFromClocks().size());
  EXPECT_EQ(0u, edge->getToClocks().size());

  auto clkA = graph.createClock("clkA", topClockTerm_, n0);
  auto clkB = graph.createClock("clkB", nullptr, n1);

  //Registering clocks grows every existing edge's masks to match, all zero.
  EXPECT_EQ(2u, edge->getFromClocks().size());
  EXPECT_EQ(2u, edge->getToClocks().size());
  EXPECT_FALSE(edge->hasClockCrossing());

  //Same clock on both sides: no crossing.
  edge->addFromClock(clkA);
  edge->addToClock(clkA);
  EXPECT_TRUE(edge->hasFromClock(clkA));
  EXPECT_TRUE(edge->hasToClock(clkA));
  EXPECT_FALSE(edge->hasClockCrossing());

  //A different clock reaches the target side: crossing.
  edge->addToClock(clkB);
  EXPECT_TRUE(edge->hasClockCrossing());
  EXPECT_FALSE(edge->hasFromClock(clkB));
  EXPECT_TRUE(edge->hasToClock(clkB));
}

TEST_F(TGraphTests, testClockCountIsUnbounded) {
  TGraph graph;
  auto seedNode = graph.createNode(nullptr);
  //Register well past the old hand-rolled 64-bit mask's limit.
  constexpr std::size_t kClockCount = 200;
  for (std::size_t i = 0; i < kClockCount; ++i) {
    graph.createClock("clk" + std::to_string(i), nullptr, seedNode);
  }
  EXPECT_EQ(kClockCount, graph.getClocksCount());

  auto e0 = graph.createEdge(graph.createNode(nullptr), graph.createNode(nullptr), nullptr);
  auto edge = graph.getEdge(e0);
  TClock::ClockId highClock = kClockCount - 1;
  edge->addFromClock(highClock);
  EXPECT_GT(edge->getFromClocks().size(), 64u);
  EXPECT_TRUE(edge->hasFromClock(highClock));
}

TEST_F(TGraphTests, testAllEdgesShareTheSameClockMaskSize) {
  TGraph graph;
  auto seedNode = graph.createNode(nullptr);
  //One edge created before any clock exists...
  auto e0 = graph.createEdge(graph.createNode(nullptr), graph.createNode(nullptr), nullptr);
  EXPECT_EQ(0u, graph.getEdge(e0)->getFromClocks().size());

  graph.createClock("clkA", nullptr, seedNode);
  //...ends up sized like an edge created after that clock exists.
  //NB: re-fetch getEdge() after each mutation - createEdge()/createClock()
  //can reallocate the backing storage and invalidate previously-returned
  //TEdge* pointers; EdgeId is the stable handle, not the pointer.
  auto e1 = graph.createEdge(graph.createNode(nullptr), graph.createNode(nullptr), nullptr);
  EXPECT_EQ(1u, graph.getEdge(e0)->getFromClocks().size());
  EXPECT_EQ(1u, graph.getEdge(e1)->getFromClocks().size());
  EXPECT_EQ(1u, graph.getEdge(e0)->getToClocks().size());
  EXPECT_EQ(1u, graph.getEdge(e1)->getToClocks().size());

  //Every additional clock keeps both edges' masks in lockstep with getClocksCount().
  for (int i = 0; i < 10; ++i) {
    graph.createClock("clk" + std::to_string(i), nullptr, seedNode);
  }
  EXPECT_EQ(graph.getClocksCount(), graph.getEdge(e0)->getFromClocks().size());
  EXPECT_EQ(graph.getClocksCount(), graph.getEdge(e1)->getFromClocks().size());
  EXPECT_EQ(graph.getClocksCount(), graph.getEdge(e0)->getToClocks().size());
  EXPECT_EQ(graph.getClocksCount(), graph.getEdge(e1)->getToClocks().size());
}

TEST_F(TGraphTests, testLoopBreakerLeavesAcyclicGraphUntouched) {
  TGraph graph;
  //Diamond: n0 -> n1 -> n3, n0 -> n2 -> n3. No cycle.
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  auto n2 = graph.createNode(nullptr);
  auto n3 = graph.createNode(nullptr);
  auto e0 = graph.createEdge(n0, n1, nullptr);
  auto e1 = graph.createEdge(n0, n2, nullptr);
  auto e2 = graph.createEdge(n1, n3, nullptr);
  auto e3 = graph.createEdge(n2, n3, nullptr);

  EXPECT_EQ(0u, TLoopBreaker::run(graph));

  EXPECT_FALSE(graph.getEdge(e0)->isDisabled());
  EXPECT_FALSE(graph.getEdge(e1)->isDisabled());
  EXPECT_FALSE(graph.getEdge(e2)->isDisabled());
  EXPECT_FALSE(graph.getEdge(e3)->isDisabled());
}

TEST_F(TGraphTests, testLoopBreakerDisablesSelfLoop) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto selfLoop = graph.createEdge(n0, n0, nullptr);

  EXPECT_EQ(1u, TLoopBreaker::run(graph));
  EXPECT_TRUE(graph.getEdge(selfLoop)->isDisabled());
}

TEST_F(TGraphTests, testLoopBreakerBreaksThreeNodeCycle) {
  TGraph graph;
  //n0 -> n1 -> n2 -> n0.
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  auto n2 = graph.createNode(nullptr);
  auto e0 = graph.createEdge(n0, n1, nullptr);
  auto e1 = graph.createEdge(n1, n2, nullptr);
  auto e2 = graph.createEdge(n2, n0, nullptr);

  EXPECT_EQ(1u, TLoopBreaker::run(graph));

  //Exactly one of the three edges was cut; which one depends on DFS/edge
  //order, so just check the invariant: cutting it must break the cycle.
  const std::size_t disabledCount =
    (graph.getEdge(e0)->isDisabled() ? 1 : 0) +
    (graph.getEdge(e1)->isDisabled() ? 1 : 0) +
    (graph.getEdge(e2)->isDisabled() ? 1 : 0);
  EXPECT_EQ(1u, disabledCount);
}

TEST_F(TGraphTests, testLoopBreakerIsIdempotent) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  auto n2 = graph.createNode(nullptr);
  graph.createEdge(n0, n1, nullptr);
  graph.createEdge(n1, n2, nullptr);
  graph.createEdge(n2, n0, nullptr);

  EXPECT_EQ(1u, TLoopBreaker::run(graph));
  //Already acyclic once the back edge is disabled: nothing left to cut.
  EXPECT_EQ(0u, TLoopBreaker::run(graph));
}

TEST_F(TGraphTests, testClockPropagatorLinearChainHasNoCrossing) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  auto n2 = graph.createNode(nullptr);
  auto e0 = graph.createEdge(n0, n1, nullptr);
  auto e1 = graph.createEdge(n1, n2, nullptr);
  auto clk = graph.createClock("clk", nullptr, n0);

  TLoopBreaker::run(graph);
  TClockPropagator::run(graph);

  EXPECT_TRUE(graph.getEdge(e0)->hasFromClock(clk));
  EXPECT_TRUE(graph.getEdge(e1)->hasFromClock(clk));
  EXPECT_TRUE(graph.getEdge(e0)->hasToClock(clk));
  EXPECT_TRUE(graph.getEdge(e1)->hasToClock(clk));
  EXPECT_FALSE(graph.getEdge(e0)->hasClockCrossing());
  EXPECT_FALSE(graph.getEdge(e1)->hasClockCrossing());
}

TEST_F(TGraphTests, testClockPropagatorFlagsMergeOfTwoClockDomains) {
  TGraph graph;
  //a --clkA--> merge <--clkB-- b, merge -> downstream.
  auto a = graph.createNode(nullptr);
  auto b = graph.createNode(nullptr);
  auto merge = graph.createNode(nullptr);
  auto downstream = graph.createNode(nullptr);
  auto eA = graph.createEdge(a, merge, nullptr);
  auto eB = graph.createEdge(b, merge, nullptr);
  auto eOut = graph.createEdge(merge, downstream, nullptr);

  auto clkA = graph.createClock("clkA", nullptr, a);
  auto clkB = graph.createClock("clkB", nullptr, b);

  TLoopBreaker::run(graph);
  TClockPropagator::run(graph);

  //Each incoming edge only carries its own clock from the source side.
  EXPECT_TRUE(graph.getEdge(eA)->hasFromClock(clkA));
  EXPECT_FALSE(graph.getEdge(eA)->hasFromClock(clkB));
  EXPECT_TRUE(graph.getEdge(eB)->hasFromClock(clkB));
  EXPECT_FALSE(graph.getEdge(eB)->hasFromClock(clkA));

  //But the merge node sees both, so both incoming edges' ToClocks carry both.
  EXPECT_TRUE(graph.getEdge(eA)->hasToClock(clkA));
  EXPECT_TRUE(graph.getEdge(eA)->hasToClock(clkB));
  EXPECT_TRUE(graph.getEdge(eB)->hasToClock(clkA));
  EXPECT_TRUE(graph.getEdge(eB)->hasToClock(clkB));

  //Flagged: this is exactly the CDC-relevant point, two domains converging.
  EXPECT_TRUE(graph.getEdge(eA)->hasClockCrossing());
  EXPECT_TRUE(graph.getEdge(eB)->hasClockCrossing());

  //Downstream of the merge there's a single path carrying both clocks -
  //no further divergence, so this edge is not flagged even though it
  //carries multiple clock domains.
  EXPECT_TRUE(graph.getEdge(eOut)->hasFromClock(clkA));
  EXPECT_TRUE(graph.getEdge(eOut)->hasFromClock(clkB));
  EXPECT_FALSE(graph.getEdge(eOut)->hasClockCrossing());
}

TEST_F(TGraphTests, testClockPropagatorThrowsOnResidualCycle) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  graph.createEdge(n0, n1, nullptr);
  graph.createEdge(n1, n0, nullptr);
  //No TLoopBreaker::run() call: the graph still has a cycle among enabled
  //edges, which propagateForward() must reject rather than silently
  //produce incomplete masks.
  EXPECT_THROW(TClockPropagator::propagateForward(graph), std::logic_error);
}

TEST_F(TGraphTests, testClockPropagatorSucceedsAfterLoopBreaking) {
  TGraph graph;
  auto n0 = graph.createNode(nullptr);
  auto n1 = graph.createNode(nullptr);
  auto e0 = graph.createEdge(n0, n1, nullptr);
  auto e1 = graph.createEdge(n1, n0, nullptr);
  auto clk = graph.createClock("clk", nullptr, n0);

  TLoopBreaker::run(graph);
  EXPECT_NO_THROW(TClockPropagator::run(graph));

  //The back edge got disabled by the loop breaker; the clock should still
  //resolve on whichever edge survived enabled.
  const bool resolvedOnAnEnabledEdge =
    (!graph.getEdge(e0)->isDisabled() && graph.getEdge(e0)->hasFromClock(clk)) ||
    (!graph.getEdge(e1)->isDisabled() && graph.getEdge(e1)->hasFromClock(clk));
  EXPECT_TRUE(resolvedOnAnEnabledEdge);
}
