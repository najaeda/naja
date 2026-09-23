// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "VHDLConstructor.h"
#include "VHDLRTLConstructor.h"

#include "NLException.h"
#include "NLName.h"
#include "SNLBitNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLRTLPrimitives.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLTerm.h"
#include "vhdl/Analyzer.h"
#include "vhdl/Parser.h"

#include <cctype>
#include <exception>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
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

std::string diagnosticMessage(
    std::string_view phase, std::string_view message,
    const vhdl::SourceSpan& span) {
  return std::string(phase) + " failed at line " +
      std::to_string(span.start.line) + ", column " +
      std::to_string(span.start.column) + ": " + std::string(message);
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

std::string canonicalBasicName(std::string_view name) {
  std::string canonical(name);
  for (auto& character : canonical) {
    character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
  }
  return canonical;
}

std::optional<std::size_t> supportedWidth(
    const vhdl::TypeMark& type, const vhdl::DiscreteRange* resolvedRange) {
  if (type.name.canonical == "bit" && !type.constraint) return 1;
  if (type.name.canonical != "bit_vector" || !resolvedRange) return std::nullopt;
  const auto& range = *resolvedRange;
  if ((range.ascending && range.left > range.right) ||
      (!range.ascending && range.left < range.right)) return std::nullopt;
  if (range.left < std::numeric_limits<NLID::Bit>::min() ||
      range.left > std::numeric_limits<NLID::Bit>::max() ||
      range.right < std::numeric_limits<NLID::Bit>::min() ||
      range.right > std::numeric_limits<NLID::Bit>::max()) return std::nullopt;
  return static_cast<std::size_t>(range.left > range.right
      ? range.left - range.right + 1 : range.right - range.left + 1);
}

const vhdl::DiscreteRange* effectiveRange(
    const vhdl::TypeMark& type, const vhdl::AnalysisResult& analysis) {
  if (const auto* resolved = analysis.getRange(type)) return resolved;
  return type.constraint ? &*type.constraint : nullptr;
}

}  // namespace

SNLDesign* VHDLConstructor::construct(std::string_view source) const {
  return construct(source, {});
}

