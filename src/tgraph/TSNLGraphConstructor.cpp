// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "TSNLGraphConstructor.h"

#include <set>
#include <utility>
#include <vector>

#include "NLException.h"
#include "SNLBitNet.h"
#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLPath.h"

namespace naja::TG {
namespace {

using namespace naja::NL;
using PinRole = TNode::PinRole;
using CellKind = TNode::CellKind;
using Model = SNLDesignModeling;
using NodeMap = std::map<SNLOccurrence, TGraph::NodeId>;

void fail(const SNLOccurrence& occurrence, const std::string& reason) {
  throw NLException("TSNLGraphConstructor: " + reason + " at " + occurrence.getDescription());
}

void checkDirection(const SNLOccurrence& occurrence, SNLTerm::Direction direction) {
  if (direction != SNLTerm::Direction::Input && direction != SNLTerm::Direction::Output) {
    fail(occurrence, "unsupported terminal direction");
  }
}

TGraph::NodeId addNode(TGraph& graph, NodeMap& nodes, const SNLOccurrence& occurrence) {
  auto id = graph.createNode(new TSNLNodeProxy(occurrence));
  nodes.emplace(occurrence, id);
  return id;
}

PinRole pinRole(SNLInstTerm* term, bool sequential) {
  if (!sequential) {
    return term->getDirection() == SNLTerm::Direction::Input ? PinRole::DataInput : PinRole::DataOutput;
  }
  switch (Model::getTermRole(term)) {
    case Model::SNLTermRole::Clock: return PinRole::Clock;
    case Model::SNLTermRole::DataInput: return PinRole::DataInput;
    case Model::SNLTermRole::DataOutput: return PinRole::DataOutput;
    case Model::SNLTermRole::Enable: return PinRole::Enable;
    case Model::SNLTermRole::SyncReset: return PinRole::SyncReset;
    case Model::SNLTermRole::SyncSet: return PinRole::SyncSet;
    case Model::SNLTermRole::AsyncReset: return PinRole::AsyncReset;
    case Model::SNLTermRole::AsyncSet: return PinRole::AsyncSet;
    default: return PinRole::Unknown;
  }
}

bool isCaptureRole(PinRole role) {
  return role == PinRole::DataInput || role == PinRole::Enable ||
    role == PinRole::SyncReset || role == PinRole::SyncSet;
}

void buildLeaf(TGraph& graph, NodeMap& nodes, const SNLPath& path, SNLInstance* instance) {
  auto* model = instance->getModel();
  const SNLOccurrence occurrence(path, instance);
  if (Model::hasMemoryInterface(model)) {
    fail(occurrence, "memory analysis is unsupported");
  }
  const bool sequential = Model::isSequential(model);
  SNLInstTerm* clock = nullptr;
  std::set<SNLBitTerm*> stateOutputs;
  if (sequential) {
    if (!Model::hasSequentialModel(model)) {
      fail(occurrence, "sequential cell has no explicit flip-flop/latch model");
    }
    const auto& seq = Model::getSequentialModel(instance);
    if (seq.kind != Model::SequentialModel::Kind::FlipFlop) {
      fail(occurrence, "latch analysis is unsupported");
    }
    //A single term, optionally inverted, is supported. Composite clock
    //expressions cannot be faithfully represented as a single C -> Q arc.
    const auto& expression = seq.clockedOn;
    auto root = expression.root;
    std::set<size_t> visited;
    while (root < expression.nodes.size() && visited.insert(root).second) {
      const auto& node = expression.nodes[root];
      if (node.operation == Model::BooleanExpression::Operator::Term && node.term) {
        clock = instance->getInstTerm(node.term);
        break;
      }
      if (node.operation != Model::BooleanExpression::Operator::Not || node.operands.size() != 1) {
        break;
      }
      root = node.operands.front();
    }
    if (!clock || pinRole(clock, true) != PinRole::Clock) {
      fail(occurrence, "unsupported clock expression or missing clock role");
    }
    for (const auto& output : seq.outputs) {
      auto index = output.function.root;
      std::set<size_t> seen;
      bool stateOutput = false;
      while (index < output.function.nodes.size() && seen.insert(index).second) {
        const auto& node = output.function.nodes[index];
        if (node.operation == Model::BooleanExpression::Operator::State) {
          stateOutput = node.state < seq.states.size();
          break;
        }
        if (node.operation != Model::BooleanExpression::Operator::Not || node.operands.size() != 1) {
          break;
        }
        index = node.operands.front();
      }
      if (!stateOutput) {
        fail(occurrence, "mixed combinational/sequential output is unsupported");
      }
      stateOutputs.insert(output.term);
    }
  } else if (!Model::hasModeling(model) && Model::getTruthTableCount(model) == 0) {
    fail(occurrence, "leaf cell has no combinational modeling");
  }

  auto nodeId = [&](SNLInstTerm* term) { return nodes.at(SNLOccurrence(path, term)); };
  std::set<SNLInstTerm*> outputs;
  std::set<SNLInstTerm*> captures;
  for (auto* term : instance->getInstTerms()) {
    auto* node = graph.getNode(nodeId(term));
    node->setCellKind(sequential ? CellKind::FlipFlop : CellKind::Combinational);
    auto role = pinRole(term, sequential);
    if (role == PinRole::Unknown ||
        (term->getDirection() == SNLTerm::Direction::Output) != (role == PinRole::DataOutput) ||
        (role == PinRole::Clock && term != clock)) {
      fail(SNLOccurrence(path, term), "unsupported or inconsistent pin role");
    }
    node->setPinRole(role);
  }

  if (sequential) {
    for (auto* output : Model::getClockRelatedOutputs(clock)) {
      if (pinRole(output, true) != PinRole::DataOutput) {
        fail(occurrence, "clock-to-output arc targets a non-output role");
      }
      if (!stateOutputs.count(output->getBitTerm())) {
        fail(occurrence, "clock-to-output arc has no sequential state output");
      }
      graph.createEdge(nodeId(clock), nodeId(output),
        new TSNLEdgeProxy(TSNLEdgeProxy::Kind::ClockToOutput, occurrence));
      outputs.insert(output);
    }
    for (auto* input : Model::getClockRelatedInputs(clock)) {
      if (isCaptureRole(pinRole(input, true))) {
        graph.createCaptureEdge(nodeId(clock), nodeId(input));
        captures.insert(input);
      }
    }
    for (auto* term : instance->getInstTerms()) {
      const auto role = pinRole(term, true);
      if ((role == PinRole::DataOutput && !outputs.count(term)) ||
          (isCaptureRole(role) && !captures.count(term))) {
        fail(SNLOccurrence(path, term), "missing sequential timing relationship");
      }
    }
  } else {
    for (auto* output : instance->getInstTerms()) {
      if (output->getDirection() != SNLTerm::Direction::Output) {
        continue;
      }
      const auto table = Model::getTruthTable(instance, output->getBitTerm()->getOrderID());
      if (table.isInitialized() && table.size() == 0) {
        continue;
      }
      auto inputs = Model::getCombinatorialInputs(output);
      if (inputs.empty()) {
        fail(SNLOccurrence(path, output), "output has no dependency model");
      }
      for (auto* input : inputs) {
        if (input->getDirection() != SNLTerm::Direction::Input) {
          fail(occurrence, "combinational arc source is not an input");
        }
        graph.createEdge(nodeId(input), nodeId(output),
          new TSNLEdgeProxy(TSNLEdgeProxy::Kind::Combinational, occurrence));
      }
    }
  }
}

void buildNets(TGraph& graph, const NodeMap& nodes, SNLDesign* design, const SNLPath& path) {
  for (auto* net : design->getBitNets()) {
    std::vector<TGraph::NodeId> drivers;
    std::vector<TGraph::NodeId> readers;
    for (auto* term : net->getBitTerms()) {
      auto& endpoints = term->getDirection() == SNLTerm::Direction::Input ? drivers : readers;
      endpoints.push_back(nodes.at(SNLOccurrence(path, term)));
    }
    for (auto* term : net->getInstTerms()) {
      auto& endpoints = term->getDirection() == SNLTerm::Direction::Output ? drivers : readers;
      endpoints.push_back(nodes.at(SNLOccurrence(path, term)));
    }
    if (drivers.size() > 1 || (net->getType().isDriving() && !drivers.empty())) {
      fail(SNLOccurrence(path, net), "multiple net drivers are unsupported");
    }
    for (auto driver : drivers) {
      for (auto reader : readers) {
        graph.createEdge(driver, reader,
          new TSNLEdgeProxy(TSNLEdgeProxy::Kind::Connectivity, SNLOccurrence(path, net)));
      }
    }
  }
}

}  // namespace

void TSNLGraphConstructor::construct() {
  if (!top_ || top_->isLeaf()) {
    throw NLException("TSNLGraphConstructor: expected a structural top design");
  }
  auto graph = std::make_unique<TGraph>();
  NodeMap nodes;
  for (auto* term : top_->getBitTerms()) {
    checkDirection(SNLOccurrence(term), term->getDirection());
    auto id = addNode(*graph, nodes, SNLOccurrence(term));
    graph->getNode(id)->setPinRole(term->getDirection() == SNLTerm::Direction::Input ?
      PinRole::DataInput : PinRole::DataOutput);
  }
  struct Context { SNLDesign* design; SNLPath path; };
  std::vector<Context> pending {{top_, SNLPath()}};
  while (!pending.empty()) {
    auto context = pending.back();
    pending.pop_back();
    for (auto* instance : context.design->getInstances()) {
      auto* model = instance->getModel();
      if (model == top_) {
        fail(SNLOccurrence(context.path, instance), "recursive hierarchy");
      }
      for (auto* ancestor : context.path.getInstances()) {
        if (ancestor->getModel() == model) {
          fail(SNLOccurrence(context.path, instance), "recursive hierarchy");
        }
      }
      SNLPath childPath(context.path, instance);
      for (auto* term : instance->getInstTerms()) {
        SNLOccurrence occurrence(context.path, term);
        checkDirection(occurrence, term->getDirection());
        auto id = addNode(*graph, nodes, occurrence);
        if (!model->isLeaf()) {
          graph->getNode(id)->setPinRole(term->getDirection() == SNLTerm::Direction::Input ?
            PinRole::DataInput : PinRole::DataOutput);
          nodes.emplace(SNLOccurrence(childPath, term->getBitTerm()), id);
        }
      }
      if (model->isLeaf()) {
        buildLeaf(*graph, nodes, context.path, instance);
      } else {
        pending.push_back({model, childPath});
      }
    }
    buildNets(*graph, nodes, context.design, context.path);
  }
  graph_ = std::move(graph);
  nodes_ = std::move(nodes);
}

TGraph& TSNLGraphConstructor::getGraph() {
  if (!graph_) throw NLException("TSNLGraphConstructor: construct() has not succeeded");
  return *graph_;
}

const TGraph& TSNLGraphConstructor::getGraph() const {
  if (!graph_) throw NLException("TSNLGraphConstructor: construct() has not succeeded");
  return *graph_;
}

TGraph::NodeId TSNLGraphConstructor::getNode(const naja::NL::SNLOccurrence& occurrence) const {
  const auto it = nodes_.find(occurrence);
  return it == nodes_.end() ? TGraph::kInvalidNode : it->second;
}

}  // namespace naja::TG
