// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NLDB.h"
#include "NLDB0.h"
#include "NLException.h"
#include "NLLibrary.h"
#include "NLUniverse.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLInstParameter.h"
#include "SNLParameter.h"
#include "SNLPath.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "TClockPropagator.h"
#include "TSNLGraphConstructor.h"

using namespace naja::NL;
using namespace naja::TG;

class TSNLGraphConstructorTests: public ::testing::Test {
  protected:
    void SetUp() override {
      auto* db = NLDB::create(NLUniverse::create());
      library_ = NLLibrary::create(db);
      top_ = SNLDesign::create(library_, NLName("top"));
    }
    void TearDown() override { NLUniverse::get()->destroy(); }
    SNLScalarTerm* port(SNLDesign* design, const char* name, SNLTerm::Direction direction) {
      return SNLScalarTerm::create(design, direction, NLName(name));
    }
    void connect(SNLDesign* design, std::initializer_list<SNLNetComponent*> terms) {
      auto* net = SNLScalarNet::create(design);
      for (auto* term : terms) term->setNet(net);
    }
    SNLInstTerm* term(SNLInstance* instance, const char* name) {
      return instance->getInstTerm(instance->getModel()->getScalarTerm(NLName(name)));
    }
    TGraph::EdgeId edge(const TGraph& graph, TGraph::NodeId from, TGraph::NodeId to) {
      for (const auto& e : graph.getEdgesStorage()) {
        if (e.isAlive() && e.getSource() == from && e.getTarget() == to) return e.getIndex();
      }
      return TGraph::kInvalidEdge;
    }
    NLLibrary* library_ {nullptr};
    SNLDesign* top_ {nullptr};
};

TEST_F(TSNLGraphConstructorTests, registerTransferThroughCombinationalCell) {
  auto* clockA = port(top_, "clock_a", SNLTerm::Direction::Input);
  auto* clockB = port(top_, "clock_b", SNLTerm::Direction::Input);
  auto* input = port(top_, "input", SNLTerm::Direction::Input);
  auto* output = port(top_, "output", SNLTerm::Direction::Output);
  auto* launch = SNLInstance::create(top_, NLDB0::getDFF(), NLName("launch"));
  auto* capture = SNLInstance::create(top_, NLDB0::getDFF(), NLName("capture"));
  auto* buffer = SNLInstance::create(top_, NLDB0::getAssign(), NLName("buffer"));
  connect(top_, {clockA, term(launch, "C")});
  connect(top_, {clockB, term(capture, "C")});
  connect(top_, {input, term(launch, "D")});
  connect(top_, {term(launch, "Q"), buffer->getInstTerm(NLDB0::getAssignInput())});
  connect(top_, {buffer->getInstTerm(NLDB0::getAssignOutput()), term(capture, "D")});
  connect(top_, {term(capture, "Q"), output});

  TSNLGraphConstructor constructor(top_);
  EXPECT_THROW(constructor.getGraph(), NLException);
  constructor.construct();
  auto& graph = constructor.getGraph();
  auto id = [&](SNLNetComponent* component) { return constructor.getNode(SNLOccurrence(component)); };
  EXPECT_EQ(2u, graph.getCaptureEdgesCount());
  EXPECT_EQ(0u, graph.getClocksCount());
  EXPECT_TRUE(graph.getNode(id(term(launch, "Q")))->isFlopDataOutput());
  EXPECT_TRUE(graph.getNode(id(term(capture, "D")))->isFlopDataInput());
  EXPECT_TRUE(graph.getNode(id(buffer->getInstTerm(NLDB0::getAssignInput())))->isCombinationalReceiver());
  auto cq = edge(graph, id(term(launch, "C")), id(term(launch, "Q")));
  ASSERT_NE(TGraph::kInvalidEdge, cq);
  EXPECT_EQ(TSNLEdgeProxy::Kind::ClockToOutput,
    dynamic_cast<const TSNLEdgeProxy*>(graph.getEdge(cq)->getProxy())->getKind());
  EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, id(term(launch, "D")), id(term(launch, "Q"))));
  EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, id(term(capture, "C")), id(term(capture, "D"))));

  auto a = graph.createClock("a", clockA, id(clockA));
  auto b = graph.createClock("b", clockB, id(clockB));
  TClockPropagator::run(graph);
  auto data = edge(graph, id(buffer->getInstTerm(NLDB0::getAssignOutput())), id(term(capture, "D")));
  ASSERT_NE(TGraph::kInvalidEdge, data);
  EXPECT_TRUE(graph.getEdge(data)->hasFromClock(a));
  EXPECT_TRUE(graph.getEdge(data)->hasToClock(b));
  EXPECT_TRUE(graph.getEdge(data)->hasClockCrossing());
  EXPECT_TRUE(graph.getEdge(cq)->getToClocks().none());
  const auto* proxy = dynamic_cast<const TSNLNodeProxy*>(graph.getNode(id(term(capture, "D")))->getProxy());
  ASSERT_NE(nullptr, proxy);
  EXPECT_EQ(SNLOccurrence(term(capture, "D")), proxy->getOccurrence());
}

