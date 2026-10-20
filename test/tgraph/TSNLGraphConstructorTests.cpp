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
    void expectFailure(TSNLGraphConstructor& constructor, const std::string& reason) {
      try {
        constructor.construct();
        FAIL() << "Expected construction to fail: " << reason;
      } catch (const NLException& error) {
        EXPECT_NE(std::string::npos, std::string(error.what()).find(reason)) << error.what();
      }
      EXPECT_THROW(constructor.getGraph(), NLException);
    }
    SNLDesign* modeledFlop() {
      using Model = SNLDesignModeling;
      auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
      auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("flop"));
      auto* clock = port(model, "C", SNLTerm::Direction::Input);
      auto* data = port(model, "D", SNLTerm::Direction::Input);
      auto* output = port(model, "Q", SNLTerm::Direction::Output);
      Model::SequentialModel sequence;
      sequence.clockedOn.root = sequence.clockedOn.addTerm(clock);
      sequence.states.emplace_back();
      auto& next = sequence.states.front().nextState;
      next.root = next.addTerm(data);
      sequence.outputs.push_back({output, {}});
      auto& function = sequence.outputs.front().function;
      function.root = function.addState(0);
      Model::setSequentialModel(model, sequence);
      Model::setTermRole(clock, Model::SNLTermRole::Clock);
      Model::setTermRole(data, Model::SNLTermRole::DataInput);
      Model::setTermRole(output, Model::SNLTermRole::DataOutput);
      Model::addInputsToClockArcs({data}, clock);
      Model::addClockToOutputsArcs(clock, {output});
      return model;
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
  auto* syncSet = SNLInstance::create(top_, NLDB0::getDFFSS(), NLName("sync_set"));
  auto* asyncSet = SNLInstance::create(top_, NLDB0::getDFFS(), NLName("async_set"));
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
  check(syncSet, "S", TNode::PinRole::SyncSet, 1);
  check(asyncSet, "S", TNode::PinRole::AsyncSet, 0);
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

TEST_F(TSNLGraphConstructorTests, invertedClockAndStateOutputPreserveTimingEdges) {
  using Model = SNLDesignModeling;
  using Operator = Model::BooleanExpression::Operator;
  auto* model = NLDB0::getDFF();
  auto sequence = Model::getSequentialModel(model);
  sequence.clockedOn.root = sequence.clockedOn.addOperation(Operator::Not, {sequence.clockedOn.root});
  auto& output = sequence.outputs.front().function;
  output.root = output.addOperation(Operator::Not, {output.root});
  Model::setSequentialModel(model, sequence);
  auto* instance = SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  const auto& constConstructor = constructor;
  EXPECT_THROW(constConstructor.getGraph(), NLException);
  constructor.construct();
  const auto& graph = constConstructor.getGraph();
  EXPECT_EQ(&constructor.getGraph(), &graph);
  auto clock = constructor.getNode(SNLOccurrence(term(instance, "C")));
  auto data = constructor.getNode(SNLOccurrence(term(instance, "D")));
  auto q = constructor.getNode(SNLOccurrence(term(instance, "Q")));
  EXPECT_NE(TGraph::kInvalidEdge, edge(graph, clock, q));
  EXPECT_EQ(TGraph::kInvalidEdge, edge(graph, data, q));
  EXPECT_EQ(1u, graph.getNode(data)->getCaptureInEdges().size());
}

TEST_F(TSNLGraphConstructorTests, sequentialTimingWithoutExplicitModelIsRejected) {
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("legacy"));
  auto* clock = port(model, "clock", SNLTerm::Direction::Input);
  auto* data = port(model, "data", SNLTerm::Direction::Input);
  SNLDesignModeling::addInputsToClockArcs({data}, clock);
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "sequential cell has no explicit flip-flop/latch model");
}