SNLDesign* VHDLConstructor::constructFile(
    const std::filesystem::path& path, std::string_view top) const {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    unsupported("cannot open VHDL file '" + path.string() + "'");
  }
  const std::string source(
      (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  try {
    return construct(source, top);
  } catch (const std::exception& exception) {
    throw NLException(
        "VHDL file '" + path.string() + "': " + exception.what());
  }
}

SNLDesign* VHDLConstructor::construct(
    std::string_view source, std::string_view top) const {
  if (!library_) {
    unsupported("null library");
  }

  const auto parsed = vhdl::Parser::parse(source);
  if (parsed.hasErrors()) {
    const auto& diagnostic = parsed.diagnostics.front();
    unsupported(diagnosticMessage(
        "parse", diagnostic.message, diagnostic.span));
  }
  if (requiresVHDLRTL(parsed.syntax))
    return constructVHDLRTL(library_, parsed.syntax, top);
  const auto analyzed = vhdl::Analyzer::analyze(parsed.syntax);
  if (analyzed.hasErrors()) {
    const auto& diagnostic = analyzed.diagnostics.front();
    unsupported(diagnosticMessage(
        "analysis", diagnostic.message, diagnostic.span));
  }
  const bool hierarchy = parsed.syntax.entities.size() != 1 ||
      parsed.syntax.architectures.size() != 1 ||
      (!parsed.syntax.architectures.empty() &&
       !parsed.syntax.architectures.front().instantiations.empty());
  if (hierarchy) {
    if (top.empty()) {
      unsupported("an explicit top entity is required for hierarchy");
    }
    const auto topKey = canonicalBasicName(top);
    std::unordered_map<std::string, const vhdl::EntityDeclaration*> entities;
    std::unordered_map<std::string, std::vector<const vhdl::ArchitectureBody*>> architectures;
    for (const auto& entity : parsed.syntax.entities)
      entities.emplace(std::string(nameKey(entity.name)), &entity);
    for (const auto& architecture : parsed.syntax.architectures)
      architectures[std::string(nameKey(architecture.entity))].push_back(&architecture);
    const auto topEntityIt = entities.find(topKey);
    if (topEntityIt == entities.end())
      unsupported("no entity declaration for selected top '" + std::string(top) + "'");
    const auto topArchitectures = architectures.find(topKey);
    if (topArchitectures == architectures.end() || topArchitectures->second.size() != 1)
      unsupported("selected top must have exactly one architecture");
    const auto* topEntity = topEntityIt->second;
    const auto* topArchitecture = topArchitectures->second.front();
    if (!topArchitecture->assignments.empty() || !topArchitecture->processes.empty())
      unsupported("a structural top cannot mix behavior and entity instances");
    if (topArchitecture->instantiations.empty())
      unsupported("selected hierarchy top has no entity instances");

    struct ObjectInfo {
      const vhdl::TypeMark* type;
      std::optional<vhdl::PortMode> mode;
    };
    std::unordered_map<std::string, ObjectInfo> objects;
    std::unordered_map<std::string, std::size_t> drivers;
    for (const auto& port : topEntity->ports) {
      if (!supportedWidth(port.type, effectiveRange(port.type, analyzed)) ||
          (port.mode != vhdl::PortMode::In && port.mode != vhdl::PortMode::Out))
        unsupported("hierarchy ports must be in/out bit or constrained non-null bit_vector");
      for (const auto& name : port.names) {
        objects.emplace(std::string(nameKey(name)),
            ObjectInfo{&port.type, port.mode});
        if (port.mode == vhdl::PortMode::Out) drivers.emplace(nameKey(name), 0);
      }
    }
    for (const auto& signal : topArchitecture->signals) {
      if (!supportedWidth(signal.type, effectiveRange(signal.type, analyzed)))
        unsupported("hierarchy signals must be bit or constrained non-null bit_vector");
      for (const auto& name : signal.names) {
        objects.emplace(std::string(nameKey(name)),
            ObjectInfo{&signal.type, std::nullopt});
        drivers.emplace(nameKey(name), 0);
      }
    }

    struct BoundInstance {
      const vhdl::EntityInstantiation* syntax;
      const vhdl::EntityDeclaration* entity;
      std::string specialization;
    };
    std::vector<BoundInstance> boundInstances;
    std::vector<std::string> childKeys;
    std::unordered_set<std::string> seenChildKeys;
    for (const auto& instantiation : topArchitecture->instantiations) {
      const auto childKey = std::string(nameKey(instantiation.entity));
      if (childKey == topKey)
        unsupported("recursive hierarchy is not supported");
      const auto childEntityIt = entities.find(childKey);
      const auto childArchitectureIt = architectures.find(childKey);
      if (childEntityIt == entities.end() || childArchitectureIt == architectures.end() ||
          childArchitectureIt->second.size() != 1)
        unsupported("each instantiated entity must have exactly one architecture");
      const auto* childArchitecture = childArchitectureIt->second.front();
      if (!childArchitecture->instantiations.empty())
        unsupported("only one level of hierarchy is supported");
      std::vector<std::pair<const vhdl::PortDeclaration*, const vhdl::Name*>> formals;
      for (const auto& port : childEntityIt->second->ports) {
        const auto* specializedRange = analyzed.getRange(instantiation, port.type);
        if (!supportedWidth(port.type, specializedRange) ||
            (port.mode != vhdl::PortMode::In && port.mode != vhdl::PortMode::Out))
          unsupported("child ports must be in/out bit or constrained non-null bit_vector");
        for (const auto& name : port.names) formals.emplace_back(&port, &name);
      }
      if (formals.size() != instantiation.actuals.size())
        unsupported("positional port-map arity mismatch");
      for (std::size_t index = 0; index < formals.size(); ++index) {
        const auto actual = objects.find(std::string(nameKey(instantiation.actuals[index])));
        if (actual == objects.end()) unsupported("port-map actual has no declaration");
        const auto actualWidth = supportedWidth(
            *actual->second.type, effectiveRange(*actual->second.type, analyzed));
        const auto formalWidth = supportedWidth(formals[index].first->type,
            analyzed.getRange(instantiation, formals[index].first->type));
        if (actualWidth != formalWidth)
          unsupported("port-map actual and formal widths differ");
        const auto formalMode = formals[index].first->mode;
        if (formalMode == vhdl::PortMode::In &&
            actual->second.mode == vhdl::PortMode::Out)
          unsupported("an input formal cannot read a top output port");
        if (formalMode == vhdl::PortMode::Out) {
          if (actual->second.mode == vhdl::PortMode::In)
            unsupported("an output formal cannot drive a top input port");
          if (++drivers[std::string(nameKey(instantiation.actuals[index]))] != 1)
            unsupported("multiple hierarchy drivers for one actual are not supported");
        }
      }
      std::string specialization = childKey;
      const auto values = analyzed.genericValues.find(&instantiation);
      if (values != analyzed.genericValues.end()) {
        std::vector<std::pair<std::string, std::int64_t>> ordered(
            values->second.begin(), values->second.end());
        std::sort(ordered.begin(), ordered.end());
        for (const auto& [name, value] : ordered)
          specialization += "_" + name + "_" + std::to_string(value);
      }
      if (seenChildKeys.insert(specialization).second)
        childKeys.push_back(specialization);
      boundInstances.push_back(
          {&instantiation, childEntityIt->second, std::move(specialization)});
    }
    for (const auto& [name, count] : drivers)
      if (count != 1) unsupported("hierarchy signal has no supported driver: " + name);
    if (library_->getSNLDesign(NLName(topEntity->name.spelling)))
      unsupported("a design with the top entity name already exists");

    try {
      std::unordered_map<std::string, SNLDesign*> models;
      for (const auto& specialization : childKeys) {
        const auto bound = std::find_if(boundInstances.begin(), boundInstances.end(),
            [&](const BoundInstance& candidate) {
              return candidate.specialization == specialization;
            });
        const auto* childEntity = bound->entity;
        const auto childKey = std::string(nameKey(childEntity->name));
        const auto* childArchitecture = architectures.at(childKey).front();
        const auto entityStart = childEntity->span.start.offset;
        const auto architectureStart = childArchitecture->span.start.offset;
        std::string childSource(source.substr(
            entityStart, childEntity->span.end.offset - entityStart));
        childSource.push_back('\n');
        childSource.append(source.substr(architectureStart,
            childArchitecture->span.end.offset - architectureStart));
        struct Edit { std::size_t start; std::size_t length; std::string value; };
        std::vector<Edit> edits;
        const auto& values = analyzed.genericValues.at(bound->syntax);
        for (const auto& generic : childEntity->generics) {
          std::optional<std::int64_t> value;
          for (const auto& name : generic.names) {
            const auto current = values.at(std::string(nameKey(name)));
            if (value && *value != current)
              unsupported("grouped generics with different actual values are not supported");
            value = current;
          }
          if (generic.defaultValue) {
            edits.push_back({generic.defaultValue->span.start.offset - entityStart,
                generic.defaultValue->span.end.offset - generic.defaultValue->span.start.offset,
                std::to_string(*value)});
          } else {
            edits.push_back({generic.type.name.span.end.offset - entityStart, 0,
                " := " + std::to_string(*value)});
          }
        }
        std::sort(edits.begin(), edits.end(),
            [](const Edit& left, const Edit& right) { return left.start > right.start; });
        for (const auto& edit : edits)
          childSource.replace(edit.start, edit.length, edit.value);
        auto* model = construct(childSource);
        model->setName(NLName(specialization));
        models.emplace(specialization, model);
      }
      auto* design = SNLDesign::create(library_, NLName(topEntity->name.spelling));
      std::unordered_map<std::string, SNLNet*> nets;
      for (const auto& port : topEntity->ports) {
        const auto direction = port.mode == vhdl::PortMode::In
            ? SNLTerm::Direction::Input : SNLTerm::Direction::Output;
        for (const auto& name : port.names) {
          SNLNet* net = nullptr;
          if (const auto* range = effectiveRange(port.type, analyzed)) {
            const auto left = static_cast<NLID::Bit>(range->left);
            const auto right = static_cast<NLID::Bit>(range->right);
            auto* term = SNLBusTerm::create(design, direction, left, right, NLName(name.spelling));
            net = SNLBusNet::create(design, left, right, NLName(name.spelling));
            term->setNet(net);
          } else {
            auto* term = SNLScalarTerm::create(design, direction, NLName(name.spelling));
            net = SNLScalarNet::create(design, NLName(name.spelling));
            term->setNet(net);
          }
          nets.emplace(std::string(nameKey(name)), net);
        }
      }
      for (const auto& signal : topArchitecture->signals) {
        for (const auto& name : signal.names) {
          SNLNet* net = nullptr;
          if (const auto* range = effectiveRange(signal.type, analyzed)) {
            net = SNLBusNet::create(design,
                static_cast<NLID::Bit>(range->left),
                static_cast<NLID::Bit>(range->right), NLName(name.spelling));
          } else {
            net = SNLScalarNet::create(design, NLName(name.spelling));
          }
          nets.emplace(std::string(nameKey(name)), net);
        }
      }
      for (const auto& bound : boundInstances) {
        auto* instance = SNLInstance::create(design,
            models.at(bound.specialization),
            NLName(bound.syntax->label.spelling));
        std::size_t index = 0;
        for (const auto& port : bound.entity->ports) {
          for (const auto& formal : port.names) {
            auto* term = instance->getModel()->getTerm(NLName(formal.spelling));
            auto* net = nets.at(std::string(nameKey(bound.syntax->actuals[index++])));
            instance->setTermNet(term, net);
          }
        }
      }
      return design;
    } catch (...) {
      if (auto* design = library_->getSNLDesign(NLName(topEntity->name.spelling)))
        design->destroy();
      for (const auto& childKey : childKeys)
        if (auto* design = library_->getSNLDesign(NLName(entities.at(childKey)->name.spelling)))
          design->destroy();
      throw;
    }
  }
  if (parsed.syntax.entities.size() != 1 || parsed.syntax.architectures.size() != 1) {
    unsupported("exactly one entity and one architecture are supported");
  }

  const auto& entity = parsed.syntax.entities.front();
  if (!top.empty() && canonicalBasicName(top) != nameKey(entity.name))
    unsupported("selected top does not match the source entity");
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
  std::string enableName;
  std::string resetName;
  if (clocked) {
    const auto& process = architecture.processes.front();
    if (nameKey(process.sensitivity) != nameKey(process.eventSignal) ||
        nameKey(process.eventSignal) != nameKey(process.levelSignal) ||
        process.level != "'1'") {
      unsupported("expected matching sensitivity/event/level clocks and positive edge");
    }
    selectName = nameKey(process.eventSignal);
    if (process.enableSignal) {
      if (process.enableLevel != "'1'")
        unsupported("only active-high clock enables are supported");
      enableName = nameKey(*process.enableSignal);
    }
    if (process.resetSignal) {
      if (process.resetLevel != "'1'")
        unsupported("only active-high synchronous resets are supported");
      resetName = nameKey(*process.resetSignal);
    }
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
    const auto* range = effectiveRange(port.type, analyzed);
    const bool scalarBit = port.type.name.canonical == "bit" && !range;
    const bool bitVector = port.type.name.canonical == "bit_vector" && range;
    if (!scalarBit && !bitVector) {
      unsupported("only scalar bit and constrained bit_vector ports are supported");
    }
    if (clocked && !scalarBit)
      unsupported("clocked lowering currently supports only scalar bit ports");
    if (bitVector &&
        ((range->ascending && range->left > range->right) ||
         (!range->ascending && range->left < range->right)))
      unsupported("null bit_vector ranges have no SNL hardware representation");
    if (bitVector &&
        (range->left < std::numeric_limits<NLID::Bit>::min() ||
         range->left > std::numeric_limits<NLID::Bit>::max() ||
         range->right < std::numeric_limits<NLID::Bit>::min() ||
         range->right > std::numeric_limits<NLID::Bit>::max())) {
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
    if (!resetName.empty() && !variables.empty())
      unsupported("process variables with synchronous reset are not supported");
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
    if (!resetName.empty()) {
      std::unordered_set<std::string> resetTargets;
      for (const auto& resetAssignment : process.resetAssignments) {
        const auto target = std::string(nameKey(resetAssignment.target));
        if (resetAssignment.kind != vhdl::AssignmentKind::Signal ||
            resetAssignment.value->kind != vhdl::Expression::Kind::CharacterLiteral ||
            resetAssignment.value->text != "'0'") {
          unsupported("synchronous reset branches must assign signal targets to '0'");
        }
        if (!written.contains(target))
          unsupported("synchronous reset and data branches must assign the same targets");
        if (!resetTargets.insert(target).second)
          unsupported("multiple synchronous reset writes to one target are not supported");
      }
      if (resetTargets.size() != written.size())
        unsupported("synchronous reset and data branches must assign the same targets");
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
  if (clocked) {
    checkMode(selectName, vhdl::PortMode::In, "clock");
    if (!enableName.empty())
      checkMode(enableName, vhdl::PortMode::In, "enable");
    if (!resetName.empty())
      checkMode(resetName, vhdl::PortMode::In, "reset");
  }

  struct Signal {
    SNLNet* net;
  };
  std::unordered_map<std::string, Signal> signals;
  auto designGuard = std::unique_ptr<SNLDesign, void (*)(SNLDesign*)>(
      SNLDesign::create(library_, NLName(entity.name.spelling)),
      [](SNLDesign* design) { design->destroy(); });
  auto* design = designGuard.get();
  for (const auto& port : entity.ports) {
    const auto direction = port.mode == vhdl::PortMode::In
        ? SNLTerm::Direction::Input : SNLTerm::Direction::Output;
    for (const auto& name : port.names) {
      if (const auto* range = effectiveRange(port.type, analyzed)) {
        const auto left = static_cast<NLID::Bit>(range->left);
        const auto right = static_cast<NLID::Bit>(range->right);
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
    for (const auto& write : schedule.writes) {
      if (!resetName.empty() && !enableName.empty()) {
        SNLRTLPrimitives::createDFFSRE(design, select.net,
            findSignal(write.source).net, findSignal(enableName).net,
            findSignal(resetName).net, findSignal(write.target).net);
      } else if (!resetName.empty()) {
        SNLRTLPrimitives::createDFFSR(design, select.net,
            findSignal(write.source).net, findSignal(resetName).net,
            findSignal(write.target).net);
      } else if (enableName.empty()) {
        SNLRTLPrimitives::createDFF(design, select.net,
            findSignal(write.source).net, findSignal(write.target).net);
      } else {
        SNLRTLPrimitives::createDFFE(design, select.net,
            findSignal(write.source).net, findSignal(enableName).net,
            findSignal(write.target).net);
      }
    }
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
  return designGuard.release();
}

}  // namespace naja::NL