TEST_F(TSNLGraphConstructorTests, sharedHierarchyPreservesOccurrencesAndPortAliases) {
  auto* block = SNLDesign::create(library_, NLName("block"));
  auto* in = port(block, "in", SNLTerm::Direction::Input);
  auto* out = port(block, "out", SNLTerm::Direction::Output);
  auto* buffer = SNLInstance::create(block, NLDB0::getAssign(), NLName("buffer"));
  connect(block, {in, buffer->getInstTerm(NLDB0::getAssignInput())});
  connect(block, {buffer->getInstTerm(NLDB0::getAssignOutput()), out});
  auto* left = SNLInstance::create(top_, block, NLName("left"));
  auto* right = SNLInstance::create(top_, block, NLName("right"));
  auto* root = port(top_, "root", SNLTerm::Direction::Input);
  auto* sink = port(top_, "sink", SNLTerm::Direction::Output);
  connect(top_, {root, left->getInstTerm(in)});
  connect(top_, {left->getInstTerm(out), right->getInstTerm(in)});
  connect(top_, {right->getInstTerm(out), sink});
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto& graph = constructor.getGraph();
  auto leftPin = SNLOccurrence(SNLPath(left), buffer->getInstTerm(NLDB0::getAssignInput()));
  auto rightPin = SNLOccurrence(SNLPath(right), buffer->getInstTerm(NLDB0::getAssignInput()));
  EXPECT_NE(constructor.getNode(leftPin), constructor.getNode(rightPin));
  EXPECT_EQ(constructor.getNode(SNLOccurrence(left->getInstTerm(in))),
    constructor.getNode(SNLOccurrence(SNLPath(left), in)));
  auto clk = graph.createClock("clk", root, constructor.getNode(SNLOccurrence(root)));
  TClockPropagator::run(graph);
  auto sinkId = constructor.getNode(SNLOccurrence(sink));
  ASSERT_EQ(1u, graph.getNode(sinkId)->getInDegree());
  EXPECT_TRUE(graph.getEdge(graph.getNode(sinkId)->getInEdges().front())->hasFromClock(clk));
  EXPECT_EQ(TGraph::kInvalidNode, constructor.getNode(SNLOccurrence(in)));
}

TEST_F(TSNLGraphConstructorTests, busFlopsAndControlRoles) {
  auto* wideModel = NLDB0::getOrCreateDFF(3);
  auto* wide = SNLInstance::create(top_, wideModel, NLName("wide"));
  auto* enabled = SNLInstance::create(top_, NLDB0::getDFFRE(), NLName("enabled"));
  auto* synchronous = SNLInstance::create(top_, NLDB0::getDFFSR(), NLName("synchronous"));
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto& graph = constructor.getGraph();
  for (auto* bit : wideModel->getBusTerm(NLName("D"))->getBits()) {
    auto id = constructor.getNode(SNLOccurrence(wide->getInstTerm(bit)));
    EXPECT_TRUE(graph.getNode(id)->isFlopDataInput());
    EXPECT_EQ(1u, graph.getNode(id)->getCaptureInEdges().size());
  }
  auto check = [&](SNLInstance* instance, const char* name, TNode::PinRole role, size_t captures) {
    auto id = constructor.getNode(SNLOccurrence(term(instance, name)));
    EXPECT_EQ(role, graph.getNode(id)->getPinRole());
    EXPECT_EQ(captures, graph.getNode(id)->getCaptureInEdges().size());
  };
  check(enabled, "E", TNode::PinRole::Enable, 1);
  check(enabled, "R", TNode::PinRole::AsyncReset, 0);
  check(synchronous, "R", TNode::PinRole::SyncReset, 1);
}

