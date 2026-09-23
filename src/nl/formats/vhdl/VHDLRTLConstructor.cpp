// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "VHDLRTLConstructor.h"

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

#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>
#include <map>
#include <memory>
#include <set>

namespace naja::NL {
namespace {
using Expr = vhdl::Expression;
using Statement = vhdl::SequentialStatement;
using Bits = std::vector<SNLBitNet*>;

[[noreturn]] void fail(const std::string& message) {
  throw NLException("VHDL constructor: " + message);
}

std::string key(const vhdl::Name& name) {
  return name.canonical.empty() ? name.spelling : name.canonical;
}

std::string key(const Expr& expression) {
  return expression.canonical.empty() ? expression.text : expression.canonical;
}

struct Range {
  int64_t left;
  int64_t right;
  bool ascending;
  size_t size() const {
    if (ascending ? left > right : left < right) return 0;
    const auto width = ascending ? __int128(right) - left + 1 : __int128(left) - right + 1;
    if (width > 65536) fail("array range exceeds static elaboration limit");
    return static_cast<size_t>(width);
  }
  size_t position(int64_t index) const {
    if (ascending ? index < left || index > right : index > left || index < right)
      fail("static index " + std::to_string(index) + " is outside the declared range");
    return static_cast<size_t>(ascending ? index - left : left - index);
  }
};

struct Shape {
  // One nominal type at each dimension, followed by the scalar element type.
  std::vector<std::string> types;
  std::vector<Range> ranges;
  size_t size() const {
    size_t size = 1;
    for (const auto& range : ranges) {
      const auto width = range.size();
      if (!width || size > 65536 / width) fail("null or oversized hardware array");
      size *= width;
    }
    return size;
  }
};

bool compatible(const Shape& a, const Shape& b) {
  if (a.types != b.types || a.ranges.size() != b.ranges.size()) return false;
  for (size_t i = 0; i < a.ranges.size(); ++i)
    if (a.ranges[i].size() != b.ranges[i].size()) return false;
  return true;
}

struct Value { Shape shape; Bits bits; };
struct Object {
  Value value;
  bool input = false;
  bool output = false;
  bool variable = false;
};
struct Selection { std::string name; Shape shape; size_t offset = 0; };
struct State {
  std::map<std::string, Bits> variables;
  std::map<std::string, Bits> scheduled;
  std::map<std::string, std::vector<bool>> written;
};

class RTLConstructor {
 public:
  RTLConstructor(SNLDesign* design, const vhdl::EntityDeclaration& entity,
                 const vhdl::ArchitectureBody& architecture):
      design_(design), entity_(entity), architecture_(architecture) {}

  void run() {
    context(entity_.context);
    for (const auto& generic : entity_.generics) {
      const auto type = key(generic.type.name);
      if ((type != "integer" && type != "natural" && type != "positive") ||
          generic.type.constraint || !generic.defaultValue)
        fail("RTL generics require a defaulted integer subtype");
      const auto value = integer(*generic.defaultValue);
      if ((type == "positive" && value < 1) || (type == "natural" && value < 0))
        fail("generic value violates its subtype");
      for (const auto& name : generic.names) {
        if (!integers_.emplace(key(name), value).second) fail("duplicate generic");
      }
    }
    for (const auto& port : entity_.ports) {
      if (port.mode != vhdl::PortMode::In && port.mode != vhdl::PortMode::Out)
        fail("RTL ports must be in or out");
      const auto type = shape(port.type);
      for (const auto& name : port.names)
        addObject(name, type, port.mode == vhdl::PortMode::In,
                  port.mode == vhdl::PortMode::Out, false, true);
    }
    context(architecture_.context);
    for (const auto& declaration : architecture_.arrayTypes) {
      const auto name = key(declaration.name);
      if (arrays_.contains(name) || integers_.contains(name) || objects_.contains(name) ||
          name == "bit" || name == "bit_vector" || name == "std_logic" ||
          name == "std_logic_vector") fail("duplicate or shadowing array type: " + name);
      auto element = shape(declaration.elementType);
      element.types.insert(element.types.begin(), name);
      element.ranges.insert(element.ranges.begin(), range(declaration.indexRange));
      element.size();
      arrays_.emplace(name, std::move(element));
      arrayTypeOffsets_.emplace(name, declaration.span.start.offset);
    }
    for (const auto& signal : architecture_.signals) {
      const auto type = shape(signal.type);
      for (const auto& name : signal.names) addObject(name, type);
    }
    State empty;
    for (const auto& assignment : architecture_.assignments)
      concurrent(assignment, empty);
    if (architecture_.processes.size() > 1) fail("multiple RTL processes are not supported");
    for (const auto& process : architecture_.processes) lowerProcess(process);
    for (const auto& [name, object] : objects_) {
      if (object.variable || object.input) continue;
      for (auto* bit : object.value.bits)
        if ((object.output || reads_.contains(bit)) && !drivers_.contains(bit))
          fail("signal bit has no driver: " + name);
    }
  }

