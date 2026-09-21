// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "VHDLConstructor.h"

#include "NLException.h"
#include "NLName.h"
#include "SNLBitNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLDesign.h"
#include "SNLRTLPrimitives.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "vhdl/Analyzer.h"
#include "vhdl/Parser.h"

#include <functional>
#include <limits>
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

SNLRTLPrimitives::GateKind logicGateKind(std::string_view op) {
  if (op == "and") return SNLRTLPrimitives::GateKind::And;
  if (op == "nand") return SNLRTLPrimitives::GateKind::Nand;
  if (op == "or") return SNLRTLPrimitives::GateKind::Or;
  if (op == "nor") return SNLRTLPrimitives::GateKind::Nor;
  if (op == "xor") return SNLRTLPrimitives::GateKind::Xor;
  if (op == "xnor") return SNLRTLPrimitives::GateKind::Xnor;
  unsupported("unsupported scalar logical operator: " + std::string(op));
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
  std::string selectName;
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
    if (analyzed.getType(value) != vhdl::ScalarType::Bit &&
        analyzed.getType(value) != vhdl::ScalarType::BitVector) {
      unsupported("concurrent assignment value must have bit or constrained bit_vector type");
    }
    if (value.kind == vhdl::Expression::Kind::Conditional) {
      if (!value.condition || value.condition->kind != vhdl::Expression::Kind::Binary ||
          value.condition->text != "=") {
        unsupported("expected an equality condition");
      }
      const auto* selectExpression = value.condition->left.get();
      const auto* literalExpression = value.condition->right.get();
      if (selectExpression->kind != vhdl::Expression::Kind::Name ||
          literalExpression->kind != vhdl::Expression::Kind::CharacterLiteral ||
          literalExpression->text != "'1'") {
        unsupported("the condition must compare a scalar name with '1'");
      }
      selectName = selectExpression->canonical.empty()
          ? selectExpression->text : selectExpression->canonical;
    }
  }
  std::unordered_map<std::string, vhdl::PortMode> portModes;
  for (const auto& port : entity.ports) {
    const bool scalarBit = port.type.name.canonical == "bit" && !port.type.constraint;
    const bool bitVector = port.type.name.canonical == "bit_vector" && port.type.constraint;
    if (!scalarBit && !bitVector) {
      unsupported("only scalar bit and constrained bit_vector ports are supported");
    }
    if (clocked && !scalarBit)
      unsupported("clocked lowering currently supports only scalar bit ports");
    if (bitVector &&
        ((port.type.constraint->ascending &&
          port.type.constraint->left > port.type.constraint->right) ||
         (!port.type.constraint->ascending &&
          port.type.constraint->left < port.type.constraint->right)))
      unsupported("null bit_vector ranges have no SNL hardware representation");
    if (bitVector &&
        (port.type.constraint->left < std::numeric_limits<NLID::Bit>::min() ||
         port.type.constraint->left > std::numeric_limits<NLID::Bit>::max() ||
         port.type.constraint->right < std::numeric_limits<NLID::Bit>::min() ||
         port.type.constraint->right > std::numeric_limits<NLID::Bit>::max())) {
      unsupported("bit_vector bounds exceed the supported net index range");
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
  vhdl::ScheduleResult schedule;
  if (clocked) {
    const auto& process = architecture.processes.front();
    std::unordered_set<std::string> variables;
    for (const auto& variable : process.variables) {
      if (variable.type.constraint || variable.type.name.canonical != "bit")
        unsupported("only scalar bit process variables are supported");
      for (const auto& name : variable.names)
        variables.emplace(nameKey(name));
    }
    for (const auto& statement : process.assignments) {
      const auto data = requireName(*statement.value);
      if (!variables.contains(std::string(data)) && !internals.contains(std::string(data)))
        checkMode(data, vhdl::PortMode::In, "data");
    }
    schedule = vhdl::Analyzer::schedule(process);
    if (schedule.hasErrors())
      unsupported("scheduling failed: " + schedule.diagnostics.front().message);
    const std::unordered_set<std::string> retained(
        schedule.retainedVariables.begin(), schedule.retainedVariables.end());
    std::size_t signalWrites = 0;
    for (const auto& write : schedule.writes)
      signalWrites += write.kind == vhdl::AssignmentKind::Signal;
    if (signalWrites == 0)
      unsupported("a clocked process must schedule a signal write");
    for (const auto& write : schedule.writes) {
      const auto& target = write.target;
      if (write.kind == vhdl::AssignmentKind::Variable) {
        if (!retained.contains(target))
          unsupported("variable state write does not name retained storage");
      } else {
        if (!internals.contains(target))
          checkMode(target, vhdl::PortMode::Out, "assignment target");
        if (!written.insert(target).second)
          unsupported("multiple scheduled writes to one target are not supported");
      }
      const auto& data = write.source;
      if (!internals.contains(std::string(data)) && !retained.contains(data))
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
    const auto validateExpression = [&](const auto& self,
                                        const vhdl::Expression& expression) -> void {
      switch (expression.kind) {
        case vhdl::Expression::Kind::Name: {
          const auto name = expression.canonical.empty()
              ? std::string_view(expression.text)
              : std::string_view(expression.canonical);
          checkMode(name, vhdl::PortMode::In, "expression operand");
          return;
        }
        case vhdl::Expression::Kind::CharacterLiteral:
          if (expression.text != "'0'" && expression.text != "'1'")
            unsupported("only bit character literals '0' and '1' are supported");
          return;
        case vhdl::Expression::Kind::Unary:
          if (expression.text != "not")
            unsupported("unsupported scalar unary expression");
          self(self, *expression.left);
          return;
        case vhdl::Expression::Kind::Binary:
          logicGateKind(expression.text);
          self(self, *expression.left);
          self(self, *expression.right);
          return;
        default:
          unsupported("unsupported scalar bit expression shape");
      }
    };
    if (assignment.value->kind == vhdl::Expression::Kind::Conditional) {
      checkMode(selectName, vhdl::PortMode::In, "select");
      validateExpression(validateExpression, *assignment.value->left);
      validateExpression(validateExpression, *assignment.value->right);
    } else {
      validateExpression(validateExpression, *assignment.value);
    }
  }
  if (clocked)
    checkMode(selectName, vhdl::PortMode::In, "clock");

  struct Signal {
    SNLNet* net;
  };
  std::unordered_map<std::string, Signal> signals;
  auto* design = SNLDesign::create(library_, NLName(entity.name.spelling));
  for (const auto& port : entity.ports) {
    const auto direction = port.mode == vhdl::PortMode::In
        ? SNLTerm::Direction::Input : SNLTerm::Direction::Output;
    for (const auto& name : port.names) {
      if (port.type.constraint) {
        const auto left = static_cast<NLID::Bit>(port.type.constraint->left);
        const auto right = static_cast<NLID::Bit>(port.type.constraint->right);
        auto* term = SNLBusTerm::create(
          design, direction, left, right, NLName(name.spelling));
        auto* net = SNLBusNet::create(design, left, right, NLName(name.spelling));
        term->setNet(net);
        signals.emplace(std::string(nameKey(name)), Signal{net});
      } else {
        auto* term = SNLScalarTerm::create(design, direction, NLName(name.spelling));
        auto* net = SNLScalarNet::create(design, NLName(name.spelling));
        term->setNet(net);
        signals.emplace(std::string(nameKey(name)), Signal{net});
      }
    }
  }

  for (const auto& signal : architecture.signals) {
    for (const auto& name : signal.names)
      signals.emplace(std::string(nameKey(name)),
          Signal{SNLScalarNet::create(design, NLName(name.spelling))});
  }

  if (clocked) {
    const std::unordered_set<std::string> retained(
        schedule.retainedVariables.begin(), schedule.retainedVariables.end());
    for (const auto& declaration : architecture.processes.front().variables) {
      for (const auto& name : declaration.names) {
        if (retained.contains(std::string(nameKey(name))))
          signals.emplace(std::string(nameKey(name)),
              Signal{SNLScalarNet::create(design, NLName(name.spelling))});
      }
    }
  }

  const auto findSignal = [&signals](std::string_view name) -> Signal& {
    const auto it = signals.find(std::string(name));
    if (it == signals.end()) {
      unsupported("name is not a supported scalar signal: " + std::string(name));
    }
    return it->second;
  };
  if (clocked) {
    auto& select = findSignal(selectName);
    // The frontend has frozen RHS values at each scheduled write, applying
    // immediate variable assignments without forwarding scheduled signal writes.
    for (const auto& write : schedule.writes)
      SNLRTLPrimitives::createDFF(design, select.net,
          findSignal(write.source).net, findSignal(write.target).net);
  } else {
    auto& output = findSignal(nameKey(assignment.target));
    const auto createExpressionNet = [&](const vhdl::Expression& expression) -> SNLNet* {
      if (analyzed.getType(expression) == vhdl::ScalarType::Bit)
        return SNLScalarNet::create(design);
      const auto* range = analyzed.getRange(expression);
      if (!range)
        unsupported("vector expression has no constrained range");
      return SNLBusNet::create(
        design, static_cast<NLID::Bit>(range->left),
        static_cast<NLID::Bit>(range->right));
    };
    const auto hardwareBits = [](SNLNet* net) {
      SNLRTLPrimitives::Bits bits;
      if (auto* bit = dynamic_cast<SNLBitNet*>(net)) {
        bits.push_back(bit);
      } else {
        auto* bus = static_cast<SNLBusNet*>(net);
        bits.reserve(static_cast<size_t>(bus->getWidth()));
        for (size_t position = 0; position < static_cast<size_t>(bus->getWidth()); ++position)
          bits.push_back(bus->getBitAtPosition(
            static_cast<size_t>(bus->getWidth()) - 1 - position));
      }
      return bits;
    };
    std::function<SNLNet*(const vhdl::Expression&, SNLNet*)> lowerExpression;
    lowerExpression = [&](const vhdl::Expression& expression,
                          SNLNet* requestedOutput) -> SNLNet* {
      if (expression.kind == vhdl::Expression::Kind::Name) {
        const auto name = expression.canonical.empty()
            ? std::string_view(expression.text)
            : std::string_view(expression.canonical);
        auto* input = findSignal(name).net;
        if (!requestedOutput)
          return input;
        SNLRTLPrimitives::createBitwiseGate(
            design, SNLRTLPrimitives::GateKind::Buf, {input}, requestedOutput);
        return requestedOutput;
      }
      if (expression.kind == vhdl::Expression::Kind::CharacterLiteral) {
        auto* constant = requestedOutput
            ? requestedOutput : SNLScalarNet::create(design);
        constant->setType(expression.text == "'1'"
            ? SNLNet::Type::Assign1 : SNLNet::Type::Assign0);
        return constant;
      }
      if (expression.kind == vhdl::Expression::Kind::Unary) {
        auto* input = lowerExpression(*expression.left, nullptr);
        auto* result = requestedOutput ? requestedOutput : createExpressionNet(expression);
        SNLRTLPrimitives::createBitwiseGate(
            design, SNLRTLPrimitives::GateKind::Not, {input}, result);
        return result;
      }
      if (expression.kind == vhdl::Expression::Kind::Binary) {
        auto* left = lowerExpression(*expression.left, nullptr);
        auto* right = lowerExpression(*expression.right, nullptr);
        auto* result = requestedOutput ? requestedOutput : createExpressionNet(expression);
        SNLRTLPrimitives::createBitwiseGate(
            design, logicGateKind(expression.text), {left, right}, result);
        return result;
      }
      if (expression.kind == vhdl::Expression::Kind::Conditional) {
        auto* whenTrue = lowerExpression(*expression.left, nullptr);
        auto* whenFalse = lowerExpression(*expression.right, nullptr);
        auto* result = requestedOutput ? requestedOutput : createExpressionNet(expression);
        auto* select = dynamic_cast<SNLBitNet*>(findSignal(selectName).net);
        if (!select)
          unsupported("conditional select must be scalar bit");
        SNLRTLPrimitives::createMux(
            design, select, hardwareBits(whenTrue), hardwareBits(whenFalse), result);
        return result;
      }
      unsupported("unsupported scalar bit expression during lowering");
    };
    lowerExpression(*assignment.value, output.net);
  }
  return design;
}

}  // namespace naja::NL