TEST_F(TSNLGraphConstructorTests, compositeClockExpressionIsRejected) {
  using Model = SNLDesignModeling;
  auto* model = NLDB0::getDFF();
  auto sequence = Model::getSequentialModel(model);
  auto data = sequence.clockedOn.addTerm(NLDB0::getDFFData());
  sequence.clockedOn.root = sequence.clockedOn.addOperation(
    Model::BooleanExpression::Operator::And, {sequence.clockedOn.root, data});
  Model::setSequentialModel(model, sequence);
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "unsupported clock expression or missing clock role");
}

TEST_F(TSNLGraphConstructorTests, missingClockRoleIsRejected) {
  SNLDesignModeling::setTermRole(NLDB0::getDFFClock(), SNLDesignModeling::SNLTermRole::DataInput);
  SNLInstance::create(top_, NLDB0::getDFF(), NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "unsupported clock expression or missing clock role");
}

TEST_F(TSNLGraphConstructorTests, unknownPinRoleIsRejected) {
  auto* model = NLDB0::getDFF();
  port(model, "extra", SNLTerm::Direction::Input);
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "unsupported or inconsistent pin role");
}

TEST_F(TSNLGraphConstructorTests, clockArcToInputIsRejected) {
  auto* model = modeledFlop();
  SNLDesignModeling::addClockToOutputsArcs(
    model->getScalarTerm(NLName("C")), {model->getScalarTerm(NLName("D"))});
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "clock-to-output arc targets a non-output role");
}

TEST_F(TSNLGraphConstructorTests, clockArcWithoutStateOutputIsRejected) {
  auto* model = NLDB0::getDFF();
  auto* extra = port(model, "extra", SNLTerm::Direction::Output);
  SNLDesignModeling::setTermRole(extra, SNLDesignModeling::SNLTermRole::DataOutput);
  SNLDesignModeling::addClockToOutputsArcs(NLDB0::getDFFClock(), {extra});
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "clock-to-output arc has no sequential state output");
}

TEST_F(TSNLGraphConstructorTests, capturePinWithoutTimingRelationshipIsRejected) {
  auto* model = modeledFlop();
  auto* extra = port(model, "extra", SNLTerm::Direction::Input);
  SNLDesignModeling::setTermRole(extra, SNLDesignModeling::SNLTermRole::DataInput);
  SNLInstance::create(top_, model, NLName("flop"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "missing sequential timing relationship");
}

TEST_F(TSNLGraphConstructorTests, outputWithoutDependencyIsRejected) {
  auto* primitives = NLLibrary::create(library_->getDB(), NLLibrary::Type::Primitives);
  auto* model = SNLDesign::create(primitives, SNLDesign::Type::Primitive, NLName("partial"));
  auto* input = port(model, "input", SNLTerm::Direction::Input);
  auto* output = port(model, "output", SNLTerm::Direction::Output);
  SNLDesignModeling::addCombinatorialArcs({input}, {output});
  port(model, "extra", SNLTerm::Direction::Output);
  SNLInstance::create(top_, model, NLName("buffer"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "output has no dependency model");
}

TEST_F(TSNLGraphConstructorTests, combinationalArcFromOutputIsRejected) {
  auto* model = NLDB0::getAssign();
  SNLDesignModeling::addCombinatorialArcs({NLDB0::getAssignOutput()}, {NLDB0::getAssignOutput()});
  SNLInstance::create(top_, model, NLName("buffer"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "combinational arc source is not an input");
}

TEST_F(TSNLGraphConstructorTests, recursiveHierarchyBelowTopIsRejected) {
  auto* child = SNLDesign::create(library_, NLName("child"));
  auto* descendant = SNLDesign::create(library_, NLName("descendant"));
  SNLInstance::create(top_, child, NLName("child"));
  SNLInstance::create(child, descendant, NLName("descendant"));
  SNLInstance::create(descendant, child, NLName("recursive"));
  TSNLGraphConstructor constructor(top_);
  expectFailure(constructor, "recursive hierarchy");
}