 private:
  void context(const vhdl::ContextClause& clauses) {
    for (const auto& library : clauses.libraries)
      for (const auto& name : library.names) libraries_.insert(key(name));
    for (const auto& use : clauses.uses) {
      if (use.selectedName.size() != 3 || key(use.selectedName[0]) != "ieee" ||
          key(use.selectedName[2]) != "all" || !libraries_.contains("ieee"))
        fail("unsupported RTL package visibility");
      const auto package = key(use.selectedName[1]);
      if (package == "std_logic_1164") stdLogic_ = true;
      else if (package != "numeric_std" && package != "std_logic_unsigned")
        fail("unsupported IEEE package: " + package);
      // Importing numeric packages does not enable arithmetic on logic vectors.
      // Only the explicitly checked bitwise operators below are lowered.
    }
  }

  int64_t integer(const Expr& expression) {
    if (expression.kind == Expr::Kind::Name) {
      const auto found = integers_.find(key(expression));
      if (found != integers_.end()) return found->second;
    } else if (expression.kind == Expr::Kind::IntegerLiteral) {
      std::string digits;
      for (char c : expression.text) if (c != '_') digits += c;
      int64_t value;
      const auto result = std::from_chars(digits.data(), digits.data() + digits.size(), value);
      if (result.ec == std::errc{} && result.ptr == digits.data() + digits.size()) return value;
    } else if (expression.kind == Expr::Kind::Unary || expression.kind == Expr::Kind::Binary) {
      const __int128 left = integer(*expression.left);
      __int128 value;
      const auto& op = expression.text;
      if (expression.kind == Expr::Kind::Unary) {
        if (op != "+" && op != "-") fail("unsupported static unary operator");
        value = op == "-" ? -left : left;
      } else {
        const __int128 right = integer(*expression.right);
        if (op == "+") value = left + right;
        else if (op == "-") value = left - right;
        else if (op == "*") value = left * right;
        else fail("unsupported static integer operator");
      }
      if (value < std::numeric_limits<int64_t>::min() || value > std::numeric_limits<int64_t>::max())
        fail("static integer overflow");
      return static_cast<int64_t>(value);
    }
    fail("index or bound must be a static integer expression");
  }

  Range range(const vhdl::DiscreteRange& range) {
    return {range.leftExpression ? integer(*range.leftExpression) : range.left,
            range.rightExpression ? integer(*range.rightExpression) : range.right,
            range.ascending};
  }

  Shape shape(const vhdl::TypeMark& type) {
    const auto name = key(type.name);
    if (const auto found = arrays_.find(name); found != arrays_.end()) {
      if (type.name.span.start.offset < arrayTypeOffsets_.at(name))
        fail("array type is used before its declaration: " + name);
      if (type.constraint) fail("reconstraining an array type is not supported");
      return found->second;
    }
    if (name == "bit" || (stdLogic_ && name == "std_logic")) {
      if (type.constraint) fail("scalar type cannot have a range");
      return {{name}, {}};
    }
    if (name == "bit_vector" || (stdLogic_ && name == "std_logic_vector")) {
      if (!type.constraint) fail("vector requires a constraint");
      return {{name, name == "bit_vector" ? "bit" : "std_logic"}, {range(*type.constraint)}};
    }
    fail("unsupported or invisible RTL type: " + name);
  }

