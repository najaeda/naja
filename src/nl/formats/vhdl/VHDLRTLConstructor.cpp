// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "VHDLRTLConstructor.h"

#include "NLException.h"
#include "NLDB0.h"
#include "SNLInstParameter.h"
#include "NLLibrary.h"
#include "NajaPrivateProperty.h"
#include "NLName.h"
#include "SNLBitNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLBusTermBit.h"
#include <functional>
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
  size_t integerWidth = 0;
  size_t size() const {
    if (integerWidth) return integerWidth;
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
  if (a.integerWidth && b.integerWidth) return true;
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
  bool constant = false;
};
struct Selection { std::string name; Shape shape; size_t offset = 0; };
struct State {
  std::map<std::string, Bits> variables;
  std::map<std::string, Bits> scheduled;
  std::map<std::string, std::vector<bool>> written;
};

class RTLSourceProperty final : public naja::NajaPrivateProperty {
 public:
  std::string signature;
  std::string getName() const override { return "VHDLRTLSource"; }
  std::string getString() const override { return getName(); }
  static void attach(SNLDesign* design, const std::string& signature) {
    auto* property = new RTLSourceProperty;
    property->signature = signature;
    property->postCreate(design);
  }
};

class RTLConstructor {
 public:
  RTLConstructor(SNLDesign* design, const vhdl::EntityDeclaration& entity,
                 const vhdl::ArchitectureBody& architecture,
                 const vhdl::DesignFile& syntax,
                 std::map<std::string, int64_t> generics,
                 std::function<SNLDesign*(const std::string&, const std::map<std::string, int64_t>&)> build):
      design_(design), entity_(entity), architecture_(architecture), syntax_(syntax),
      genericOverrides_(std::move(generics)), build_(std::move(build)) {}

