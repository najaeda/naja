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
#include <unordered_set>

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
  if (architecture.assignments.size() + architecture.processes.size() != 1) {
    unsupported("exactly one concurrent assignment or clocked process is supported");
  }
  const bool clocked = !architecture.processes.empty();
  const auto& assignment = clocked ? architecture.processes.front().assignments.front()
                                   : architecture.assignments.front();
  const auto requireName = [](const vhdl::Expression& expression) -> std::string_view {
    if (expression.kind != vhdl::Expression::Kind::Name) {
      unsupported("data values must be scalar names");
    }
    return expression.canonical.empty() ? std::string_view(expression.text)
                                        : std::string_view(expression.canonical);
  };
  std::string_view selectName, trueName, falseName;
  if (clocked) {
    const auto& process = architecture.processes.front();
    if (nameKey(process.sensitivity) != nameKey(process.eventSignal) ||
        nameKey(process.eventSignal) != nameKey(process.levelSignal) ||
        process.level != "'1'") {
      unsupported("expected matching sensitivity/event/level clocks and positive edge");
    }
    selectName = nameKey(process.eventSignal);
  } else {
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
    trueName = requireName(*value.left);
    falseName = requireName(*value.right);

    selectName = selectExpression->canonical.empty()
        ? std::string_view(selectExpression->text)
        : std::string_view(selectExpression->canonical);
  }
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
  std::unordered_set<std::string> internals;
  for (const auto& signal : architecture.signals) {
    if (signal.type.constraint || signal.type.name.canonical != "bit")
      unsupported("only scalar bit internal signals are supported");
    for (const auto& name : signal.names)
      internals.emplace(nameKey(name));
  }
  std::unordered_set<std::string> written;
  if (clocked) {
    for (const auto& write : architecture.processes.front().assignments) {
      const std::string target(nameKey(write.target));
      if (!internals.contains(target))
        checkMode(target, vhdl::PortMode::Out, "assignment target");
      if (!written.insert(target).second)
        unsupported("multiple scheduled writes to one target are not supported");
      const auto data = requireName(*write.value);
      if (!internals.contains(std::string(data)))
        checkMode(data, vhdl::PortMode::In, "data");
    }
    for (const auto& internal : internals) {
      if (!written.contains(internal))
        unsupported("internal signal has no supported driver: " + internal);
    }
  } else {
    if (!internals.empty())
      unsupported("internal signals currently require a clocked process");
    checkMode(nameKey(assignment.target), vhdl::PortMode::Out, "assignment target");
  }
  checkMode(selectName, vhdl::PortMode::In, clocked ? "clock" : "select");
  if (!clocked) {
    checkMode(trueName, vhdl::PortMode::In, "true branch");
    checkMode(falseName, vhdl::PortMode::In, "false branch");
  }

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

  for (const auto& signal : architecture.signals) {
    for (const auto& name : signal.names)
      signals.emplace(std::string(nameKey(name)),
          Signal{SNLScalarNet::create(design, NLName(name.spelling))});
  }

  const auto findSignal = [&signals](std::string_view name) -> Signal& {
    const auto it = signals.find(std::string(name));
    if (it == signals.end()) {
      unsupported("name is not a supported scalar signal: " + std::string(name));
    }
    return it->second;
  };
  auto& select = findSignal(selectName);
  if (clocked) {
    // Resolve every RHS to the current signal net, never to an earlier RHS.
    // DFFs sample those nets together: source order cannot bypass a stage.
    for (const auto& write : architecture.processes.front().assignments)
      SNLRTLPrimitives::createDFF(design, select.net,
          findSignal(requireName(*write.value)).net,
          findSignal(nameKey(write.target)).net);
  } else {
    auto& output = findSignal(nameKey(assignment.target));
    auto& whenTrue = findSignal(trueName);
    auto& whenFalse = findSignal(falseName);
    SNLRTLPrimitives::createMux(design, select.net, {whenTrue.net},
                                {whenFalse.net}, output.net);
  }
  return design;
}

}  // namespace naja::NL