  void addObject(const vhdl::Name& name, const Shape& type, bool input = false,
                 bool output = false, bool variable = false, bool port = false) {
    const auto id = key(name);
    if (objects_.contains(id) || integers_.contains(id) || arrays_.contains(id))
      fail("duplicate or shadowing object: " + id);
    Bits bits;
    const auto size = type.size();
    if (variable) bits.resize(size, nullptr);
    else if (type.ranges.size() == 1) {
      const auto& r = type.ranges.front();
      if (r.left < std::numeric_limits<NLID::Bit>::min() || r.left > std::numeric_limits<NLID::Bit>::max() ||
          r.right < std::numeric_limits<NLID::Bit>::min() || r.right > std::numeric_limits<NLID::Bit>::max())
        fail("hardware vector bounds exceed net index range");
      auto* net = SNLBusNet::create(design_, r.left, r.right, NLName(name.spelling));
      for (size_t i = 0; i < size; ++i) bits.push_back(net->getBitAtPosition(i));
      if (port) SNLBusTerm::create(design_, input ? SNLTerm::Direction::Input :
          SNLTerm::Direction::Output, r.left, r.right, NLName(name.spelling))->setNet(net);
    } else {
      for (size_t i = 0; i < size; ++i) {
        auto* bit = SNLScalarNet::create(design_, NLName(name.spelling +
            (size == 1 ? "" : "_" + std::to_string(i))));
        bits.push_back(bit);
      }
      if (port) SNLScalarTerm::create(design_, input ? SNLTerm::Direction::Input :
          SNLTerm::Direction::Output, NLName(name.spelling))->setNet(bits.front());
    }
    objects_.emplace(id, Object{{type, std::move(bits)}, input, output, variable});
  }

  Selection selection(const std::string& name) {
    const auto found = objects_.find(name);
    if (found == objects_.end()) fail("no declaration for object: " + name);
    return {name, found->second.value.shape, 0};
  }
  void index(Selection& selected, const Expr& expression) {
    if (selected.shape.ranges.empty()) fail("cannot index a scalar object");
    const auto position = selected.shape.ranges.front().position(integer(expression));
    selected.shape.ranges.erase(selected.shape.ranges.begin());
    selected.shape.types.erase(selected.shape.types.begin());
    selected.offset += position * selected.shape.size();
  }
  Selection selection(const Expr& expression) {
    if (expression.kind == Expr::Kind::Name) return selection(key(expression));
    if (expression.kind != Expr::Kind::Indexed) fail("indexed prefix must name an object");
    auto selected = selection(*expression.left);
    index(selected, *expression.right);
    return selected;
  }
  Selection selection(const vhdl::Assignment& assignment) {
    auto selected = selection(key(assignment.target));
    for (const auto& expression : assignment.indices) index(selected, *expression);
    return selected;
  }

  SNLBitNet* constant(bool one) {
    auto*& net = one ? one_ : zero_;
    if (!net) {
      net = SNLScalarNet::create(design_);
      net->setType(one ? SNLNet::Type::Assign1 : SNLNet::Type::Assign0);
    }
    return net;
  }
  std::optional<bool> constantValue(SNLBitNet* net) {
    if (net && net->isConstant0()) return false;
    if (net && net->isConstant1()) return true;
    return std::nullopt;
  }
  SNLBitNet* mux(SNLBitNet* condition, SNLBitNet* yes, SNLBitNet* no) {
    if (auto value = constantValue(condition)) return *value ? yes : no;
    if (yes == no) return yes;
    // An incompletely assigned variable remains unavailable on a later read.
    if (!yes || !no) return nullptr;
    auto* output = SNLScalarNet::create(design_);
    SNLRTLPrimitives::createMux(design_, condition, {no}, {yes}, output);
    return output;
  }
  SNLBitNet* gate(const std::string& op, SNLBitNet* a, SNLBitNet* b = nullptr) {
    using Gate = SNLRTLPrimitives::GateKind;
    static const std::map<std::string, Gate> kinds{{"and", Gate::And}, {"or", Gate::Or},
        {"xor", Gate::Xor}, {"nand", Gate::Nand}, {"nor", Gate::Nor},
        {"xnor", Gate::Xnor}, {"not", Gate::Not}};
    const auto kind = kinds.find(op);
    if (kind == kinds.end()) fail("unsupported RTL operator: " + op);
    const auto av = constantValue(a), bv = constantValue(b);
    if (av && (bv || op == "not")) {
      bool value = op == "not" ? !*av : op == "and" || op == "nand" ? *av && *bv :
          op == "or" || op == "nor" ? *av || *bv : *av != *bv;
      if (op == "nand" || op == "nor" || op == "xnor") value = !value;
      return constant(value);
    }
    auto* output = SNLScalarNet::create(design_);
    std::vector<SNLNet*> inputs{a};
    if (b) inputs.push_back(b);
    SNLRTLPrimitives::createGate(design_, kind->second, inputs, output);
    return output;
  }