  void run() {
    context(entity_.context);
    for (const auto& generic : entity_.generics) {
      const auto type = key(generic.type.name);
      if ((type != "integer" && type != "natural" && type != "positive") ||
          !generic.defaultValue)
        fail("RTL generics require a defaulted integer subtype");
      for (const auto& name : generic.names) {
        const auto value = genericOverrides_.contains(key(name)) ? genericOverrides_.at(key(name)) :
            integer(*generic.defaultValue);
        if (generic.type.constraint) range(*generic.type.constraint).position(value);
        if ((type == "positive" && value < 1) || (type == "natural" && value < 0))
          fail("generic value violates its subtype");
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
    std::set<std::string> localComponents;
    for (const auto& component : architecture_.components) {
      const auto name = key(component.name);
      if (!localComponents.insert(name).second) fail("duplicate local component: " + name);
      components_[name] = &component;
    }
    for (const auto& declaration : architecture_.arrayTypes) addArray(declaration);
    for (const auto& signal : architecture_.signals) {
      const auto type = shape(signal.type);
      for (const auto& name : signal.names) addObject(name, type);
      if (signal.initializer) {
        const auto value = expression(*signal.initializer, State{}, &type);
        for (const auto& name : signal.names) {
          const auto& bits = objects_.at(key(name)).value.bits;
          for (size_t i = 0; i < bits.size(); ++i) {
            const auto initial = constantValue(value.bits[i]);
            if (!initial) fail("signal initializer must be a constant binary value");
            initialValues_[bits[i]] = *initial;
          }
        }
      }
    }
    State empty;
    for (const auto& assignment : architecture_.assignments)
      concurrent(assignment, empty);
    for (const auto& generate : architecture_.generates) lowerGenerate(generate, empty);
    for (const auto& instance : architecture_.instantiations) lowerInstance(instance);
    for (const auto& process : architecture_.processes) lowerProcess(process);
    for (const auto& [bit, value] : initialValues_)
      if (!initializedFlops_.contains(bit)) fail("initialized signal must be driven by a local clocked process");
    for (const auto& [name, object] : objects_) {
      if (object.variable || object.input || object.constant) continue;
      for (auto* bit : object.value.bits)
        if ((object.output || reads_.contains(bit)) && !drivers_.contains(bit))
          fail("signal bit has no driver: " + name);
    }
  }

 private:
  void addArray(const vhdl::ArrayTypeDeclaration& declaration) {
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

  void lowerGenerate(const vhdl::GenerateStatement& generate, const State& state) {
    auto name = key(generate.iterator);
    if (integers_.contains(name) || objects_.contains(name)) fail("shadowed generate parameter");
    const auto bounds = range(generate.range);
    for (size_t i = 0; i < bounds.size(); ++i) {
      if (++steps_ > 100000) fail("static generate elaboration limit exceeded");
      integers_[name] = bounds.ascending ? bounds.left + i : bounds.left - i;
      for (const auto& assignment : generate.assignments) concurrent(assignment, state);
      for (const auto& child : generate.generates) lowerGenerate(child, state);
    }
    integers_.erase(name);
  }

  void lowerInstance(const vhdl::EntityInstantiation& instance) {
    if (key(instance.library) != "work" || instance.architecture)
      fail("unsupported RTL entity binding");
    const vhdl::EntityDeclaration* entity = nullptr;
    for (const auto& candidate : syntax_.entities)
      if (key(candidate.name) == key(instance.entity)) entity = &candidate;
    if (!entity) fail("missing entity: " + key(instance.entity));
    if (instance.component && !components_.contains(key(instance.entity)))
      fail("component is not visible: " + key(instance.entity));
    const auto& interface = instance.component ? *components_.at(key(instance.entity)) : *entity;
    std::vector<std::string> genericNames;
    for (const auto& generic : interface.generics)
      for (const auto& name : generic.names) genericNames.push_back(key(name));
    std::map<std::string, int64_t> overrides;
    bool named = false;
    for (size_t i = 0; i < instance.generics.size(); ++i) {
      const auto& association = instance.generics[i];
      if (i >= genericNames.size()) fail("too many generic actuals");
      if (!association.formal && named) fail("positional generic follows named generic");
      named |= association.formal.has_value();
      const auto name = association.formal ? key(*association.formal) : genericNames[i];
      if (std::find(genericNames.begin(), genericNames.end(), name) == genericNames.end() ||
          !overrides.emplace(name, integer(*association.actual)).second) fail("invalid generic association");
    }
    const auto parentIntegers = integers_;
    for (const auto& generic : entity->generics)
      for (const auto& name : generic.names) {
        if (!generic.defaultValue && !overrides.contains(key(name))) fail("missing generic value");
        integers_[key(name)] = overrides.contains(key(name)) ? overrides.at(key(name)) : integer(*generic.defaultValue);
      }
    std::map<std::string, Shape> formalShapes;
    for (const auto& port : entity->ports)
      for (const auto& name : port.names) formalShapes.emplace(key(name), shape(port.type));
    if (instance.component) {
      const auto& component = *components_.at(key(instance.entity));
      std::map<std::string, vhdl::PortMode> modes;
      for (const auto& port : entity->ports)
        for (const auto& name : port.names) modes.emplace(key(name), port.mode);
      std::set<std::string> declaredPorts;
      for (const auto& port : component.ports) for (const auto& name : port.names) {
        const auto id = key(name);
        if (!declaredPorts.insert(id).second || !modes.contains(id) || modes.at(id) != port.mode ||
            !compatible(shape(port.type), formalShapes.at(id))) fail("component port interface mismatch");
      }
      if (declaredPorts.size() != modes.size() || component.generics.size() != entity->generics.size())
        fail("component declaration does not match entity interface");
      for (size_t i = 0; i < component.generics.size(); ++i) {
        const auto& declared = component.generics[i];
        const auto& actual = entity->generics[i];
        if (declared.names.size() != actual.names.size() || key(declared.type.name) != key(actual.type.name))
          fail("component generic interface mismatch");
        for (size_t j = 0; j < declared.names.size(); ++j)
          if (key(declared.names[j]) != key(actual.names[j])) fail("component generic name mismatch");
      }
    }
    integers_ = parentIntegers;
    auto* model = build_(key(instance.entity), overrides);
    std::vector<SNLTerm*> ports;
    for (const auto& declaration : interface.ports) for (const auto& name : declaration.names) {
      SNLTerm* matching = nullptr;
      for (auto* term : model->getTerms()) {
        std::string id = term->getName().getString();
        for (auto& c : id) c = std::tolower(static_cast<unsigned char>(c));
        if (id == key(name)) matching = term;
      }
      if (!matching) fail("missing bound port");
      ports.push_back(matching);
    }
    if (ports.size() != instance.actuals.size()) fail("port association count mismatch");
    auto* child = SNLInstance::create(design_, model, NLName(instance.label.spelling));
    std::set<SNLTerm*> bound;
    named = false;
    for (size_t i = 0; i < ports.size(); ++i) {
      auto* port = ports[i];
      if (!instance.formals.empty() && instance.formals[i]) {
        named = true;
        port = nullptr;
        for (auto* candidate : ports) {
          std::string name = candidate->getName().getString();
          for (auto& c : name) c = std::tolower(static_cast<unsigned char>(c));
          if (name == key(*instance.formals[i])) port = candidate;
        }
      } else if (named) fail("positional port follows named port");
      if (!port || !bound.insert(port).second) fail("invalid port association");
      const auto selected = selection(key(instance.actuals[i]));
      const auto& object = objects_.at(selected.name);
      std::string formalName = port->getName().getString();
      for (auto& c : formalName) c = std::tolower(static_cast<unsigned char>(c));
      if (object.value.bits.size() != port->getWidth() || !compatible(object.value.shape, formalShapes.at(formalName)))
        fail("port type or width mismatch");
      size_t bit = 0;
      for (auto* term : port->getBits()) {
        auto* net = object.value.bits[bit++];
        if (port->getDirection() == SNLTerm::Direction::Output) {
          if (object.input || object.constant || !drivers_.insert(net).second)
            fail("invalid component output driver");
        } else reads_.insert(net);
        child->getInstTerm(term)->setNet(net);
      }
    }
  }

  void context(const vhdl::ContextClause& clauses) {
    for (const auto& library : clauses.libraries)
      for (const auto& name : library.names) libraries_.insert(key(name));
    for (const auto& use : clauses.uses) {
      if (use.selectedName.size() == 3 && key(use.selectedName[0]) == "work" &&
          key(use.selectedName[2]) == "all") {
        const auto name = key(use.selectedName[1]);
        if (!imported_.insert(name).second) continue;
        const vhdl::PackageDeclaration* package = nullptr;
        for (const auto& candidate : syntax_.packages)
          if (!candidate.body && key(candidate.name) == name) package = &candidate;
        if (!package) fail("missing package: " + name);
        const auto callerLibraries = libraries_;
        const bool callerLogic = stdLogic_, callerUnsigned = unsigned_, callerArith = arith_, callerNumeric = numeric_;
        context(package->context);
        for (const auto& type : package->arrayTypes) addArray(type);
        for (const auto& declaration : package->constants) {
          const auto type = shape(declaration.object.type);
          const auto value = expression(*declaration.value, State{}, &type);
          for (const auto& id : declaration.object.names) {
            if (!objects_.emplace(key(id), Object{value, false, false, false, true}).second)
              fail("duplicate package constant");
          }
        }
        for (const auto& component : package->components)
          if (!components_.emplace(key(component.name), &component).second) fail("ambiguous component");
        libraries_ = callerLibraries;
        stdLogic_ = callerLogic;
        unsigned_ = callerUnsigned;
        arith_ = callerArith;
        numeric_ = callerNumeric;
        continue;
      }
      if (use.selectedName.size() != 3 || key(use.selectedName[0]) != "ieee" ||
          key(use.selectedName[2]) != "all" || !libraries_.contains("ieee"))
        fail("unsupported RTL package visibility");
      const auto package = key(use.selectedName[1]);
      if (package == "std_logic_1164") stdLogic_ = true;
      else if (package == "std_logic_unsigned") unsigned_ = true;
      else if (package == "std_logic_arith") arith_ = true;
      else if (package == "numeric_std") numeric_ = true;
      else
        fail("unsupported IEEE package: " + package);
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
    if (name == "integer" || name == "natural" || name == "positive") {
      if (!type.constraint) fail("hardware integer requires a nonnegative range");
      const auto bounds = range(*type.constraint);
      if (!bounds.ascending || bounds.left < 0 || bounds.right < bounds.left || bounds.right > INT32_MAX)
        fail("unsupported hardware integer range");
      size_t width = 1;
      while ((uint64_t(1) << width) <= uint64_t(bounds.right)) ++width;
      return {{"integer"}, {}, width};
    }
    if (name == "bit" || (stdLogic_ && name == "std_logic")) {
      if (type.constraint) fail("scalar type cannot have a range");
      return {{name}, {}};
    }
    if (name == "bit_vector" || (stdLogic_ && name == "std_logic_vector") ||
        (numeric_ && name == "unsigned")) {
      if (!type.constraint) fail("vector requires a constraint");
      return {{name, name == "bit_vector" ? "bit" : "std_logic"}, {range(*type.constraint)}};
    }
    fail("unsupported or invisible RTL type: " + name);
  }

  void addObject(const vhdl::Name& name, const Shape& type, bool input = false,
                 bool output = false, bool variable = false, bool port = false) {
    const auto id = key(name);
    if (port && (type.ranges.size() > 1 || type.integerWidth))
      fail("RTL ports require scalar logic or one-dimensional logic vectors");
    if (objects_.contains(id) || integers_.contains(id) || arrays_.contains(id))
      fail("duplicate or shadowing object: " + id);
    Bits bits;
    const auto size = type.size();
    if (variable) bits.resize(size, nullptr);
    else if (type.ranges.size() == 1 || type.integerWidth) {
      const auto r = type.integerWidth ? Range{int64_t(size - 1), 0, false} : type.ranges.front();
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
    if (expression.kind == Expr::Kind::Range) {
      const auto& original = selected.shape.ranges.front();
      Range slice{integer(*expression.left), integer(*expression.right), expression.text == "to"};
      if (slice.ascending != original.ascending || !slice.size()) fail("invalid slice direction or range");
      const auto position = original.position(slice.left);
      original.position(slice.right);
      const auto stride = selected.shape.size() / original.size();
      selected.offset += position * stride;
      selected.shape.ranges.front() = slice;
      return;
    }
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

  Bits add(const Bits& left, const Bits& right, bool subtract) {
    Bits result(left.size());
    auto* carry = constant(subtract);
    for (size_t i = left.size(); i; --i) {
      auto* a = left[i - 1];
      auto* b = subtract ? gate("not", right[i - 1]) : right[i - 1];
      auto* ab = gate("xor", a, b);
      result[i - 1] = gate("xor", ab, carry);
      carry = gate("or", gate("and", a, b), gate("and", ab, carry));
    }
    return result;
  }

  bool isStatic(const Expr& expr) const {
    if (expr.kind == Expr::Kind::IntegerLiteral) return true;
    if (expr.kind == Expr::Kind::Name) return integers_.contains(key(expr));
    return (expr.kind == Expr::Kind::Unary || expr.kind == Expr::Kind::Binary) &&
        (expr.text == "+" || expr.text == "-" || expr.text == "*") &&
        isStatic(*expr.left) && (!expr.right || isStatic(*expr.right));
  }
  Value number(int64_t number, const Shape& type) {
    Value value{type, {}};
    if (number < 0) fail("negative hardware integers are not supported");
    for (size_t i = type.size(); i; --i)
      value.bits.push_back(constant(i <= 64 && ((uint64_t(number) >> (i - 1)) & 1)));
    return value;
  }
  Bits resize(Bits bits, size_t width) {
    if (bits.size() > width) bits.erase(bits.begin(), bits.end() - width);
    else bits.insert(bits.begin(), width - bits.size(), constant(false));
    return bits;
  }
  SNLBitNet* equal(const Bits& a, const Bits& b) {
    if (a.size() != b.size()) fail("internal comparison width mismatch");
    auto* result = constant(true);
    for (size_t i = 0; i < a.size(); ++i)
      result = gate("and", result, gate("xnor", a[i], b[i]));
    return result;
  }
  Shape vectorShape(size_t width, const std::string& scalar = "std_logic") {
    if (!width) fail("empty vector");
    return {{scalar == "bit" ? "bit_vector" : "std_logic_vector", scalar},
            {{int64_t(width - 1), 0, false}}};
  }
  Value readObject(const std::string& name, const State& state) {
    const auto& object = objects_.at(name);
    if (object.output) fail("reading an out port is outside the RTL profile");
    Value value = object.value;
    if (object.variable) value.bits = state.variables.at(name);
    for (auto* bit : value.bits) {
      if (!bit) fail("variable read before definite assignment: " + name);

    }
    if (sensitivity_ && !object.constant && !object.variable && !sensitivity_->contains(name))
      fail("process sensitivity omits a signal read outside the clock guard: " + name);
    if (sensitivity_ && object.variable) fail("variable read outside the clock guard is not supported");
    return value;
  }
  Value readIndex(Value value, const Expr& indexExpr, const State& state) {
    if (value.shape.ranges.empty()) fail("cannot index a scalar object");
    const auto bounds = value.shape.ranges.front();
    const auto stride = value.shape.size() / bounds.size();
    if (indexExpr.kind == Expr::Kind::Range) {
      Range slice{integer(*indexExpr.left), integer(*indexExpr.right), indexExpr.text == "to"};
      if (!slice.size() || slice.ascending != bounds.ascending) fail("invalid slice range");
      const auto offset = bounds.position(slice.left) * stride;
      bounds.position(slice.right);
      value.shape.ranges.front() = slice;
      value.bits = Bits(value.bits.begin() + offset, value.bits.begin() + offset + value.shape.size());
      return value;
    }
    value.shape.ranges.erase(value.shape.ranges.begin());
    value.shape.types.erase(value.shape.types.begin());
    if (isStatic(indexExpr)) {
      const auto offset = bounds.position(integer(indexExpr)) * stride;
      value.bits = Bits(value.bits.begin() + offset, value.bits.begin() + offset + stride);
      return value;
    }
    for (auto* bit : value.bits) reads_.insert(bit);
    auto address = expression(indexExpr, state);
    if (!address.shape.integerWidth) fail("array index must have integer type");
    Bits result(stride, constant(false));
    for (size_t i = 0; i < bounds.size(); ++i) {
      auto index = bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i);
      if (index < 0) fail("dynamic indexing of negative bounds is unsupported");
      const auto width = std::max<size_t>(address.bits.size(), 32);
      auto* select = equal(resize(address.bits, width), number(index, {{"integer"}, {}, width}).bits);
      for (size_t j = 0; j < stride; ++j)
        result[j] = mux(select, value.bits[i * stride + j], result[j]);
    }
    value.bits = std::move(result);
    return value;
  }

  Value readSelected(const Expr& expr, const State& state) {
    if (expr.kind == Expr::Kind::Name) {
      if (!objects_.contains(key(expr))) fail("no declaration for object: " + key(expr));
      return readObject(key(expr), state);
    }
    if (expr.kind != Expr::Kind::Indexed) fail("indexed prefix must name an object");
    return readIndex(readSelected(*expr.left, state), *expr.right, state);
  }

  Value expression(const Expr& expr, const State& state, const Shape* expected = nullptr) {
    Value value;
    Shape inferred;
    if (isStatic(expr)) {
      const Shape type = expected ? *expected : Shape{{"integer"}, {}, 32};
      if (!type.integerWidth && !(unsigned_ && type.types.front() == "std_logic_vector"))
        fail("integer literal is incompatible with target");
      value = number(integer(expr), type);
    } else if (expr.kind == Expr::Kind::Name) {
      const auto name = key(expr);
      if (!objects_.contains(name)) fail("no declaration for object: " + name);
      value = readObject(name, state);
      for (auto* bit : value.bits) reads_.insert(bit);
    } else if (expr.kind == Expr::Kind::Indexed) {
      if (expr.left->kind == Expr::Kind::Name && key(*expr.left) == "conv_integer") {
        if (!unsigned_ || !arith_) fail("conv_integer requires std_logic_arith and std_logic_unsigned");
        value = expression(*expr.right, state);
        if (value.shape.types.front() != "std_logic_vector" || value.bits.size() > 31)
          fail("unsupported conv_integer argument");
        value.shape = {{"integer"}, {}, value.bits.size()};
      } else value = readSelected(expr, state);
      for (auto* bit : value.bits) reads_.insert(bit);
    } else if (expr.kind == Expr::Kind::Conditional) {
      const auto condition = expression(*expr.condition, state);
      if (condition.shape.types != std::vector<std::string>{"boolean"}) fail("condition must be boolean");
      auto yes = expression(*expr.left, state, expected);
      auto no = expression(*expr.right, state, &yes.shape);
      if (!compatible(yes.shape, no.shape)) fail("conditional type mismatch");
      value.shape = yes.shape;
      for (size_t i = 0; i < yes.bits.size(); ++i)
        value.bits.push_back(mux(condition.bits.front(), yes.bits[i], no.bits[i]));
    } else if (expr.kind == Expr::Kind::Aggregate) {
      if (!expected || expected->ranges.empty() || expr.elements.size() != expected->ranges.front().size())
        fail("positional aggregate length mismatch");
      value.shape = *expected;
      auto element = *expected;
      element.types.erase(element.types.begin());
      element.ranges.erase(element.ranges.begin());
      for (const auto& item : expr.elements) {
        const auto part = expression(*item, state, &element);
        value.bits.insert(value.bits.end(), part.bits.begin(), part.bits.end());
      }
    } else if (expr.kind == Expr::Kind::CharacterLiteral || expr.kind == Expr::Kind::StringLiteral ||
               expr.kind == Expr::Kind::Others || expr.kind == Expr::Kind::BitStringLiteral) {
      if (!expected && expr.kind != Expr::Kind::Others) {
        const auto digits = expr.kind == Expr::Kind::BitStringLiteral ?
            std::count_if(expr.text.begin() + 1, expr.text.end() - 1, [](char c) { return c != '_'; }) :
            expr.text.size() - 2;
        const auto width = digits * (expr.kind == Expr::Kind::BitStringLiteral ? (expr.canonical == "x" ? 4 : expr.canonical == "o" ? 3 : 1) : 1);
        inferred = expr.kind == Expr::Kind::CharacterLiteral ? Shape{{"std_logic"}, {}} : vectorShape(width);
        expected = &inferred;
      }
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
          if (expr.kind == Expr::Kind::BitStringLiteral) {
            if (c == '_') continue;
            const int digit = c >= '0' && c <= '9' ? c - '0' :
                std::tolower(static_cast<unsigned char>(c)) - 'a' + 10;
            const int width = expr.canonical == "x" ? 4 : expr.canonical == "o" ? 3 : 1;
            if (digit < 0 || digit >= (1 << width)) fail("invalid bit string digit");
            for (int bit = width - 1; bit >= 0; --bit) value.bits.push_back(constant((digit >> bit) & 1));
          } else {
            if (c != '0' && c != '1') fail("RTL literals must contain only '0' and '1'");
            value.bits.push_back(constant(c == '1'));
          }
        }
      }
    } else if (expr.kind == Expr::Kind::Unary) {
      value = expression(*expr.left, state, expected);
      if (expr.text != "not" || value.shape.integerWidth)
        fail("unsupported hardware unary operator");
      for (auto*& bit : value.bits) bit = gate("not", bit);
    } else if (expr.kind == Expr::Kind::Binary) {
      const bool comparison = expr.text == "=" || expr.text == "/=";
      const bool arithmetic = expr.text == "+" || expr.text == "-";
      if (expr.text == "&") {
        auto left = expression(*expr.left, state);
        auto right = expression(*expr.right, state);
        if (left.shape.integerWidth || right.shape.integerWidth || left.shape.ranges.size() > 1 ||
            right.shape.ranges.size() > 1 || left.shape.types.back() != right.shape.types.back())
          fail("unsupported concatenation operands");
        value.bits = left.bits;
        value.bits.insert(value.bits.end(), right.bits.begin(), right.bits.end());
        value.shape = vectorShape(value.bits.size(), left.shape.types.back());
        if (left.shape.types.front() == "unsigned" || right.shape.types.front() == "unsigned") {
          if ((left.shape.ranges.size() && left.shape.types.front() != "unsigned") ||
              (right.shape.ranges.size() && right.shape.types.front() != "unsigned"))
            fail("concatenation operand type mismatch");
          value.shape.types.front() = "unsigned";
        }
      } else {
        const bool numericOperation = numeric_ && (arithmetic || expr.text == "*" || comparison);
        auto left = expression(*expr.left, state, numericOperation || comparison ? nullptr : expected);
        const bool unsignedOperands = numeric_ && left.shape.types.front() == "unsigned";
        const bool rightLiteral = expr.right->kind == Expr::Kind::StringLiteral ||
            expr.right->kind == Expr::Kind::BitStringLiteral || expr.right->kind == Expr::Kind::Others;
        auto right = expression(*expr.right, state,
            (unsignedOperands && !rightLiteral) || (comparison && left.shape.integerWidth) ? nullptr : &left.shape);
        if (unsignedOperands && (arithmetic || comparison || expr.text == "*")) {
          if (right.shape.types.front() != "unsigned") fail("unsigned operand type mismatch");
          const auto width = expr.text == "*" ? left.bits.size() + right.bits.size() :
              std::max(left.bits.size(), right.bits.size());
          left.bits = resize(left.bits, width);
          right.bits = resize(right.bits, width);
          value.shape = vectorShape(width);
          value.shape.types.front() = "unsigned";
          if (comparison) {
            auto* result = equal(left.bits, right.bits);
            value = {{{"boolean"}, {}}, {expr.text == "=" ? result : gate("not", result)}};
          } else if (expr.text == "*") {
            value.bits = Bits(width, constant(false));
            for (size_t shift = 0; shift < width; ++shift) {
              Bits row(width, constant(false));
              for (size_t i = shift; i < width; ++i)
                row[width - 1 - i] = gate("and", left.bits[width - 1 - i + shift], right.bits[width - 1 - shift]);
              value.bits = add(value.bits, row, false);
            }
          } else value.bits = add(left.bits, right.bits, expr.text == "-");
          if (expected && !compatible(value.shape, *expected))
            fail("assignment or expression type/length mismatch");
          return value;
        }
        if (comparison && left.shape.integerWidth && right.shape.integerWidth) {
          const auto width = std::max(left.bits.size(), right.bits.size());
          left.bits = resize(left.bits, width);
          right.bits = resize(right.bits, width);
        }
        if (!compatible(left.shape, right.shape)) fail("binary operand type mismatch");
        if (comparison) {
          auto* result = equal(left.bits, right.bits);
          value = {{{"boolean"}, {}}, {expr.text == "=" ? result : gate("not", result)}};
        } else if (arithmetic) {
          if (!left.shape.integerWidth && !(unsigned_ && left.shape.types.front() == "std_logic_vector"))
            fail("arithmetic requires integer or std_logic_unsigned operands");
          auto* carry = constant(expr.text == "-");
          value.shape = left.shape;
          value.bits.resize(left.bits.size());
          for (size_t i = left.bits.size(); i; --i) {
            auto* a = left.bits[i - 1];
            auto* b = expr.text == "-" ? gate("not", right.bits[i - 1]) : right.bits[i - 1];
            auto* ab = gate("xor", a, b);
            value.bits[i - 1] = gate("xor", ab, carry);
            carry = gate("or", gate("and", a, b), gate("and", ab, carry));
          }
        } else {
          if (left.shape.ranges.size() > 1 || left.shape.integerWidth)
            fail("bitwise operators require scalar logic, boolean or logic vectors");
          value.shape = left.shape;
          for (size_t i = 0; i < left.bits.size(); ++i)
            value.bits.push_back(gate(expr.text, left.bits[i], right.bits[i]));
        }
      }
    } else fail("unsupported RTL expression");
    if (expected && expected->integerWidth && value.shape.integerWidth) {
      value.bits = resize(value.bits, expected->integerWidth);
      value.shape = *expected;
    }
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
    if (object.input || object.constant || object.variable || assignment.kind != vhdl::AssignmentKind::Signal)
      fail("concurrent assignment must drive a writable signal");
    auto value = expression(*assignment.value, state, &target.shape);
    for (size_t i = 0; i < value.bits.size(); ++i) drive(object.value.bits[target.offset + i], value.bits[i]);
  }

  void assign(const vhdl::Assignment& assignment, State& state) {
    if (!assignment.indices.empty() && assignment.indices.front()->kind != Expr::Kind::Range &&
        !isStatic(*assignment.indices.front())) {
      const auto name = key(assignment.target);
      const auto base = selection(name);
      if (base.shape.ranges.empty()) fail("dynamic target is not an array");
      const auto& object = objects_.at(name);
      if (object.input || object.constant || object.variable || assignment.kind != vhdl::AssignmentKind::Signal)
        fail("dynamic assignment requires a writable signal array");
      const auto bounds = base.shape.ranges.front();
      auto element = base.shape;
      element.ranges.erase(element.ranges.begin());
      element.types.erase(element.types.begin());
      const auto stride = element.size();
      const auto address = expression(*assignment.indices.front(), state);
      if (!address.shape.integerWidth) fail("array index must be integer");
      Selection selected{name, element, 0};
      for (size_t j = 1; j < assignment.indices.size(); ++j) index(selected, *assignment.indices[j]);
      const auto value = expression(*assignment.value, state, &selected.shape);
      auto& bits = state.scheduled.at(name);
      for (size_t i = 0; i < bounds.size(); ++i) {
        const auto indexValue = bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i);
        const auto width = std::max<size_t>(32, address.bits.size());
        auto* condition = equal(resize(address.bits, width), number(indexValue, {{"integer"}, {}, width}).bits);
        for (size_t j = 0; j < value.bits.size(); ++j) {
          const auto offset = i * stride + selected.offset + j;
          bits[offset] = mux(condition, value.bits[j], bits[offset]);
          state.written.at(name)[offset] = true;
        }
      }
      return;
    }
    auto target = selection(assignment);
    const auto& object = objects_.at(target.name);
    if (object.input || object.constant || object.variable != (assignment.kind == vhdl::AssignmentKind::Variable))
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
        if (auto selected = constantValue(condition.bits.front())) {
          this->statements(*selected ? statement.statements : statement.alternative, state);
          continue;
        }
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
      else if (!object.input && !object.constant) {
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
        auto* flop = SNLRTLPrimitives::createDFF(design_, objects_.at(clockName).value.bits.front(), bits[i], target);
        if (const auto initial = initialValues_.find(target); initial != initialValues_.end()) {
          SNLInstParameter::create(flop, flop->getModel()->getParameter(NLName("INIT")),
              NLDB0::formatDFFInitValue(1, initial->second ? "1" : "0"));
          initializedFlops_.insert(target);
        }
        ++writes;
      }
    if (!writes) fail("clocked process does not write any signal");
    if (!process.sensitivityList.empty()) {
      sensitivity_ = &sensitivity;
      for (const auto& assignment : process.assignments) concurrent(assignment, state);
      sensitivity_ = nullptr;
    }
    for (const auto& variable : process.variables)
      for (const auto& name : variable.names) objects_.erase(key(name));
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
  const vhdl::DesignFile& syntax_;
  std::map<std::string, int64_t> genericOverrides_;
  std::function<SNLDesign*(const std::string&, const std::map<std::string, int64_t>&)> build_;
  std::set<std::string> imported_;
  std::map<std::string, const vhdl::EntityDeclaration*> components_;
  bool unsigned_ = false, arith_ = false, numeric_ = false;
  std::map<std::string, int64_t> integers_;
  std::map<std::string, Shape> arrays_;
  std::map<std::string, size_t> arrayTypeOffsets_;
  std::map<std::string, Object> objects_;
  std::set<SNLBitNet*> drivers_, reads_;
  std::map<SNLBitNet*, bool> initialValues_;
  std::set<SNLBitNet*> initializedFlops_;
  std::set<std::string> libraries_{"std", "work"};
  const std::set<std::string>* sensitivity_ = nullptr;
  bool stdLogic_ = false;
  size_t steps_ = 0;
  SNLBitNet* zero_ = nullptr;
  SNLBitNet* one_ = nullptr;
};
}

bool requiresVHDLRTL(const vhdl::DesignFile& syntax) {
  if (!syntax.packages.empty()) return true;
  for (const auto& entity : syntax.entities) {
    for (const auto& generic : entity.generics) if (generic.type.constraint) return true;
    for (const auto& port : entity.ports) if (key(port.type.name) == "unsigned") return true;
  }
  const auto extendedExpression = [](const auto& self, const Expr* expression) -> bool {
    return expression && (expression->kind == Expr::Kind::Indexed ||
        expression->kind == Expr::Kind::Others || expression->kind == Expr::Kind::Aggregate ||
        expression->kind == Expr::Kind::BitStringLiteral || self(self, expression->left.get()) ||
        self(self, expression->right.get()) || self(self, expression->condition.get()));
  };
  const auto extendedAssignment = [&](const vhdl::Assignment& assignment) {
    return !assignment.indices.empty() || extendedExpression(extendedExpression, assignment.value.get());
  };
  for (const auto& architecture : syntax.architectures) {
    if (!architecture.generates.empty() || !architecture.components.empty()) return true;
    for (const auto& signal : architecture.signals) if (signal.initializer || key(signal.type.name) == "unsigned") return true;
    for (const auto& instance : architecture.instantiations)
      if (instance.component || std::any_of(instance.formals.begin(), instance.formals.end(),
          [](const auto& formal) { return formal.has_value(); })) return true;
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
                          std::string_view top, std::string_view source) {
  std::set<std::string> packages, bodies;
  for (const auto& package : syntax.packages) {
    const auto name = key(package.name);
    if (!package.body && !packages.insert(name).second) fail("duplicate package");
    if (package.body && (!packages.contains(name) || !bodies.insert(name).second))
      fail("duplicate package body or body without declaration");
  }
  if (syntax.entities.empty() && syntax.architectures.empty()) {
    if (!top.empty()) fail("package source does not define a top entity");
    return nullptr;
  }
  std::string selected(top);
  for (char& c : selected) c = std::tolower(static_cast<unsigned char>(c));
  if (selected.empty()) {
    if (syntax.entities.size() != 1) fail("multiple RTL entities require an explicit top");
    selected = key(syntax.entities.front().name);
  }
  std::vector<SNLDesign*> created;
  std::set<std::string> active;
  std::function<SNLDesign*(const std::string&, const std::map<std::string, int64_t>&)> build;
  build = [&](const std::string& name, const std::map<std::string, int64_t>& overrides) -> SNLDesign* {
    if (!active.insert(name).second) fail("recursive RTL hierarchy");
    const vhdl::EntityDeclaration* entity = nullptr;
    const vhdl::ArchitectureBody* architecture = nullptr;
    for (const auto& candidate : syntax.entities) if (key(candidate.name) == name) {
      if (entity) fail("duplicate RTL entity");
      entity = &candidate;
    }
    for (const auto& candidate : syntax.architectures) if (key(candidate.entity) == name) {
      if (architecture) fail("multiple RTL architectures are unsupported");
      architecture = &candidate;
    }
    if (!entity || !architecture) fail("missing RTL entity or architecture: " + name);
    std::string modelName = entity->name.spelling;
    for (const auto& [generic, value] : overrides) modelName += "__" + generic + "_" + std::to_string(value);
    const auto signature = std::string(source.substr(0,
        std::max(entity->span.end.offset, architecture->span.end.offset))) + modelName;
    if (auto* existing = library->getSNLDesign(NLName(modelName))) {
      auto* property = dynamic_cast<RTLSourceProperty*>(existing->getProperty("VHDLRTLSource"));
      if (!property || property->signature != signature) fail("conflicting RTL design: " + modelName);
      active.erase(name);
      return existing;
    }
    auto* design = SNLDesign::create(library, NLName(modelName));
    created.push_back(design);
    RTLConstructor(design, *entity, *architecture, syntax, overrides, build).run();
    RTLSourceProperty::attach(design, signature);
    active.erase(name);
    return design;
  };
  try {
    return build(selected, {});
  } catch (...) {
    for (auto* design : created) design->destroy();
    throw;
  }
}
}