TEST_F(TSNLGraphConstructorTests, parameterizedCombinationalArcsUseInstanceValues) {
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("selector"));
  auto* a = port(model, "a", SNLTerm::Direction::Input);
  auto* b = port(model, "b", SNLTerm::Direction::Input);
  auto* q = port(model, "q", SNLTerm::Direction::Output);
  SNLParameter::create(model, NLName("SELECT"), SNLParameter::Type::Decimal, "0");
  SNLDesignModeling::setParameter(model, "SELECT", "0");
  SNLDesignModeling::addCombinatorialArcs("0", {a}, {q});
  SNLDesignModeling::addCombinatorialArcs("1", {b}, {q});
  auto* first = SNLInstance::create(top_, model, NLName("first"));
  auto* second = SNLInstance::create(top_, model, NLName("second"));
  SNLInstParameter::create(second, model->getParameter(NLName("SELECT")), "1");
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto& graph = constructor.getGraph();
  auto id = [&](SNLInstance* instance, SNLBitTerm* bit) {
    return constructor.getNode(SNLOccurrence(instance->getInstTerm(bit)));
  };
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, id(first, a), id(first, q)));
  EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, id(first, b), id(first, q)));
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, id(second, b), id(second, q)));
  EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, id(second, a), id(second, q)));
}

TEST_F(TSNLGraphConstructorTests, unsupportedModelsFailWithoutReplacingPreviousGraph) {
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto* previous = &constructor.getGraph();
  auto* unknown = SNLDesign::create(library_, SNLDesign::Type::UserBlackBox, NLName("unknown"));
  auto* instance = SNLInstance::create(top_, unknown, NLName("instance"));
  EXPECT_THROW(constructor.construct(), NLException);
  EXPECT_EQ(previous, &constructor.getGraph());
  instance->destroy();
  instance = SNLInstance::create(top_, NLDB0::getDLatch(), NLName("latch"));
  EXPECT_THROW(constructor.construct(), NLException);
  instance->destroy();
  auto* io = port(top_, "io", SNLTerm::Direction::InOut);
  EXPECT_THROW(constructor.construct(), NLException);
  io->destroy();
  auto* a = port(top_, "a", SNLTerm::Direction::Input);
  auto* b = port(top_, "b", SNLTerm::Direction::Input);
  connect(top_, {a, b});
  EXPECT_THROW(constructor.construct(), NLException);
  EXPECT_EQ(previous, &constructor.getGraph());
}

TEST_F(TSNLGraphConstructorTests, recursiveHierarchyAndInvalidTopAreRejected) {
  TSNLGraphConstructor nullConstructor(nullptr);
  EXPECT_THROW(nullConstructor.construct(), NLException);
  TSNLGraphConstructor leafConstructor(NLDB0::getDFF());
  EXPECT_THROW(leafConstructor.construct(), NLException);
  auto* child = SNLDesign::create(library_, NLName("child"));
  SNLInstance::create(top_, child, NLName("child"));
  SNLInstance::create(child, top_, NLName("recursive"));
  TSNLGraphConstructor constructor(top_);
  EXPECT_THROW(constructor.construct(), NLException);
}

TEST_F(TSNLGraphConstructorTests, constantsAndUnconnectedPinsDoNotInventDependencies) {
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("constant"));
  port(model, "unused", SNLTerm::Direction::Input);
  port(model, "value", SNLTerm::Direction::Output);
  SNLDesignModeling::setTruthTable(model, SNLTruthTable(0, 1, {}));
  auto* instance = SNLInstance::create(top_, model, NLName("constant"));
  auto* output = port(top_, "out", SNLTerm::Direction::Output);
  connect(top_, {term(instance, "value"), output});
  TSNLGraphConstructor constructor(top_);
  constructor.construct();
  auto& graph = constructor.getGraph();
  EXPECT_EQ(3u, graph.getNodesCount());
  EXPECT_EQ(1u, graph.getEdgesCount());
  auto unused = constructor.getNode(SNLOccurrence(term(instance, "unused")));
  EXPECT_EQ(0u, graph.getNode(unused)->getOutDegree());
  EXPECT_NO_THROW(TClockPropagator::run(graph));
}

TEST_F(TSNLGraphConstructorTests, mixedSequentialOutputsAreRejected) {
  auto* model = NLDB0::getDFF();
  auto sequence = SNLDesignModeling::getSequentialModel(model);
  sequence.outputs.front().function = {};
  sequence.outputs.front().function.root =
    sequence.outputs.front().function.addTerm(NLDB0::getDFFData());
  SNLDesignModeling::setSequentialModel(model, sequence);
  SNLInstance::create(top_, model, NLName("mixed"));
  TSNLGraphConstructor constructor(top_);
  EXPECT_THROW(constructor.construct(), NLException);
  EXPECT_THROW(constructor.getGraph(), NLException);
}