  Value expression(const Expr& expr, const State& state, const Shape* expected = nullptr) {
    Value value;
    if (expr.kind == Expr::Kind::Name || expr.kind == Expr::Kind::Indexed) {
      auto selected = selection(expr);
      const auto& object = objects_.at(selected.name);
      if (object.output) fail("reading an out port is outside the RTL profile");
      const auto& bits = object.variable ? state.variables.at(selected.name) : object.value.bits;
      value = {selected.shape, Bits(bits.begin() + selected.offset,
          bits.begin() + selected.offset + selected.shape.size())};
      for (auto* bit : value.bits) {
        if (!bit) fail("variable read before definite assignment: " + selected.name);
        if (!object.variable) reads_.insert(bit);
      }
      if (sensitivity_ && !object.variable && !sensitivity_->contains(selected.name))
        fail("process sensitivity omits a signal read outside the clock guard: " + selected.name);
      if (sensitivity_ && object.variable) fail("variable read outside the clock guard is not supported");
    } else if (expr.kind == Expr::Kind::CharacterLiteral || expr.kind == Expr::Kind::StringLiteral ||
               expr.kind == Expr::Kind::Others) {
      if (!expected) fail("literal requires a constrained target type");
      value.shape = *expected;
      if (expr.kind == Expr::Kind::Others) {
        if (expected->ranges.empty()) fail("others aggregate requires an array target");
        auto element = *expected;
        element.ranges.erase(element.ranges.begin());
        element.types.erase(element.types.begin());
        const auto bits = expression(*expr.left, state, &element).bits;
        for (size_t i = 0; i < expected->ranges.front().size(); ++i)
          value.bits.insert(value.bits.end(), bits.begin(), bits.end());
      } else {
        const bool scalar = expr.kind == Expr::Kind::CharacterLiteral;
        if ((scalar && !expected->ranges.empty()) || (!scalar && expected->ranges.size() != 1) ||
            (expected->types.back() != "bit" && expected->types.back() != "std_logic"))
          fail("literal type does not match target");
        for (size_t i = 1; i + 1 < expr.text.size(); ++i) {
          const auto c = expr.text[i];
          if (c != '0' && c != '1') fail("RTL literals must contain only '0' and '1'");
          value.bits.push_back(constant(c == '1'));
        }
      }
    } else if (expr.kind == Expr::Kind::Unary) {
      value = expression(*expr.left, state, expected);
      if (expr.text != "not") fail("unsupported hardware unary operator");
      for (auto*& bit : value.bits) bit = gate("not", bit);
    } else if (expr.kind == Expr::Kind::Binary) {
      const bool comparison = expr.text == "=" || expr.text == "/=";
      auto left = expression(*expr.left, state, comparison ? nullptr : expected);
      auto right = expression(*expr.right, state, &left.shape);
      if (!compatible(left.shape, right.shape)) fail("binary operand type mismatch");
      if (comparison) {
        auto* equal = constant(true);
        for (size_t i = 0; i < left.bits.size(); ++i)
          equal = gate("and", equal, gate("xnor", left.bits[i], right.bits[i]));
        value = {{{"boolean"}, {}}, {expr.text == "=" ? equal : gate("not", equal)}};
      } else {
        if (left.shape.ranges.size() > 1) fail("bitwise operators on nested arrays are not supported");
        value.shape = left.shape;
        for (size_t i = 0; i < left.bits.size(); ++i)
          value.bits.push_back(gate(expr.text, left.bits[i], right.bits[i]));
      }
    } else fail("unsupported RTL expression");
    if (value.bits.size() != value.shape.size() || (expected && !compatible(value.shape, *expected)))
      fail("assignment or expression type/length mismatch");
    return value;
  }

