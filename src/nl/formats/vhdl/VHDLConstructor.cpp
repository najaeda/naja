// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "VHDLConstructor.h"

#include "NLException.h"
#include "NLName.h"
#include "SNLDesign.h"
#include "SNLRTLPrimitives.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "vhdl/Analyzer.h"
#include "vhdl/Parser.h"

#include <string>
#include <unordered_map>

namespace naja::NL {
namespace {

std::string_view nameKey(const vhdl::Name& name) {
  return name.canonical.empty() ? std::string_view(name.spelling)
                                : std::string_view(name.canonical);
}

[[noreturn]] void unsupported(const std::string& message) {
  throw NLException("VHDL constructor: " + message);
}

}  // namespace

SNLDesign* VHDLConstructor::construct(std::string_view source) const {
  if (!library_) {
    unsupported("null library");
  }

  const auto parsed = vhdl::Parser::parse(source);
  if (parsed.hasErrors()) {
    unsupported("parse failed: " + parsed.diagnostics.front().message);
  }
  const auto analyzed = vhdl::Analyzer::analyze(parsed.syntax);
  if (analyzed.hasErrors()) {
    unsupported("analysis failed: " + analyzed.diagnostics.front().message);
  }
  if (parsed.syntax.entities.size() != 1 || parsed.syntax.architectures.size() != 1) {
    unsupported("exactly one entity and one architecture are supported");
  }

  const auto& entity = parsed.syntax.entities.front();
  const auto& architecture = parsed.syntax.architectures.front();
  if (nameKey(entity.name) != nameKey(architecture.entity)) {
    unsupported("architecture does not belong to the entity");
  }
  if (architecture.assignments.size() != 1) {
    unsupported("exactly one concurrent assignment is supported");
  }
  const auto& assignment = architecture.assignments.front();
  const auto& value = *assignment.value;
  if (value.kind != vhdl::Expression::Kind::Conditional || !value.condition ||
      value.condition->kind != vhdl::Expression::Kind::Binary ||
      value.condition->text != "=") {
    unsupported("expected a conditional assignment with an equality condition");
  }

  const vhdl::Expression* selectExpression = value.condition->left.get();
  const vhdl::Expression* literalExpression = value.condition->right.get();
  if (selectExpression->kind != vhdl::Expression::Kind::Name ||
      literalExpression->kind != vhdl::Expression::Kind::CharacterLiteral ||
      literalExpression->text != "'1'") {
    unsupported("the condition must compare a scalar name with '1'");
  }
  const auto requireName = [](const vhdl::Expression& expression) -> std::string_view {
    if (expression.kind != vhdl::Expression::Kind::Name) {
      unsupported("conditional branches must be scalar names");
    }
    return expression.canonical.empty() ? std::string_view(expression.text)
                                        : std::string_view(expression.canonical);
  };
  const auto trueName = requireName(*value.left);
  const auto falseName = requireName(*value.right);

  const auto selectName = selectExpression->canonical.empty()
      ? std::string_view(selectExpression->text)
      : std::string_view(selectExpression->canonical);
  std::unordered_map<std::string, vhdl::PortMode> portModes;
  for (const auto& port : entity.ports) {
    if (port.type.constraint || port.type.name.canonical != "bit") {
      unsupported("the first lowering slice supports only scalar bit ports");
    }
    if (port.mode != vhdl::PortMode::In && port.mode != vhdl::PortMode::Out) {
      unsupported("only in and out ports are supported");
    }
    for (const auto& name : port.names) {
      portModes.emplace(std::string(nameKey(name)), port.mode);
    }
  }
  const auto checkMode = [&portModes](std::string_view name, vhdl::PortMode mode,
                                      std::string_view role) {
    const auto it = portModes.find(std::string(name));
    if (it == portModes.end() || it->second != mode) {
      unsupported(std::string(role) + " must name a " +
                  (mode == vhdl::PortMode::Out ? "out" : "in") + " port");
    }
  };
  checkMode(nameKey(assignment.target), vhdl::PortMode::Out, "assignment target");
  checkMode(selectName, vhdl::PortMode::In, "select");
  checkMode(trueName, vhdl::PortMode::In, "true branch");
  checkMode(falseName, vhdl::PortMode::In, "false branch");

  struct Signal {
    SNLScalarNet* net;
  };
  std::unordered_map<std::string, Signal> signals;
  auto* design = SNLDesign::create(library_, NLName(entity.name.spelling));
  for (const auto& port : entity.ports) {
    const auto direction = port.mode == vhdl::PortMode::In
        ? SNLTerm::Direction::Input : SNLTerm::Direction::Output;
    for (const auto& name : port.names) {
      auto* term = SNLScalarTerm::create(design, direction, NLName(name.spelling));
      auto* net = SNLScalarNet::create(design, NLName(name.spelling));
      term->setNet(net);
      signals.emplace(std::string(nameKey(name)), Signal{net});
    }
  }

  const auto findSignal = [&signals](std::string_view name) -> Signal& {
    const auto it = signals.find(std::string(name));
    if (it == signals.end()) {
      unsupported("name is not a supported scalar port: " + std::string(name));
    }
    return it->second;
  };
  auto& output = findSignal(nameKey(assignment.target));
  auto& select = findSignal(selectName);
  auto& whenTrue = findSignal(trueName);
  auto& whenFalse = findSignal(falseName);
  SNLRTLPrimitives::createMux(design, select.net, {whenTrue.net},
                              {whenFalse.net}, output.net);
  return design;
}

}  // namespace naja::NL
