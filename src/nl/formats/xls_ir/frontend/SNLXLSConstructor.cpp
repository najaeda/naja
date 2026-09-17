// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLXLSConstructor.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "NLID.h"
#include "NLLibrary.h"
#include "NLName.h"
#include "SNLBitTerm.h"
#include "SNLBusNet.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLDesignBuilder.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLTerm.h"
#include "SNLXLSConstructorException.h"

namespace naja::NL {

namespace {

using Widths = std::unordered_map<std::string, size_t>;
using OrderedNodes = std::vector<const SNLXLSIRNode*>;

bool isRepresentableWidth(size_t width) {
  return width != 0 &&
    width - 1 <= static_cast<size_t>(std::numeric_limits<NLID::Bit>::max());
}

std::string describeNode(const SNLXLSIRNode& node) {
  std::ostringstream description;
  description << "node '" << node.name << "' (id=" << node.id
              << ", op=" << node.op << ")";
  if (!node.source.empty()) {
    description << " at " << node.source;
  }
  return description.str();
}

[[noreturn]] void failFunction(
  const SNLXLSIRFunction& function,
  const std::string& reason) {
  throw SNLXLSConstructorException(
    "XLS function '" + function.name + "': " + reason);
}

void requireOperandCount(
  const SNLXLSIRFunction& function,
  const SNLXLSIRNode& node,
  size_t expected) {
  if (node.operands.size() != expected) {
    failFunction(
      function,
      describeNode(node) + " expects " + std::to_string(expected) +
        " operands, got " + std::to_string(node.operands.size()));
  }
}

OrderedNodes validateAndOrder(const SNLXLSIRFunction& function) {
  if (function.name.empty()) {
    failFunction(function, "function name is empty");
  }
  if (function.outputName.empty()) {
    failFunction(function, "output name is empty");
  }
  if (function.result.empty()) {
    failFunction(function, "result value is empty");
  }

  Widths widths;
  std::unordered_set<std::string> parameterNames;
  for (const auto& parameter : function.parameters) {
    if (parameter.name.empty()) {
      failFunction(function, "parameter name is empty");
    }
    if (!isRepresentableWidth(parameter.width)) {
      failFunction(
        function,
        "parameter '" + parameter.name + "' has an invalid width " +
          std::to_string(parameter.width));
    }
    if (!widths.emplace(parameter.name, parameter.width).second) {
      failFunction(function, "duplicate value '" + parameter.name + "'");
    }
    parameterNames.insert(parameter.name);
  }
  if (parameterNames.contains(function.outputName)) {
    failFunction(
      function,
      "output name '" + function.outputName + "' conflicts with a parameter");
  }

  std::unordered_set<int64_t> nodeIDs;
  for (const auto& node : function.nodes) {
    if (node.name.empty()) {
      failFunction(function, "node name is empty");
    }
    if (node.id <= 0 || !nodeIDs.insert(node.id).second) {
      failFunction(function, "invalid or duplicate node id " + std::to_string(node.id));
    }
    if (!isRepresentableWidth(node.width)) {
      failFunction(
        function,
        describeNode(node) + " has an invalid width " +
          std::to_string(node.width));
    }
    if (!widths.emplace(node.name, node.width).second) {
      failFunction(function, "duplicate value '" + node.name + "'");
    }
  }

  for (const auto& node : function.nodes) {
    const size_t expectedOperands = node.op == "sel" ? 3 : 2;
    if (node.op != "add" && node.op != "sub" && node.op != "sel") {
      failFunction(function, describeNode(node) + " is unsupported");
    }
    requireOperandCount(function, node, expectedOperands);
    for (const auto& operand : node.operands) {
      if (!widths.contains(operand)) {
        failFunction(
          function,
          describeNode(node) + " references unknown operand '" + operand + "'");
      }
    }
    if (node.op == "sel") {
      if (widths.at(node.operands[0]) != 1) {
        failFunction(function, describeNode(node) + " selector must have width 1");
      }
      if (widths.at(node.operands[1]) != node.width ||
          widths.at(node.operands[2]) != node.width) {
        failFunction(function, describeNode(node) + " case width mismatch");
      }
    } else if (widths.at(node.operands[0]) != node.width ||
               widths.at(node.operands[1]) != node.width) {
      failFunction(function, describeNode(node) + " operand width mismatch");
    }
  }

  if (!widths.contains(function.result)) {
    failFunction(function, "result references unknown value '" + function.result + "'");
  }

  std::unordered_set<std::string> available = std::move(parameterNames);
  std::vector<bool> emitted(function.nodes.size(), false);
  OrderedNodes ordered;
  ordered.reserve(function.nodes.size());
  while (ordered.size() != function.nodes.size()) {
    bool progressed = false;
    for (size_t index = 0; index < function.nodes.size(); ++index) {
      if (emitted[index]) {
        continue;
      }
      const auto& node = function.nodes[index];
      if (std::all_of(
            node.operands.begin(),
            node.operands.end(),
            [&available](const std::string& operand) {
              return available.contains(operand);
            })) {
        emitted[index] = true;
        available.insert(node.name);
        ordered.push_back(&node);
        progressed = true;
      }
    }
    if (!progressed) {
      failFunction(function, "node dependency graph contains a cycle");
    }
  }
  return ordered;
}

SNLDesignBuilder::Bits createParameter(
  SNLDesign* design,
  const SNLXLSIRParameter& parameter) {
  if (parameter.width == 1) {
    auto* term = SNLScalarTerm::create(
      design, SNLTerm::Direction::Input, NLName(parameter.name));
    auto* net = SNLScalarNet::create(design, NLName(parameter.name));
    term->setNet(net);
    return {net};
  }

  auto msb = static_cast<NLID::Bit>(parameter.width - 1);
  auto* term = SNLBusTerm::create(
    design, SNLTerm::Direction::Input, msb, 0, NLName(parameter.name));
  auto* net = SNLBusNet::create(design, msb, 0, NLName(parameter.name));
  term->setNet(net);
  return SNLDesignBuilder::collectBits(net);
}

void createOutput(
  SNLDesign* design,
  const std::string& name,
  const SNLDesignBuilder::Bits& bits) {
  if (bits.size() == 1) {
    auto* term = SNLScalarTerm::create(
      design, SNLTerm::Direction::Output, NLName(name));
    term->setNet(bits.front());
    return;
  }

  auto msb = static_cast<NLID::Bit>(bits.size() - 1);
  auto* term = SNLBusTerm::create(
    design, SNLTerm::Direction::Output, msb, 0, NLName(name));
  for (size_t bit = 0; bit < bits.size(); ++bit) {
    term->getBit(static_cast<NLID::Bit>(bit))->setNet(bits[bit]);
  }
}

}  // namespace

SNLXLSConstructor::SNLXLSConstructor(NLLibrary* library): library_(library) {
  if (!library_) {
    throw SNLXLSConstructorException("XLS constructor requires a library");
  }
}

SNLDesign* SNLXLSConstructor::construct(const SNLXLSIRFunction& function) {
  const auto orderedNodes = validateAndOrder(function);
  if (library_->getSNLDesign(NLName(function.name))) {
    failFunction(function, "a design with this name already exists");
  }

  auto* design = SNLDesign::create(library_, NLName(function.name));
  try {
    std::unordered_map<std::string, SNLDesignBuilder::Bits> values;
    for (const auto& parameter : function.parameters) {
      values.emplace(parameter.name, createParameter(design, parameter));
    }

    SNLDesignBuilder builder(design);
    for (const auto* node : orderedNodes) {
      SNLDesignBuilder::Bits result;
      bool built = false;
      if (node->op == "add") {
        built = builder.add(
          values.at(node->operands[0]), values.at(node->operands[1]), result);
      } else if (node->op == "sub") {
        built = builder.subtract(
          values.at(node->operands[0]), values.at(node->operands[1]), result);
      } else if (node->op == "sel") {
        built = builder.mux(
          values.at(node->operands[0]).front(),
          values.at(node->operands[1]),
          values.at(node->operands[2]),
          result);
      }
      if (!built || result.size() != node->width) {
        failFunction(function, "failed to lower " + describeNode(*node));
      }
      values.emplace(node->name, std::move(result));
    }

    createOutput(design, function.outputName, values.at(function.result));
    return design;
  } catch (...) {
    design->destroy();
    throw;
  }
}

}  // namespace naja::NL