  void drive(SNLBitNet* target, SNLBitNet* source) {
    if (!drivers_.insert(target).second) fail("multiple drivers for one signal bit");
    if (auto value = constantValue(source))
      target->setType(*value ? SNLNet::Type::Assign1 : SNLNet::Type::Assign0);
    else SNLRTLPrimitives::createGate(design_, SNLRTLPrimitives::GateKind::Buf, {source}, target);
  }
  void concurrent(const vhdl::Assignment& assignment, const State& state) {
    auto target = selection(assignment);
    const auto& object = objects_.at(target.name);
    if (object.input || object.variable || assignment.kind != vhdl::AssignmentKind::Signal)
      fail("concurrent assignment must drive a writable signal");
    auto value = expression(*assignment.value, state, &target.shape);
    for (size_t i = 0; i < value.bits.size(); ++i) drive(object.value.bits[target.offset + i], value.bits[i]);
  }

  void assign(const vhdl::Assignment& assignment, State& state) {
    auto target = selection(assignment);
    const auto& object = objects_.at(target.name);
    if (object.input || object.variable != (assignment.kind == vhdl::AssignmentKind::Variable))
      fail("assignment operator does not match a writable object");
    const auto value = expression(*assignment.value, state, &target.shape);
    auto& bits = object.variable ? state.variables.at(target.name) : state.scheduled.at(target.name);
    for (size_t i = 0; i < value.bits.size(); ++i) {
      bits[target.offset + i] = value.bits[i];
      if (!object.variable) state.written.at(target.name)[target.offset + i] = true;
    }
  }

  void merge(State& state, const State& yes, const State& no, SNLBitNet* condition) {
    for (auto& [name, bits] : state.variables)
      for (size_t i = 0; i < bits.size(); ++i)
        bits[i] = mux(condition, yes.variables.at(name)[i], no.variables.at(name)[i]);
    for (auto& [name, bits] : state.scheduled)
      for (size_t i = 0; i < bits.size(); ++i) {
        bits[i] = mux(condition, yes.scheduled.at(name)[i], no.scheduled.at(name)[i]);
        state.written.at(name)[i] = yes.written.at(name)[i] || no.written.at(name)[i];
      }
  }

  void statements(const std::vector<Statement>& statements, State& state) {
    for (const auto& statement : statements) {
      if (++steps_ > 100000) fail("static process elaboration limit exceeded");
      if (statement.kind == Statement::Kind::Assignment) assign(statement.assignment, state);
      else if (statement.kind == Statement::Kind::If) {
        const auto condition = expression(*statement.condition, state);
        if (condition.shape.types != std::vector<std::string>{"boolean"})
          fail("if condition must be boolean");
        auto yes = state, no = state;
        this->statements(statement.statements, yes);
        this->statements(statement.alternative, no);
        merge(state, yes, no, condition.bits.front());
      } else {
        const auto bounds = range(statement.range);
        const auto name = key(statement.iterator);
        if (integers_.contains(name) || objects_.contains(name))
          fail("shadowed loop parameters are not supported");
        for (size_t i = 0; i < bounds.size(); ++i) {
          integers_[name] = bounds.ascending ? bounds.left + static_cast<int64_t>(i)
                                            : bounds.left - static_cast<int64_t>(i);
          this->statements(statement.statements, state);
        }
        integers_.erase(name);
      }
    }
  }

  void lowerProcess(const vhdl::ClockedProcess& process) {
    const auto clockName = key(process.eventSignal);
    auto clock = selection(clockName);
    if (!objects_.at(clockName).input || clock.shape.size() != 1 ||
        !clock.shape.ranges.empty() || process.level != "'1'")
      fail("clock must name a scalar input with a positive edge");
    std::set<std::string> sensitivity;
    if (process.sensitivityList.empty()) sensitivity.insert(key(process.sensitivity));
    else for (const auto& name : process.sensitivityList) {
      selection(key(name));
      sensitivity.insert(key(name));
    }
    if (!sensitivity.contains(clockName)) fail("clock is absent from process sensitivity");
    if (key(process.levelSignal) != clockName) fail("clock event and level must match");
    for (const auto& variable : process.variables) {
      const auto type = shape(variable.type);
      for (const auto& name : variable.names) addObject(name, type, false, false, true);
    }
    State state;
    for (const auto& [name, object] : objects_) {
      if (object.variable) state.variables[name] = object.value.bits;
      else if (!object.input) {
        state.scheduled[name] = object.value.bits;
        state.written[name].resize(object.value.bits.size(), false);
      }
    }
    if (!process.sensitivityList.empty()) statements(process.statements, state);
    else {
      auto data = state;
      for (const auto& assignment : process.assignments) assign(assignment, data);
      if (process.enableSignal) {
        auto enable = control(*process.enableSignal, process.enableLevel, state);
        merge(state, data, state, enable);
      } else state = data;
      if (process.resetSignal) {
        auto reset = state;
        for (const auto& assignment : process.resetAssignments) assign(assignment, reset);
        auto select = control(*process.resetSignal, process.resetLevel, state);
        merge(state, reset, state, select);
      }
    }
    size_t writes = 0;
    for (const auto& [name, bits] : state.scheduled)
      for (size_t i = 0; i < bits.size(); ++i) if (state.written.at(name)[i]) {
        auto* target = objects_.at(name).value.bits[i];
        if (!drivers_.insert(target).second) fail("multiple drivers for clocked signal");
        SNLRTLPrimitives::createDFF(design_, objects_.at(clockName).value.bits.front(), bits[i], target);
        ++writes;
      }
    if (!writes) fail("clocked process does not write any signal");
    if (!process.sensitivityList.empty()) {
      sensitivity_ = &sensitivity;
      for (const auto& assignment : process.assignments) concurrent(assignment, state);
      sensitivity_ = nullptr;
    }
  }
  SNLBitNet* control(const vhdl::Name& name, const std::string& level, const State& state) {
    Expr expr;
    expr.kind = Expr::Kind::Name;
    expr.canonical = key(name);
    auto value = expression(expr, state);
    if (!value.shape.ranges.empty() || (level != "'0'" && level != "'1'"))
      fail("control must compare a scalar signal to '0' or '1'");
    return level == "'1'" ? value.bits.front() : gate("not", value.bits.front());
  }

  SNLDesign* design_;
  const vhdl::EntityDeclaration& entity_;
  const vhdl::ArchitectureBody& architecture_;
  std::map<std::string, int64_t> integers_;
  std::map<std::string, Shape> arrays_;
  std::map<std::string, size_t> arrayTypeOffsets_;
  std::map<std::string, Object> objects_;
  std::set<SNLBitNet*> drivers_, reads_;
  std::set<std::string> libraries_{"std", "work"};
  const std::set<std::string>* sensitivity_ = nullptr;
  bool stdLogic_ = false;
  size_t steps_ = 0;
  SNLBitNet* zero_ = nullptr;
  SNLBitNet* one_ = nullptr;
};
}

bool requiresVHDLRTL(const vhdl::DesignFile& syntax) {
  const auto extendedExpression = [](const auto& self, const Expr* expression) -> bool {
    return expression && (expression->kind == Expr::Kind::Indexed ||
        expression->kind == Expr::Kind::Others || self(self, expression->left.get()) ||
        self(self, expression->right.get()) || self(self, expression->condition.get()));
  };
  const auto extendedAssignment = [&](const vhdl::Assignment& assignment) {
    return !assignment.indices.empty() || extendedExpression(extendedExpression, assignment.value.get());
  };
  for (const auto& architecture : syntax.architectures) {
    for (const auto& assignment : architecture.assignments)
      if (extendedAssignment(assignment)) return true;
    for (const auto& process : architecture.processes) {
      if (!process.sensitivityList.empty()) return true;
      for (const auto& assignment : process.assignments)
        if (extendedAssignment(assignment)) return true;
      for (const auto& assignment : process.resetAssignments)
        if (extendedAssignment(assignment)) return true;
    }
  }
  return false;
}

SNLDesign* constructVHDLRTL(NLLibrary* library, const vhdl::DesignFile& syntax,
                          std::string_view top) {
  if (syntax.entities.size() != 1 || syntax.architectures.size() != 1 ||
      !syntax.architectures.front().instantiations.empty())
    fail("indexed RTL currently requires one entity and architecture without hierarchy");
  const auto& entity = syntax.entities.front();
  const auto& architecture = syntax.architectures.front();
  std::string canonicalTop(top);
  for (char& c : canonicalTop) c = std::tolower(static_cast<unsigned char>(c));
  if (key(architecture.entity) != key(entity.name) ||
      (!top.empty() && canonicalTop != key(entity.name))) fail("RTL entity/top mismatch");
  auto guard = std::unique_ptr<SNLDesign, void (*)(SNLDesign*)>(
      SNLDesign::create(library, NLName(entity.name.spelling)),
      [](SNLDesign* design) { design->destroy(); });
  RTLConstructor(guard.get(), entity, architecture).run();
  return guard.release();
}
}
