// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "VHDLRTLConstructor.h"

#include "NLException.h"
#include "NLDB0.h"
#include "SNLHDLLibrary.h"
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

#include <boost/multiprecision/cpp_int.hpp>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <utility>

namespace naja::NL {
namespace {
using boost::multiprecision::int128_t;
using Expr = vhdl::Expression;
using Statement = vhdl::SequentialStatement;
using Bits = std::vector<SNLBitNet*>;

// Nested lowering restores the enclosing location, including on exceptions.
// Thread-local storage keeps independent constructors' diagnostics isolated.
thread_local vhdl::SourceSpan diagnosticSpan;
class DiagnosticScope {
 public:
  explicit DiagnosticScope(const vhdl::SourceSpan& span): previous_(diagnosticSpan) {
    diagnosticSpan = span;
  }
  ~DiagnosticScope() { diagnosticSpan = previous_; }
 private:
  vhdl::SourceSpan previous_;
};

[[noreturn]] void fail(const std::string& message) {
  throw VHDLRTLException(message, diagnosticSpan);
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
  const vhdl::EnumerationTypeDeclaration* indexEnumeration = nullptr;
  size_t size() const {
    if (ascending ? left > right : left < right) return 0;
    const auto width = ascending ? int128_t(right) - left + 1 : int128_t(left) - right + 1;
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
  std::vector<std::pair<std::string, Shape>> fields;
  const vhdl::EnumerationTypeDeclaration* enumeration = nullptr;
  // Identity of each user-defined array/record dimension, independent of spelling.
  std::vector<const void*> declarations;
  void removeOuterType() {
    types.erase(types.begin());
    if (!declarations.empty()) declarations.erase(declarations.begin());
  }
  size_t size() const {
    if (integerWidth) return integerWidth;
    size_t size = 1;
    if (enumeration) while ((size_t(1) << size) < enumeration->literals.size()) ++size;
    if (!fields.empty()) {
      size = 0;
      for (const auto& field : fields) {
        const auto width = field.second.size();
        if (size > 65536 - width) fail("oversized hardware record");
        size += width;
      }
    }
    for (const auto& range : ranges) {
      const auto width = range.size();
      if (!width || size > 65536 / width) fail("null or oversized hardware array");
      size *= width;
    }
    return size;
  }
};

bool compatible(const Shape& a, const Shape& b) {
  if (a.enumeration != b.enumeration) return false;
  if (a.integerWidth && b.integerWidth) return true;
  if (a.types != b.types || a.ranges.size() != b.ranges.size()) return false;
  for (size_t i = 0; i < std::max(a.declarations.size(), b.declarations.size()); ++i)
    if ((i < a.declarations.size() ? a.declarations[i] : nullptr) !=
        (i < b.declarations.size() ? b.declarations[i] : nullptr)) return false;
  if (a.fields.size() != b.fields.size()) return false;
  for (size_t i = 0; i < a.fields.size(); ++i)
    if (a.fields[i].first != b.fields[i].first || !compatible(a.fields[i].second, b.fields[i].second)) return false;
  for (size_t i = 0; i < a.ranges.size(); ++i)
    if (a.ranges[i].size() != b.ranges[i].size() ||
        a.ranges[i].indexEnumeration != b.ranges[i].indexEnumeration) return false;
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
struct MemoryWrite {
  Bits address, data;
  SNLBitNet* enable = nullptr;
  bool assigned = false;
};
struct State {
  std::map<std::string, MemoryWrite> memoryWrites;
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

class LibraryContext {
 public:
  NLLibrary* destination;
  const std::vector<VHDLLibrarySource>& sources;
  NLLibrary* owner(size_t offset) const {
    for (const auto& source : sources)
      if (offset >= source.offset && offset < source.offset + source.size) return source.library;
    return destination;
  }
  // Concatenation order between libraries is not VHDL declaration order.
  bool precedes(size_t use, size_t declaration) const {
    return owner(use) == owner(declaration) && use < declaration;
  }
  size_t start(NLLibrary* library) const {
    for (const auto& source : sources) if (source.library == library) return source.offset;
    return 0;
  }
  NLLibrary* resolve(NLLibrary* work, const std::string& name) const {
    if (name == "work") return work;
    NLLibrary* result = nullptr;
    try {
      result = findHDLLibrary(work->getDB(), name);
    } catch (const NLException& error) {
      fail(error.what());
    }
    if (!result) fail("missing VHDL library: " + name);
    return result;
  }
};

struct StaticScalar {
  int64_t value = 0;
  bool boolean = false;
};
struct GenericValue {
  StaticScalar scalar;
  // Binary bits in declaration order, independent of the caller's netlist.
  std::optional<std::string> bits;
};
using GenericValues = std::map<std::string, GenericValue>;

class RTLConstructor {
 public:
  RTLConstructor(SNLDesign* design, const vhdl::EntityDeclaration& entity,
                 const vhdl::ArchitectureBody& architecture,
                 const vhdl::DesignFile& syntax, const LibraryContext& libraries,
                 GenericValues generics,
                 std::function<SNLDesign*(NLLibrary*, const std::string&, const GenericValues&)> build):
      design_(design), entity_(entity), architecture_(architecture), syntax_(syntax), libraryContext_(libraries),
      work_(libraries.owner(entity.span.start.offset)),
      genericOverrides_(std::move(generics)), build_(std::move(build)) {}

  void prepareEntity() {
    DiagnosticScope location(entity_.span);
    context(entity_.context);
    for (const auto& generic : entity_.generics) {
      DiagnosticScope location(generic.span);
      const auto type = key(generic.type.name);
      const bool vector = type == "bit_vector" || type == "std_logic_vector" ||
          type == "std_ulogic_vector" || type == "unsigned" || type == "signed";
      if (!vector && type != "integer" && type != "natural" && type != "positive" && type != "boolean")
        fail("RTL generics require an integer subtype, boolean, or constrained binary vector");
      std::optional<Shape> target;
      if (vector) {
        target = shape(generic.type);
        target->size();
      }
      for (const auto& name : generic.names) {
        DiagnosticScope location(name.span);
        const auto id = key(name);
        if (!generic.defaultValue && !genericOverrides_.contains(id))
          fail("missing generic value: " + id);
        if (functions_.contains(id) || objects_.contains(id) ||
            booleans_.contains(id) || integers_.contains(id)) fail("duplicate generic");
        const auto* expected = target ? &*target : nullptr;
        if (genericOverrides_.contains(id) && genericActual_)
          genericOverrides_.at(id) = genericActual_(id, expected);
        const auto value = genericOverrides_.contains(id) ? genericOverrides_.at(id) :
            genericValue(*generic.defaultValue, expected);
        if (vector) {
          if (!value.bits || value.bits->size() != target->size()) fail("vector generic type or length mismatch: " + id);
          Value object{*target, {}};
          for (char bit : *value.bits) object.bits.push_back(constant(bit == '1'));
          objects_.emplace(id, Object{std::move(object), false, false, false, true});
        } else {
          if (value.bits || value.scalar.boolean != (type == "boolean")) fail("generic value type mismatch: " + id);
          if (type == "boolean" && generic.type.constraint) fail("boolean generic constraints are unsupported");
          if (generic.type.constraint) range(*generic.type.constraint).position(value.scalar.value);
          if ((type == "positive" && value.scalar.value < 1) || (type == "natural" && value.scalar.value < 0))
            fail("generic value violates its subtype");
          if (value.scalar.boolean) booleans_.emplace(id, value.scalar.value);
          else integers_.emplace(id, value.scalar.value);
        }
      }
    }
  }

  GenericValue genericValue(const Expr& actual, const Shape* expected) {
    DiagnosticScope location(actual.span);
    if (!expected) return {scalar(actual), {}};
    const auto value = expression(actual, State{}, expected);
    std::string bits;
    for (auto* bit : value.bits) {
      const auto binary = constantValue(bit);
      if (!binary) fail("vector generic actual must be static and binary");
      bits += *binary ? '1' : '0';
    }
    return {{}, std::move(bits)};
  }

  void run() {
    DiagnosticScope location(entity_.span);
    prepareEntity();
    for (const auto& port : entity_.ports) {
      if (port.mode != vhdl::PortMode::In && port.mode != vhdl::PortMode::Out)
        fail("RTL ports must be in or out");
      if (port.defaultValue && port.mode != vhdl::PortMode::In)
        fail("port defaults are supported only for input ports");
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
    declarations(architecture_.arrayTypes, architecture_.constants, architecture_.recordTypes,
                 architecture_.signals, architecture_.enumerationTypes, architecture_.functions);
    inferMemories();
    checkLabels(architecture_.generates, architecture_.instantiations);
    State empty;
    for (const auto& assignment : architecture_.assignments)
      concurrent(assignment, empty);
    for (const auto& generate : architecture_.generates) lowerGenerate(generate, empty);
    for (const auto& instance : architecture_.instantiations) lowerInstance(instance);
    for (const auto& process : architecture_.processes) lowerProcess(process);
    finalizeMemories();
    for (const auto& [bit, value] : initialValues_)
      if (!initializedFlops_.contains(bit)) fail("initialized signal must be driven by a local clocked process");
    for (const auto& [name, object] : objects_)
      if (!memories_.contains(name)) checkDriven(name, object);
    for (const auto& [name, object] : generatedObjects_) checkDriven(name, object);
  }

 private:
  struct Sensitivity {
    std::set<std::string> names;
    std::set<SNLBitNet*> bits;
    std::map<SNLBitNet*, std::string> signals;
  };

  Sensitivity processSensitivity(const vhdl::ClockedProcess& process) {
    Sensitivity result;
    for (const auto& [name, object] : objects_)
      if (!object.variable && !object.constant)
        for (auto* bit : object.value.bits) result.signals.emplace(bit, name);
    if (process.allSensitivity) return result;
    const auto add = [&](const vhdl::Name& name, const std::vector<vhdl::Name>& fields) {
      DiagnosticScope location(name.span);
      auto selected = selection(key(name));
      const auto& object = objects_.at(selected.name);
      if (object.variable || object.constant) fail("process sensitivity requires signals");
      for (const auto& member : fields) field(selected, key(member));
      if (fields.empty()) result.names.insert(key(name));
      for (size_t i = 0; i < selected.shape.size(); ++i)
        result.bits.insert(object.value.bits[selected.offset + i]);
    };
    if (process.sensitivityList.empty()) add(process.sensitivity, {});
    else for (size_t i = 0; i < process.sensitivityList.size(); ++i)
      add(process.sensitivityList[i], process.sensitivityFields.empty() ?
          std::vector<vhdl::Name>{} : process.sensitivityFields[i]);
    return result;
  }

  void checkDriven(const std::string& name, const Object& object) {
    if (object.variable || object.input || object.constant) return;
    for (auto* bit : object.value.bits)
      if ((object.output || reads_.contains(bit)) && !drivers_.contains(bit))
        fail("signal bit has no driver: " + name);
  }

  void addSignal(const vhdl::SignalDeclaration& signal) {
    DiagnosticScope location(signal.span);
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

  struct MemoryRead { Bits address, data; };
  struct Memory {
    const vhdl::ClockedProcess* process = nullptr;
    NLDB0::MemorySignature signature;
    std::vector<MemoryRead> reads;
    MemoryWrite write;
    SNLBitNet* clock = nullptr;
  };

  // Only infer a single whole-word write site. Multiple sites, partial writes,
  // initialization and writes elaborated in loops keep the bit-level lowering.
  void inferMemories() {
    for (const auto& [name, object] : objects_) {
      const auto& type = object.value.shape;
      if (object.input || object.output || object.variable || object.constant ||
          !type.fields.empty() || type.ranges.empty() || type.ranges.front().indexEnumeration || type.ranges.size() > 2 ||
          !arrays_.contains(type.types.front()) ||
          std::min(type.ranges.front().left, type.ranges.front().right) < 0 ||
          std::max(type.ranges.front().left, type.ranges.front().right) > INT32_MAX ||
          std::any_of(object.value.bits.begin(), object.value.bits.end(),
              [&](auto* bit) { return initialValues_.contains(bit); })) continue;
      size_t writes = 0;
      bool supported = true;
      const vhdl::ClockedProcess* owner = nullptr;
      const auto assignment = [&](const vhdl::Assignment& a,
                                  const vhdl::ClockedProcess* process, bool loop) {
        if (key(a.target) != name) return;
        ++writes;
        owner = process;
        supported &= process && !process->combinational && !process->asynchronousReset && !loop && a.kind == vhdl::AssignmentKind::Signal &&
            a.indices.size() == 1 && a.indices.front()->kind != Expr::Kind::Range &&
            !isStatic(*a.indices.front());
      };
      std::function<void(const std::vector<Statement>&, const vhdl::ClockedProcess*, bool)> scan;
      scan = [&](const auto& statements, auto* process, bool loop) {
        for (const auto& statement : statements) {
          if (statement.kind == Statement::Kind::Assignment)
            assignment(statement.assignment, process, loop);
          scan(statement.statements, process, loop || statement.kind == Statement::Kind::For);
          scan(statement.alternative, process, loop || statement.kind == Statement::Kind::For);
          for (const auto& body : statement.caseBodies) scan(body, process, loop);
        }
      };
      for (const auto& process : architecture_.processes) {
        scan(process.statements, &process, false);
        scan(process.resetStatements, &process, false);
        for (const auto& a : process.assignments)
          assignment(a, process.sensitivityList.empty() ? &process : nullptr, false);
        for (const auto& a : process.resetAssignments) assignment(a, &process, false);
      }
      for (const auto& a : architecture_.assignments) assignment(a, nullptr, false);
      const auto instances = [&](const auto& instances) {
        for (const auto& instance : instances)
          for (const auto& actual : instance.actuals)
            if (key(actual) == name) supported = false;
      };
      instances(architecture_.instantiations);
      std::function<void(const std::vector<vhdl::GenerateStatement>&)> generates;
      generates = [&](const auto& statements) {
        for (const auto& generate : statements) {
          for (const auto& a : generate.assignments) assignment(a, nullptr, true);
          instances(generate.instantiations);
          for (const auto& process : generate.processes) {
            scan(process.statements, &process, true);
            scan(process.resetStatements, &process, true);
            for (const auto& a : process.assignments) assignment(a, &process, true);
            for (const auto& a : process.resetAssignments) assignment(a, &process, true);
          }
          generates(generate.generates);
          generates(generate.alternatives);
        }
      };
      generates(architecture_.generates);
      if (!supported || writes != 1) continue;
      Memory memory;
      memory.process = owner;
      memory.signature.depth = type.ranges.front().size();
      memory.signature.width = type.size() / memory.signature.depth;
      memory.signature.abits = 1;
      while ((size_t(1) << memory.signature.abits) < memory.signature.depth)
        ++memory.signature.abits;
      memory.signature.writePorts = 1;
      memories_.emplace(name, std::move(memory));
    }
  }

  // Full-width bounds checks prevent a wide or nonzero-based VHDL index from
  // wrapping onto a valid memory word when converted to the primitive address.
  std::pair<Bits, SNLBitNet*> memoryAddress(const std::string& name, Bits address) {
    const auto& bounds = objects_.at(name).value.shape.ranges.front();
    const auto low = std::min(bounds.left, bounds.right);
    const auto high = std::max(bounds.left, bounds.right);
    const auto width = std::max<size_t>(32, address.size());
    address = resize(std::move(address), width);
    const auto lessThan = [&](uint64_t limit) {
      auto* less = constant(false);
      for (size_t i = address.size(); i; --i) {
        const auto position = address.size() - i;
        const bool one = position < 64 && ((limit >> position) & 1);
        if (const auto bit = constantValue(address[i - 1])) {
          if (*bit != one) less = constant(one);
        } else if (const auto previous = constantValue(less)) {
          less = *previous == one ? constant(one) : gate("not", address[i - 1]);
        } else {
          less = one ? gate("or", gate("not", address[i - 1]), less)
                     : gate("and", gate("not", address[i - 1]), less);
        }
      }
      return less;
    };
    auto* valid = lessThan(uint64_t(high) + 1);
    if (low) valid = gate("and", gate("not", lessThan(low)), valid);
    if (low) address = add(address, number(low, {{"integer"}, {}, address.size()}).bits, true);
    return {resize(std::move(address), memories_.at(name).signature.abits), valid};
  }

  Value readMemory(const std::string& name, const Expr& index, const State& state) {
    if (sensitivity_) for (auto* bit : objects_.at(name).value.bits)
      if (!sensitivity_->bits.contains(bit))
        fail("process sensitivity omits a memory read: " + name);
    auto element = objects_.at(name).value.shape;
    if (isStatic(index)) element.ranges.front().position(integer(index));
    auto addressValue = expression(index, state);
    if (!addressValue.shape.integerWidth) fail("array index must have integer type");
    auto [address, valid] = memoryAddress(name, std::move(addressValue.bits));
    element.ranges.erase(element.ranges.begin());
    element.removeOuterType();
    auto& memory = memories_.at(name);
    // Reuse a port for identical resolved addresses (including constant words).
    for (const auto& read : memory.reads)
      if (read.address == address)
        return {element, mux(valid, read.data, Bits(element.size(), constant(false)))};
    auto* output = SNLBusNet::create(design_, element.size() - 1, 0);
    Bits data;
    for (size_t i = 0; i < element.size(); ++i) data.push_back(output->getBitAtPosition(i));
    memory.reads.push_back({std::move(address), data});
    return {element, mux(valid, data, Bits(element.size(), constant(false)))};
  }

  void finalizeMemories() {
    for (auto& [name, memory] : memories_) {
      if (!memory.clock) fail("inferred memory has no clocked writer: " + name);
      memory.signature.readPorts = std::max<size_t>(1, memory.reads.size());
      auto* model = NLDB0::getOrCreateMemory(memory.signature);
      auto instanceName = name + "_mem";
      for (size_t suffix = 1; design_->getInstance(NLName(instanceName)); ++suffix)
        instanceName = name + "_mem_" + std::to_string(suffix);
      auto* instance = SNLInstance::create(design_, model, NLName(instanceName));
      // Export uses the shared parameterized naja_mem module, whose default
      // dimensions differ from this width-specific canonical model.
      for (const auto& [parameter, value] : std::vector<std::pair<std::string, size_t>>{
          {"WIDTH", memory.signature.width}, {"DEPTH", memory.signature.depth},
          {"ABITS", memory.signature.abits}, {"RD_PORTS", memory.signature.readPorts},
          {"WR_PORTS", memory.signature.writePorts}})
        SNLInstParameter::create(instance, model->getParameter(NLName(parameter)), std::to_string(value));
      instance->setTermNet(NLDB0::getMemoryClock(model), memory.clock);
      instance->setTermNet(NLDB0::getMemoryReset(model), constant(false));
      const auto connect = [&](SNLBusTerm* term, const Bits& bits, size_t offset = 0) {
        for (size_t i = 0; i < bits.size(); ++i)
          instance->setTermNet(term->getBit(offset + bits.size() - 1 - i), bits[i]);
      };
      connect(NLDB0::getMemoryWriteAddress(model), memory.write.address);
      connect(NLDB0::getMemoryWriteData(model), memory.write.data);
      connect(NLDB0::getMemoryWriteEnable(model), {memory.write.enable});
      if (memory.reads.empty()) {
        connect(NLDB0::getMemoryReadAddress(model), Bits(memory.signature.abits, constant(false)));
      } else for (size_t port = 0; port < memory.reads.size(); ++port) {
        connect(NLDB0::getMemoryReadAddress(model), memory.reads[port].address, port * memory.signature.abits);
        connect(NLDB0::getMemoryReadData(model), memory.reads[port].data, port * memory.signature.width);
      }
      // The declaration's temporary word nets are replaced entirely by RAM.
      std::set<SNLNet*> nets;
      for (auto* bit : objects_.at(name).value.bits) {
        auto* busBit = dynamic_cast<SNLBusNetBit*>(bit);
        nets.insert(busBit ? static_cast<SNLNet*>(busBit->getBus()) : bit);
      }
      for (auto* net : nets) net->destroy();
      objects_.at(name).value.bits.clear();
    }
  }

  void addArray(const vhdl::ArrayTypeDeclaration& declaration) {
    DiagnosticScope location(declaration.span);
    const auto name = key(declaration.name);
    if (functions_.contains(name) || booleans_.contains(name) || (enumerations_.contains(name) || records_.contains(name)) || arrayDeclarations_.contains(name) || integerTables_.contains(name) || integers_.contains(name) || objects_.contains(name) ||
        name == "bit" || name == "bit_vector" || name == "std_logic" ||
        name == "std_logic_vector" || name == "std_ulogic" || name == "std_ulogic_vector" ||
        name == "unsigned" || name == "signed" || name == "integer" ||
        name == "natural" || name == "positive") fail("duplicate or shadowing array type: " + name);
    if (declaration.indexSubtype) {
      const auto subtype = key(*declaration.indexSubtype);
      if (subtype != "integer" && subtype != "natural" && subtype != "positive" && !enumerations_.contains(subtype))
        fail("unsupported unconstrained array index subtype: " + subtype);
    }
    arrayDeclarations_.emplace(name, &declaration);
    const auto elementName = key(declaration.elementType.name);
    // Integer tables are evaluated as static values, without synthesizing an
    // integer memory or losing signed values through a bit-vector conversion.
    if (elementName == "integer" || elementName == "natural" || elementName == "positive") {
      if (declaration.indexType || (declaration.indexSubtype && enumerations_.contains(key(*declaration.indexSubtype))) ||
          (declaration.indexRange.leftExpression && enumIndexType(*declaration.indexRange.leftExpression)))
        fail("enumeration-indexed integer constant tables are not yet supported");
      return;
    }
    auto element = shape(declaration.elementType);
    element.types.insert(element.types.begin(), name);
    element.declarations.insert(element.declarations.begin(), &declaration);
    element.ranges.insert(element.ranges.begin(), arrayBounds(declaration));
    if (!declaration.indexSubtype) element.size();
    arrays_.emplace(name, std::move(element));
    arrayTypeOffsets_.emplace(name, declaration.span.start.offset);
  }

  void addRecord(const vhdl::RecordTypeDeclaration& declaration) {
    DiagnosticScope location(declaration.span);
    const auto name = key(declaration.name);
    if (functions_.contains(name) || booleans_.contains(name) || (enumerations_.contains(name) || records_.contains(name)) || arrayDeclarations_.contains(name) ||
        objects_.contains(name) || integers_.contains(name) || integerTables_.contains(name) ||
        name == "bit" || name == "bit_vector" || name == "std_logic" || name == "std_ulogic" ||
        name == "std_logic_vector" || name == "std_ulogic_vector" || name == "unsigned" ||
        name == "signed" || name == "integer" || name == "natural" || name == "positive" || name == "boolean")
      fail("duplicate or shadowing record type: " + name);
    Shape record{{name}, {}};
    record.declarations.push_back(&declaration);
    std::set<std::string> names;
    for (const auto& field : declaration.fields) {
      auto type = shape(field.type);
      for (const auto& fieldName : field.names) {
        if (!names.insert(key(fieldName)).second) fail("duplicate record field: " + key(fieldName));
        record.fields.emplace_back(key(fieldName), type);
      }
    }
    record.size();
    records_.emplace(name, std::move(record));
    recordOffsets_[name] = declaration.span.start.offset;
  }

  void addEnumeration(const vhdl::EnumerationTypeDeclaration& declaration) {
    DiagnosticScope location(declaration.span);
    const auto available = [&](const std::string& name) {
      static const std::set<std::string> builtins{"bit", "bit_vector", "std_logic", "std_ulogic",
          "std_logic_vector", "std_ulogic_vector", "integer", "natural", "positive", "boolean", "true", "false", "unsigned", "signed"};
      return !builtins.contains(name) && !objects_.contains(name) && !integers_.contains(name) &&
          !booleans_.contains(name) && !functions_.contains(name) && !records_.contains(name) &&
          !arrayDeclarations_.contains(name) && !integerTables_.contains(name) && !enumerations_.contains(name);
    };
    const auto name = key(declaration.name);
    if (!available(name)) fail("duplicate or shadowing enumeration type: " + name);
    if (declaration.literals.empty() || declaration.literals.size() > 4096)
      fail("enumeration must contain 1 to 4096 identifier literals");
    enumerations_[name] = &declaration;
    Shape type{{name}, {}, 0, {}, &declaration};
    for (size_t i = 0; i < declaration.literals.size(); ++i) {
      const auto literal = key(declaration.literals[i]);
      if (!available(literal)) fail("duplicate, overloaded, or shadowing enumeration literal: " + literal);
      objects_.emplace(literal, Object{number(i, type), false, false, false, true});
    }
  }

  void declarations(const std::vector<vhdl::ArrayTypeDeclaration>& arrays,
                    const std::vector<vhdl::ConstantDeclaration>& constants,
                    const std::vector<vhdl::RecordTypeDeclaration>& records,
                    const std::vector<vhdl::SignalDeclaration>& signals = {},
                    const std::vector<vhdl::EnumerationTypeDeclaration>& enumerations = {},
                    const std::vector<vhdl::FunctionDeclaration>& functions = {}) {
    std::map<size_t, std::function<void()>> ordered;
    for (const auto& array : arrays)
      ordered.emplace(array.span.start.offset, [&array, this] { addArray(array); });
    for (const auto& constant : constants)
      ordered.emplace(constant.object.span.start.offset, [&constant, this] { addConstant(constant); });
    for (const auto& record : records)
      ordered.emplace(record.span.start.offset, [&record, this] { addRecord(record); });
    for (const auto& signal : signals)
      ordered.emplace(signal.span.start.offset, [&signal, this] { addSignal(signal); });
    for (const auto& enumeration : enumerations)
      ordered.emplace(enumeration.span.start.offset, [&enumeration, this] { addEnumeration(enumeration); });
    for (const auto& function : functions)
      ordered.emplace(function.span.start.offset, [&function, this] { addLocalFunction(function); });
    for (const auto& [offset, add] : ordered) add();
  }

  void checkInteger(int64_t value, const vhdl::TypeMark& type) {
    const auto name = key(type.name);
    if (value < INT32_MIN || value > INT32_MAX || (name == "natural" && value < 0) ||
        (name == "positive" && value < 1)) fail("integer constant violates its subtype");
    if (type.constraint) range(*type.constraint).position(value);
  }

  Range constantBounds(const vhdl::ArrayTypeDeclaration& array,
                       const vhdl::TypeMark& type, const Expr& value) {
    if (!array.indexSubtype) {
      if (type.constraint) fail("reconstraining an array type is not supported");
      return arrayBounds(array);
    }
    Range bounds;
    const auto subtype = key(*array.indexSubtype);
    if (const auto found = enumerations_.find(subtype); found != enumerations_.end()) {
      if (!type.constraint) fail("unconstrained enumeration-indexed arrays require explicit bounds");
      return enumRange(*type.constraint, found->second);
    }
    const int64_t lower = subtype == "natural" ? 0 : subtype == "positive" ? 1 : INT32_MIN;
    if (type.constraint) bounds = range(*type.constraint);
    else {
      if (value.kind != Expr::Kind::Aggregate || value.elements.empty())
        fail("unconstrained array constant requires a positional aggregate or explicit bounds");
      bounds = {lower, lower + int64_t(value.elements.size()) - 1, true};
    }
    if (bounds.left < lower || bounds.right < lower || bounds.left > INT32_MAX || bounds.right > INT32_MAX)
      fail("array bounds violate the index subtype");
    return bounds;
  }

  bool locallyStaticPortIndex(const Expr& expr) const {
    if (expr.kind == Expr::Kind::IntegerLiteral) return true;
    if (expr.kind == Expr::Kind::Name) {
      const auto found = objects_.find(key(expr));
      return locallyStaticIntegers_.contains(key(expr)) ||
          (found != objects_.end() && found->second.constant && found->second.value.shape.enumeration &&
           found->second.value.shape.ranges.empty());
    }
    return (expr.kind == Expr::Kind::Unary || expr.kind == Expr::Kind::Binary ||
            expr.kind == Expr::Kind::Range) && expr.left &&
        locallyStaticPortIndex(*expr.left) && (!expr.right || locallyStaticPortIndex(*expr.right));
  }

  void addConstant(const vhdl::ConstantDeclaration& declaration) {
    const auto& type = declaration.object.type;
    const auto name = key(type.name);
    std::set<std::string> names;
    for (const auto& id : declaration.object.names)
      if (functions_.contains(key(id)) || booleans_.contains(key(id)) || !names.insert(key(id)).second || objects_.contains(key(id)) || integers_.contains(key(id)) ||
          integerTables_.contains(key(id)) || (enumerations_.contains(key(id)) || records_.contains(key(id))) || arrayDeclarations_.contains(key(id)))
        fail("duplicate constant: " + key(id));
    if (name == "string") {
      // String constants are synthesis diagnostics metadata; retain no hardware value.
      return;
    }
    if (name == "boolean") {
      const auto value = scalar(*declaration.value);
      checkStaticType(value, type, nullptr);
      for (const auto& id : declaration.object.names) booleans_.emplace(key(id), value.value);
      return;
    }
    if (name == "integer" || name == "natural" || name == "positive") {
      const auto value = integer(*declaration.value);
      checkInteger(value, type);
      const bool local = locallyStaticPortIndex(*declaration.value) &&
          (!type.constraint ||
           ((!type.constraint->leftExpression || locallyStaticPortIndex(*type.constraint->leftExpression)) &&
            (!type.constraint->rightExpression || locallyStaticPortIndex(*type.constraint->rightExpression))));
      for (const auto& id : declaration.object.names) {
        integers_.emplace(key(id), value);
        if (local) locallyStaticIntegers_.insert(key(id));
      }
      return;
    }
    auto constantType = type;
    if (arrayDeclarations_.contains(name)) {
      const auto& array = *arrayDeclarations_.at(name);
      if (libraryContext_.precedes(type.name.span.start.offset, array.span.start.offset)) fail("array type used before declaration");
      const auto bounds = constantBounds(array, type, *declaration.value);
      if (!bounds.size()) fail("null array constant is unsupported");
      const auto element = key(array.elementType.name);
      if (element == "integer" || element == "natural" || element == "positive") {
        if (declaration.value->kind != Expr::Kind::Aggregate ||
            declaration.value->elements.size() != bounds.size()) fail("integer table aggregate length mismatch");
        IntegerTable table{bounds, {}};
        for (const auto& item : declaration.value->elements) {
          const auto value = integer(*item);
          checkInteger(value, array.elementType);
          table.values.push_back(value);
        }
        for (const auto& id : declaration.object.names) integerTables_.emplace(key(id), table);
        return;
      }
      if (array.indexSubtype && !bounds.indexEnumeration) constantType.constraint = vhdl::DiscreteRange{
          bounds.left, bounds.right, bounds.ascending, {}, {}, {}};
    }
    const auto target = shape(constantType);
    const auto value = expression(*declaration.value, State{}, &target);
    for (auto* bit : value.bits) if (!constantValue(bit)) fail("constant initializer must be static and binary");
    for (const auto& id : declaration.object.names)
      objects_.emplace(key(id), Object{value, false, false, false, true});
  }

  void checkLabels(const std::vector<vhdl::GenerateStatement>& generates,
                   const std::vector<vhdl::EntityInstantiation>& instances) {
    std::set<std::string> labels;
    for (const auto& instance : instances)
      if (!labels.insert(key(instance.label)).second) fail("duplicate concurrent label");
    for (const auto& generate : generates) {
      if (!generate.label.spelling.empty() && !labels.insert(key(generate.label)).second)
        fail("duplicate concurrent label");
      checkLabels(generate.generates, generate.instantiations);
      for (const auto& branch : generate.alternatives)
        checkLabels(branch.generates, branch.instantiations);
    }
  }

  void lowerGenerate(const vhdl::GenerateStatement& generate, const State& state,
                     const std::string& prefix = "") {
    DiagnosticScope location(generate.conditional ? generate.label.span : generate.iterator.span);
    const auto body = [&](const vhdl::GenerateStatement& branch, const std::string& scope) {
      if (++steps_ > 100000) fail("static generate elaboration limit exceeded");
      const auto outerPrefix = objectPrefix_;
      objectPrefix_ = scope;
      declarations(branch.arrayTypes, branch.constants, branch.recordTypes, branch.signals, branch.enumerationTypes);
      for (const auto& assignment : branch.assignments) concurrent(assignment, state);
      for (const auto& child : branch.generates) lowerGenerate(child, state, scope);
      for (const auto& instance : branch.instantiations) lowerInstance(instance, scope);
      for (const auto& process : branch.processes) lowerProcess(process);
      // Bindings are local to this elaborated body; hardware and driver tracking
      // remain live after the scope ends, including reads through outer signals.
      for (const auto& signal : branch.signals) for (const auto& name : signal.names) {
        generatedObjects_.emplace_back(scope + name.spelling, objects_.at(key(name)));
        objects_.erase(key(name));
      }
      for (const auto& constant : branch.constants) for (const auto& name : constant.object.names) {
        objects_.erase(key(name));
        integers_.erase(key(name));
        locallyStaticIntegers_.erase(key(name));
        booleans_.erase(key(name));
        integerTables_.erase(key(name));
      }
      for (const auto& array : branch.arrayTypes) {
        arrays_.erase(key(array.name));
        arrayDeclarations_.erase(key(array.name));
        arrayTypeOffsets_.erase(key(array.name));
      }
      for (const auto& record : branch.recordTypes) {
        records_.erase(key(record.name));
        recordOffsets_.erase(key(record.name));
      }
      for (const auto& enumeration : branch.enumerationTypes) {
        for (const auto& literal : enumeration.literals) objects_.erase(key(literal));
        enumerations_.erase(key(enumeration.name));
      }
      objectPrefix_ = outerPrefix;
    };
    if (generate.conditional) {
      const auto selected = [&](const vhdl::GenerateStatement& branch) {
        if (!branch.condition) return true;
        const auto value = scalar(*branch.condition, nullptr);
        if (!value.boolean) fail("if generate condition must be a static boolean");
        return value.value != 0;
      };
      const auto scope = prefix + generate.label.spelling + ".";
      if (selected(generate)) body(generate, scope);
      else for (const auto& branch : generate.alternatives) {
        if (selected(branch)) { body(branch, scope); break; }
      }
      return;
    }
    auto name = key(generate.iterator);
    if (integers_.contains(name) || objects_.contains(name) || integerTables_.contains(name) ||
        (enumerations_.contains(name) || records_.contains(name)) || arrayDeclarations_.contains(name)) fail("shadowed generate parameter");
    const auto bounds = range(generate.range);
    for (size_t i = 0; i < bounds.size(); ++i) {
      integers_[name] = bounds.ascending ? bounds.left + i : bounds.left - i;
      const auto scope = prefix + (generate.label.spelling.empty() ? name : generate.label.spelling) +
          "[" + std::to_string(integers_[name]) + "].";
      body(generate, scope);
    }
    integers_.erase(name);
  }

  void lowerInstance(const vhdl::EntityInstantiation& instance, const std::string& prefix = "") {
    DiagnosticScope location(instance.span);
    if (instance.architecture)
      fail("unsupported RTL entity binding");
    if (!libraries_.contains(key(instance.library))) fail("library is not visible: " + key(instance.library));
    auto* targetLibrary = libraryContext_.resolve(work_, key(instance.library));
    const vhdl::EntityDeclaration* entity = nullptr;
    for (const auto& candidate : syntax_.entities)
      if (libraryContext_.owner(candidate.span.start.offset) == targetLibrary &&
          key(candidate.name) == key(instance.entity)) entity = &candidate;
    if (!entity) fail("missing entity: " + key(instance.entity));
    if (instance.component && !components_.contains(key(instance.entity)))
      fail("component is not visible: " + key(instance.entity));
    const auto& interface = instance.component ? *components_.at(key(instance.entity)) : *entity;
    std::vector<std::string> genericNames;
    for (const auto& generic : interface.generics)
      for (const auto& name : generic.names) genericNames.push_back(key(name));
    GenericValues overrides;
    std::map<std::string, const Expr*> actuals;
    bool named = false;
    for (size_t i = 0; i < instance.generics.size(); ++i) {
      const auto& association = instance.generics[i];
      if (i >= genericNames.size()) fail("too many generic actuals");
      if (!association.formal && named) fail("positional generic follows named generic");
      named |= association.formal.has_value();
      const auto name = association.formal ? key(*association.formal) : genericNames[i];
      if (std::find(genericNames.begin(), genericNames.end(), name) == genericNames.end() ||
          !actuals.emplace(name, association.actual.get()).second) fail("invalid generic association");
      overrides.emplace(name, GenericValue{});
    }
    RTLConstructor childScope(design_, *entity, architecture_, syntax_, libraryContext_, overrides, build_);
    childScope.genericActual_ = [&](const std::string& name, const Shape* expected) {
      return genericValue(*actuals.at(name), expected);
    };
    childScope.prepareEntity();
    overrides = childScope.genericOverrides_;
    const auto parentObjects = objects_;
    const auto parentIntegers = integers_;
    const auto parentBooleans = booleans_;
    for (const auto& generic : entity->generics)
      for (const auto& name : generic.names) {
        const auto id = key(name);
        integers_.erase(id);
        booleans_.erase(id);
        objects_.erase(id);
        if (childScope.objects_.contains(id)) objects_[id] = childScope.objects_.at(id);
        else if (childScope.booleans_.contains(id)) booleans_[id] = childScope.booleans_.at(id);
        else integers_[id] = childScope.integers_.at(id);
      }
    std::map<std::string, Shape> formalShapes;
    for (const auto& port : entity->ports)
      for (const auto& name : port.names) formalShapes.emplace(key(name), childScope.shape(port.type));
    if (instance.component) {
      const auto& component = *components_.at(key(instance.entity));
      std::map<std::string, vhdl::PortMode> modes;
      for (const auto& port : entity->ports)
        for (const auto& name : port.names) modes.emplace(key(name), port.mode);
      std::set<std::string> declaredPorts;
      for (const auto& port : component.ports) for (const auto& name : port.names) {
        const auto id = key(name);
        const auto declaredShape = shape(port.type);
        if (!declaredPorts.insert(id).second || !modes.contains(id) || modes.at(id) != port.mode ||
            !compatible(declaredShape, formalShapes.at(id))) fail("component port interface mismatch");
        // Formal selections name the component interface; binding to the entity
        // preserves position even when their index bounds differ.
        formalShapes.at(id) = declaredShape;
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
    objects_ = parentObjects;
    integers_ = parentIntegers;
    booleans_ = parentBooleans;
    auto* model = build_(targetLibrary, key(instance.entity), overrides);
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
    const auto instanceName = prefix + instance.label.spelling;
    if (!instanceNames_.insert(prefix + key(instance.label)).second) fail("duplicate instance label");
    auto* child = SNLInstance::create(design_, model, NLName(instanceName));
    std::map<SNLTerm*, std::vector<bool>> bound;
    SNLTerm* previousPort = nullptr;
    named = false;
    for (size_t i = 0; i < instance.actuals.size(); ++i) {
      auto* port = i < ports.size() ? ports[i] : nullptr;
      if (!instance.formals.empty() && instance.formals[i]) {
        named = true;
        port = nullptr;
        for (auto* candidate : ports) {
          std::string name = candidate->getName().getString();
          for (auto& c : name) c = std::tolower(static_cast<unsigned char>(c));
          if (name == key(*instance.formals[i])) port = candidate;
        }
      } else if (named) fail("positional port follows named port");
      if (!port) fail("invalid port association");
      if (bound.contains(port) && previousPort != port)
        fail("individual port associations must be consecutive");
      previousPort = port;
      std::string literalFormal = port->getName().getString();
      for (auto& c : literalFormal) c = std::tolower(static_cast<unsigned char>(c));
      Selection formal{literalFormal, formalShapes.at(literalFormal), 0};
      if (!instance.formalIndices.empty()) {
        for (const auto& indexExpr : instance.formalIndices[i]) {
          DiagnosticScope location(indexExpr->span);
          if (indexExpr->kind != Expr::Kind::Selected && !locallyStaticPortIndex(*indexExpr))
            fail("port formal index requires a locally static integer expression; "
                 "only literals, integer constants, and predefined arithmetic are supported");
          index(formal, *indexExpr);
        }
      }
      auto& covered = bound.try_emplace(port, port->getWidth(), false).first->second;
      std::vector<SNLBitTerm*> formalBits;
      size_t position = 0;
      for (auto* term : port->getBits()) {
        if (position >= formal.offset && position < formal.offset + formal.shape.size()) {
          if (covered[position]) fail("overlapping port associations");
          covered[position] = true;
          formalBits.push_back(term);
        }
        ++position;
      }
      if (formalBits.size() != formal.shape.size()) fail("port selection exceeds flattened port width");
      if (!instance.actualOpen.empty() && instance.actualOpen[i]) {
        DiagnosticScope location(instance.actuals[i].span);
        if (!instance.formalIndices.empty() && !instance.formalIndices[i].empty())
          fail("open cannot be associated with an individual port subelement or slice");
        if (port->getDirection() != SNLTerm::Direction::Output)
          fail("open input ports require defaults, which are not yet supported");
        // As in the SV loader, an explicit empty connection leaves inst terms unconnected.
        continue;
      }
      if (!instance.actualLiterals.empty() && instance.actualLiterals[i]) {
        if (port->getDirection() != SNLTerm::Direction::Input)
          fail("literal port actual requires an input port");
        const auto& expected = formal.shape;
        const auto value = expression(*instance.actualLiterals[i], State{}, &expected);
        if (value.bits.size() != formalBits.size() || !compatible(value.shape, expected))
          fail("port type or width mismatch");
        size_t bit = 0;
        for (auto* term : formalBits) child->getInstTerm(term)->setNet(value.bits[bit++]);
        continue;
      }
      auto selected = selection(key(instance.actuals[i]));
      if (!instance.actualIndices.empty())
        for (const auto& indexExpr : instance.actualIndices[i]) index(selected, *indexExpr);
      const auto& object = objects_.at(selected.name);
      if (selected.shape.size() != formalBits.size() || !compatible(selected.shape, formal.shape))
        fail("port type or width mismatch");
      size_t bit = selected.offset;
      for (auto* term : formalBits) {
        auto* net = object.value.bits[bit++];
        if (port->getDirection() == SNLTerm::Direction::Output) {
          if (object.input || object.constant || !drivers_.insert(net).second)
            fail("invalid component output driver");
        } else reads_.insert(net);
        child->getInstTerm(term)->setNet(net);
      }
    }
    for (size_t declarationIndex = 0; declarationIndex < interface.ports.size(); ++declarationIndex) {
      const auto& declaration = interface.ports[declarationIndex];
      for (const auto& name : declaration.names) {
        const auto id = key(name);
        auto* port = ports.front();
        for (auto* candidate : ports) {
          std::string candidateName = candidate->getName().getString();
          for (auto& c : candidateName) c = std::tolower(static_cast<unsigned char>(c));
          if (candidateName == id) { port = candidate; break; }
        }
        const bool complete = bound.contains(port) &&
            std::find(bound.at(port).begin(), bound.at(port).end(), false) == bound.at(port).end();
        if (complete) continue;
        const auto* defaultValue = declaration.defaultValue.get();
        if (!defaultValue) fail("incomplete port association: " + port->getName().getString());
        if (port->getDirection() != SNLTerm::Direction::Input)
          fail("port defaults are supported only for input ports");
        const auto expected = formalShapes.at(id);
        const auto value = expression(*defaultValue, State{}, &expected);
        if (value.bits.size() != port->getWidth() || !compatible(value.shape, expected))
          fail("port default type or width mismatch");
        size_t bit = 0;
        for (auto* term : port->getBits()) child->getInstTerm(term)->setNet(value.bits[bit++]);
        bound[port].assign(port->getWidth(), true);
      }
    }
  }

  void context(const vhdl::ContextClause& clauses) {
    for (const auto& library : clauses.libraries)
      for (const auto& name : library.names) libraries_.insert(key(name));
    for (const auto& use : clauses.uses) {
      DiagnosticScope location(use.span);
      if (use.selectedName.size() == 3 && key(use.selectedName[0]) != "ieee" &&
          key(use.selectedName[0]) != "std" &&
          key(use.selectedName[2]) == "all") {
        const auto name = key(use.selectedName[1]);
        const auto libraryName = key(use.selectedName[0]);
        if (!libraries_.contains(libraryName)) fail("library is not visible: " + libraryName);
        auto* targetLibrary = libraryContext_.resolve(work_, libraryName);
        if (!imported_.insert({targetLibrary, name}).second) continue;
        const vhdl::PackageDeclaration* package = nullptr;
        for (const auto& candidate : syntax_.packages)
          if (!candidate.body && key(candidate.name) == name &&
              libraryContext_.owner(candidate.name.span.start.offset) == targetLibrary) package = &candidate;
        if (!package) fail("missing package: " + name);
        const auto callerLibraries = libraries_;
        const bool callerLogic = stdLogic_, callerUnsigned = unsigned_, callerArith = arith_, callerNumeric = numeric_, callerSigned = signed_;
        auto* callerWork = work_;
        work_ = targetLibrary;
        libraries_ = {"std", "work"};
        context(package->context);
        for (const auto& function : package->functions) {
          const auto name = key(function.name);
          if (integers_.contains(name) || booleans_.contains(name) || objects_.contains(name) ||
              !functions_.emplace(name, FunctionBinding{package, &function}).second)
            fail("ambiguous or overloaded package function: " + name);
        }
        declarations(package->arrayTypes, package->constants, package->recordTypes, {}, package->enumerationTypes);
        for (const auto& component : package->components)
          if (!components_.emplace(key(component.name), &component).second) fail("ambiguous component");
        work_ = callerWork;
        libraries_ = callerLibraries;
        stdLogic_ = callerLogic;
        unsigned_ = callerUnsigned;
        arith_ = callerArith;
        numeric_ = callerNumeric;
        signed_ = callerSigned;
        continue;
      }
      if (use.selectedName.size() == 3 && key(use.selectedName[0]) == "std" &&
          key(use.selectedName[1]) == "standard" && key(use.selectedName[2]) == "all") continue;
      if (use.selectedName.size() != 3 || key(use.selectedName[0]) != "ieee" ||
          key(use.selectedName[2]) != "all" || !libraries_.contains("ieee"))
        fail("unsupported RTL package visibility");
      const auto package = key(use.selectedName[1]);
      if (package == "std_logic_1164") stdLogic_ = true;
      else if (package == "std_logic_unsigned") unsigned_ = true;
      else if (package == "std_logic_signed") signed_ = true;
      else if (package == "std_logic_arith") arith_ = true;
      else if (package == "numeric_std") numeric_ = true;
      else
        fail("unsupported IEEE package: " + package);
    }
  }

  using FunctionBinding = std::pair<const vhdl::PackageDeclaration*, const vhdl::FunctionDeclaration*>;
  struct FunctionEnvironment {
    std::map<std::string, StaticScalar> values;
    std::map<std::string, FunctionBinding> functions;
  };
  struct StaticScope {
    const FunctionEnvironment* environment = nullptr;
    const vhdl::PackageDeclaration* package = nullptr;
    std::map<std::string, StaticScalar> values;
    std::map<std::string, const vhdl::TypeMark*> variables;
  };
  void addLocalFunction(const vhdl::FunctionDeclaration& function) {
    const auto name = key(function.name);
    if (!function.body) fail("architecture function forward declarations are unsupported: " + name);
    if (functions_.contains(name) || integers_.contains(name) || booleans_.contains(name) ||
        objects_.contains(name) || integerTables_.contains(name) || records_.contains(name) ||
        enumerations_.contains(name) || arrayDeclarations_.contains(name))
      fail("duplicate or overloaded architecture function: " + name);
    functions_.emplace(name, FunctionBinding{nullptr, &function});
    auto& environment = localFunctionEnvironments_[&function];
    for (const auto& [id, value] : integers_) environment.values.emplace(id, StaticScalar{value, false});
    for (const auto& [id, value] : booleans_) environment.values.emplace(id, StaticScalar{value, true});
    environment.functions = functions_;
  }

  const vhdl::PackageDeclaration* packageBody(const vhdl::PackageDeclaration& declaration) const {
    for (const auto& package : syntax_.packages)
      if (package.body && key(package.name) == key(declaration.name) &&
          libraryContext_.owner(package.name.span.start.offset) == libraryContext_.owner(declaration.name.span.start.offset)) return &package;
    return nullptr;
  }

  FunctionBinding staticFunction(const std::string& name, const StaticScope* scope) const {
    if (!scope) {
      const auto found = functions_.find(name);
      if (found != functions_.end()) return found->second;
    } else if (scope->environment) {
      const auto found = scope->environment->functions.find(name);
      if (found != scope->environment->functions.end()) return found->second;
    } else if (scope->package) {
      for (const auto& function : scope->package->functions)
        if (key(function.name) == name) return {scope->package, &function};
      const vhdl::FunctionDeclaration* found = nullptr;
      if (const auto* body = packageBody(*scope->package))
        for (const auto& function : body->functions) if (key(function.name) == name) {
          if (found) fail("overloaded private package functions are unsupported: " + name);
          found = &function;
        }
      if (found) return {scope->package, found};
    }
    return {};
  }

  static std::string staticSignature(const Expr* expression) {
    if (!expression) return {};
    std::string result = std::to_string(static_cast<int>(expression->kind)) + ":" + key(*expression) + "(";
    result += staticSignature(expression->left.get()) + "," + staticSignature(expression->right.get());
    for (const auto& element : expression->elements) result += "," + staticSignature(element.get());
    return result + ")";
  }

  static bool sameStaticType(const vhdl::TypeMark& a, const vhdl::TypeMark& b) {
    if (key(a.name) != key(b.name) || bool(a.constraint) != bool(b.constraint)) return false;
    if (!a.constraint) return true;
    return a.constraint->ascending == b.constraint->ascending &&
        staticSignature(a.constraint->leftExpression.get()) == staticSignature(b.constraint->leftExpression.get()) &&
        staticSignature(a.constraint->rightExpression.get()) == staticSignature(b.constraint->rightExpression.get()) &&
        staticSignature(a.constraint->attribute.get()) == staticSignature(b.constraint->attribute.get());
  }

  const vhdl::FunctionDeclaration& functionBody(const FunctionBinding& binding) const {
    const auto& declaration = *binding.second;
    if (declaration.body) return declaration;
    if (!binding.first) fail("missing architecture function body");
    const auto* package = packageBody(*binding.first);
    const vhdl::FunctionDeclaration* found = nullptr;
    if (package) for (const auto& candidate : package->functions) {
      if (key(candidate.name) != key(declaration.name)) continue;
      if (found) fail("overloaded or duplicate function bodies are unsupported: " + key(declaration.name));
      found = &candidate;
    }
    if (!found) fail("missing package function body: " + key(declaration.name));
    bool match = found->pure == declaration.pure && sameStaticType(found->returnType, declaration.returnType);
    std::vector<std::pair<std::string, const vhdl::TypeMark*>> declared, defined;
    for (const auto& parameter : declaration.parameters)
      for (const auto& name : parameter.names) declared.emplace_back(key(name), &parameter.type);
    for (const auto& parameter : found->parameters)
      for (const auto& name : parameter.names) defined.emplace_back(key(name), &parameter.type);
    match &= declared.size() == defined.size();
    if (match) for (size_t i = 0; i < declared.size(); ++i)
      match &= declared[i].first == defined[i].first && sameStaticType(*declared[i].second, *defined[i].second);
    if (!match) fail("package function declaration/body profile mismatch: " + key(declaration.name));
    return *found;
  }

  int64_t staticInteger(const Expr& expression, StaticScope* scope) {
    const auto value = scalar(expression, scope);
    if (value.boolean) fail("static integer expression has boolean type");
    return value.value;
  }

  void checkStaticType(const StaticScalar& value, const vhdl::TypeMark& type, StaticScope* scope) {
    const auto name = key(type.name);
    if (name == "boolean") {
      if (!value.boolean || type.constraint) fail("static boolean type mismatch");
      return;
    }
    if (name != "integer" && name != "natural" && name != "positive")
      fail("static package functions currently require integer or boolean values: " + name);
    if (value.boolean) fail("static integer type mismatch");
    if (value.value < INT32_MIN || value.value > INT32_MAX ||
        (name == "natural" && value.value < 0) || (name == "positive" && value.value < 1))
      fail("static function value violates its subtype");
    if (type.constraint) {
      if (type.constraint->attribute) fail("scalar subtype cannot use an array range attribute");
      Range bounds{staticInteger(*type.constraint->leftExpression, scope),
          staticInteger(*type.constraint->rightExpression, scope), type.constraint->ascending};
      bounds.position(value.value);
    }
  }

  void validateStaticStatements(const std::vector<Statement>& statements, std::set<std::string> variables, size_t loopDepth = 0) {
    for (const auto& statement : statements) {
      DiagnosticScope location(statement.span);
      if (statement.kind == Statement::Kind::Assignment) {
        if (statement.assignment.kind != vhdl::AssignmentKind::Variable ||
            !statement.assignment.indices.empty() || !variables.contains(key(statement.assignment.target)))
          fail("function assignment requires a local scalar variable");
      }
      if (statement.kind == Statement::Kind::Exit && !loopDepth)
        fail("exit requires an enclosing function loop");
      if (statement.kind == Statement::Kind::Case)
        fail("case in static function bodies is not yet supported");
      auto nestedVariables = variables;
      if (statement.kind == Statement::Kind::For) nestedVariables.erase(key(statement.iterator));
      validateStaticStatements(statement.statements, std::move(nestedVariables),
          loopDepth + (statement.kind == Statement::Kind::For));
      validateStaticStatements(statement.alternative, variables, loopDepth);
    }
  }

  struct StaticFlow {
    std::optional<StaticScalar> returned;
    bool exited = false;
    explicit operator bool() const { return returned.has_value() || exited; }
  };

  StaticFlow staticStatements(const std::vector<Statement>& statements, StaticScope& scope) {
    for (const auto& statement : statements) {
      DiagnosticScope location(statement.span);
      if (++steps_ > 100000) fail("static function evaluation limit exceeded");
      if (statement.kind == Statement::Kind::Null) continue;
      if (statement.kind == Statement::Kind::Return) return {scalar(*statement.returnValue, &scope)};
      if (statement.kind == Statement::Kind::Exit) {
        if (!statement.condition) return {std::nullopt, true};
        const auto condition = scalar(*statement.condition, &scope);
        if (!condition.boolean) fail("exit condition must be boolean");
        if (condition.value) return {std::nullopt, true};
        continue;
      }
      if (statement.kind == Statement::Kind::If) {
        const auto condition = scalar(*statement.condition, &scope);
        if (!condition.boolean) fail("function if condition must be boolean");
        auto result = staticStatements(condition.value ? statement.statements : statement.alternative, scope);
        if (result) return result;
      } else if (statement.kind == Statement::Kind::For) {
        if (statement.range.attribute) fail("array range attributes require vector function evaluation");
        const Range bounds{staticInteger(*statement.range.leftExpression, &scope),
            staticInteger(*statement.range.rightExpression, &scope), statement.range.ascending};
        const auto name = key(statement.iterator);
        const auto previous = scope.values.find(name);
        const std::optional<StaticScalar> saved = previous == scope.values.end() ? std::nullopt :
            std::optional<StaticScalar>(previous->second);
        const auto variable = scope.variables.extract(name);
        const auto restore = [&] {
          if (saved) scope.values[name] = *saved;
          else scope.values.erase(name);
          if (!variable.empty()) scope.variables.emplace(name, variable.mapped());
        };
        for (size_t i = 0; i < bounds.size(); ++i) {
          if (++steps_ > 100000) fail("static function evaluation limit exceeded");
          scope.values[name] = {bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i), false};
          auto result = staticStatements(statement.statements, scope);
          if (result.returned) { restore(); return result; }
          if (result.exited) break;
        }
        restore();
      } else if (statement.kind == Statement::Kind::Assignment) {
        const auto& assignment = statement.assignment;
        const auto name = key(assignment.target);
        if (assignment.kind != vhdl::AssignmentKind::Variable || !assignment.indices.empty() ||
            !scope.variables.contains(name)) fail("function assignment requires a local scalar variable");
        const auto value = scalar(*assignment.value, &scope);
        checkStaticType(value, *scope.variables.at(name), &scope);
        scope.values[name] = value;
      } else fail("unsupported static function statement");
    }
    return {};
  }

  StaticScalar callStatic(const Expr& call, StaticScope* caller) {
    const auto& prefix = call.kind == Expr::Kind::Name ? call : *call.left;
    if (prefix.kind != Expr::Kind::Name) fail("function prefix must name a visible package function");
    if ((!caller && (integers_.contains(key(prefix)) || booleans_.contains(key(prefix)) || objects_.contains(key(prefix)))) ||
        (caller && caller->values.contains(key(prefix)))) fail("function name is hidden by a local object");
    const auto binding = staticFunction(key(prefix), caller);
    if (!binding.second) fail("no visible static package function: " + key(prefix));
    const auto& declaration = *binding.second;
    if (libraryContext_.precedes(call.span.start.offset, declaration.span.start.offset))
      fail("function is used before its declaration: " + key(prefix));
    if (!declaration.pure) fail("impure package functions are unsupported");
    if (staticDepth_ >= 64) fail("static function recursion limit exceeded");
    ++staticDepth_;
    struct DepthGuard { size_t& depth; ~DepthGuard() { --depth; } } depth{staticDepth_};
    const auto& body = functionBody(binding);
    StaticScope local;
    local.package = binding.first;
    if (!binding.first) local.environment = &localFunctionEnvironments_.at(&declaration);
    std::vector<const vhdl::GenericDeclaration*> formals;
    std::vector<std::string> names;
    for (const auto& parameter : declaration.parameters) for (const auto& name : parameter.names) {
      if (std::find(names.begin(), names.end(), key(name)) != names.end()) fail("duplicate function parameter");
      names.push_back(key(name));
      formals.push_back(&parameter);
    }
    std::vector<const Expr*> arguments;
    if (call.kind == Expr::Kind::Indexed) arguments.push_back(call.right.get());
    else if (call.kind == Expr::Kind::Call)
      for (const auto& argument : call.elements) arguments.push_back(argument.get());
    std::vector<std::optional<StaticScalar>> values(names.size());
    bool named = false;
    size_t position = 0;
    for (const auto* argument : arguments) {
      size_t destination = position++;
      if (argument->kind == Expr::Kind::Association) {
        named = true;
        if (argument->left->kind != Expr::Kind::Name) fail("function formal must be a name");
        const auto found = std::find(names.begin(), names.end(), key(*argument->left));
        destination = found - names.begin();
        argument = argument->right.get();
      } else if (named) fail("positional function actual follows named association");
      if (destination >= names.size() || values[destination]) fail("invalid or duplicate function actual");
      values[destination] = scalar(*argument, caller);
    }
    for (size_t i = 0; i < names.size(); ++i) {
      if (!values[i]) {
        if (!formals[i]->defaultValue) fail("missing function argument: " + names[i]);
        // Defaults are evaluated in the declaration scope, never in the caller.
        StaticScope declarationScope;
        declarationScope.package = binding.first;
        declarationScope.environment = local.environment;
        values[i] = scalar(*formals[i]->defaultValue, &declarationScope);
      }
      checkStaticType(*values[i], formals[i]->type, &local);
      local.values.emplace(names[i], *values[i]);
    }
    std::map<size_t, std::pair<const vhdl::ObjectDeclaration*, const Expr*>> objects;
    for (const auto& variable : body.variables)
      objects.emplace(variable.span.start.offset, std::make_pair(&variable, variable.initializer.get()));
    for (const auto& constant : body.constants)
      objects.emplace(constant.object.span.start.offset, std::make_pair(&constant.object, constant.value.get()));
    for (const auto& [offset, entry] : objects) {
      const auto& object = *entry.first;
      StaticScalar value;
      const auto type = key(object.type.name);
      if (object.type.constraint && object.type.constraint->attribute)
        fail("array range attributes require vector function evaluation");
      if (entry.second) value = scalar(*entry.second, &local);
      else {
        value.boolean = type == "boolean";
        value.value = object.type.constraint ? staticInteger(*object.type.constraint->leftExpression, &local) :
            type == "integer" ? INT32_MIN : type == "positive" ? 1 : 0;
      }
      checkStaticType(value, object.type, &local);
      const bool variable = std::any_of(body.variables.begin(), body.variables.end(),
          [&](const auto& candidate) { return &candidate == &object; });
      for (const auto& name : object.names) {
        if (!local.values.emplace(key(name), value).second) fail("duplicate function local declaration");
        if (variable) local.variables.emplace(key(name), &object.type);
      }
    }
    std::set<std::string> variables;
    for (const auto& [name, type] : local.variables) variables.insert(name);
    validateStaticStatements(body.statements, std::move(variables));
    const auto result = staticStatements(body.statements, local);
    if (!result.returned) fail("function completed without returning a value: " + key(prefix));
    checkStaticType(*result.returned, declaration.returnType, &local);
    return *result.returned;
  }

  bool staticBooleanType(const Expr& expression, const StaticScope* scope) const {
    const auto scalarType = [](const std::string& name) {
      if (name == "boolean") return true;
      if (name == "integer" || name == "natural" || name == "positive") return false;
      fail("unsupported static scalar type: " + name);
    };
    if (expression.kind == Expr::Kind::IntegerLiteral) return false;
    if (expression.kind == Expr::Kind::Name) {
      const auto name = key(expression);
      if (scope) {
        if (const auto found = scope->values.find(name); found != scope->values.end()) return found->second.boolean;
        if (scope->environment) {
          const auto found = scope->environment->values.find(name);
          if (found != scope->environment->values.end()) return found->second.boolean;
        }
        if (scope->package) for (const auto& constant : scope->package->constants)
          for (const auto& id : constant.object.names)
            if (key(id) == name) return scalarType(key(constant.object.type.name));
      } else {
        if (integers_.contains(name)) return false;
        if (booleans_.contains(name)) return true;
        if (const auto found = objects_.find(name); found != objects_.end() &&
            found->second.value.shape.enumeration) return false;
      }
      if (name == "true" || name == "false") return true;
      if (const auto function = staticFunction(name, scope); function.second)
        return scalarType(key(function.second->returnType.name));
    } else if ((expression.kind == Expr::Kind::Call || expression.kind == Expr::Kind::Indexed) &&
               expression.left->kind == Expr::Kind::Name) {
      if (!scope && integerTables_.contains(key(*expression.left))) return false;
      if (const auto function = staticFunction(key(*expression.left), scope); function.second)
        return scalarType(key(function.second->returnType.name));
    } else if (expression.kind == Expr::Kind::Unary) {
      const bool operand = staticBooleanType(*expression.left, scope);
      if (expression.text == "not" && operand) return true;
      if ((expression.text == "+" || expression.text == "-" || expression.text == "abs") && !operand) return false;
      fail("static unary operand type mismatch");
    } else if (expression.kind == Expr::Kind::Binary) {
      const bool left = staticBooleanType(*expression.left, scope);
      const bool right = staticBooleanType(*expression.right, scope);
      if (left != right) fail("static operand type mismatch");
      const auto& op = expression.text;
      if (op == "=" || op == "/=" || op == "<" || op == "<=" || op == ">" || op == ">=") return true;
      if (op == "and" || op == "or" || op == "nand" || op == "nor" || op == "xor" || op == "xnor") {
        if (!left) fail("static logical operands must be boolean");
        return true;
      }
      if (!left && (op == "+" || op == "-" || op == "*" || op == "/" || op == "mod" || op == "rem" || op == "**"))
        return false;
    }
    fail("unsupported static scalar expression type");
  }

  StaticScalar scalar(const Expr& expression, StaticScope* scope = nullptr) {
    DiagnosticScope location(expression.span);
    if (++steps_ > 100000) fail("static expression evaluation limit exceeded");
    if (expression.kind == Expr::Kind::Name) {
      const auto name = key(expression);
      if (scope) {
        if (const auto found = scope->values.find(name); found != scope->values.end()) return found->second;
        if (scope->environment) {
          const auto found = scope->environment->values.find(name);
          if (found != scope->environment->values.end()) return found->second;
        }
        if (scope->package) for (const auto& declaration : scope->package->constants)
          for (const auto& id : declaration.object.names) if (key(id) == name) {
            if (libraryContext_.precedes(expression.span.start.offset, declaration.object.span.start.offset))
              fail("package constant used before its declaration");
            if (!activeStaticConstants_.insert(&declaration).second) fail("cyclic static constant");
            StaticScope packageScope;
            packageScope.package = scope->package;
            const auto value = scalar(*declaration.value, &packageScope);
            checkStaticType(value, declaration.object.type, &packageScope);
            activeStaticConstants_.erase(&declaration);
            return value;
          }
      } else {
        if (const auto found = integers_.find(name); found != integers_.end()) return {found->second, false};
        if (const auto found = booleans_.find(name); found != booleans_.end()) return {found->second, true};
        if (const auto found = objects_.find(name); found != objects_.end() && found->second.constant &&
            found->second.value.shape.enumeration) {
          int64_t value = 0;
          for (size_t i = 0; i < found->second.value.bits.size(); ++i) {
            const auto bit = constantValue(found->second.value.bits[i]);
            if (!bit) fail("enumeration literal is not static");
            if (*bit) value |= int64_t(1) << i;
          }
          return {value, false};
        }
      }
      if (name == "true" || name == "false") return {name == "true", true};
      if (staticFunction(name, scope).second) return callStatic(expression, scope);
    } else if ((expression.kind == Expr::Kind::Indexed || expression.kind == Expr::Kind::Call) &&
               expression.left->kind == Expr::Kind::Name) {
      const auto builtin = key(*expression.left);
      const auto& arguments = expression.kind == Expr::Kind::Indexed ?
          std::vector<const Expr*>{expression.right.get()} : [&] { std::vector<const Expr*> result; for (const auto& e : expression.elements) result.push_back(e.get()); return result; }();
      if (builtin == "index_size_f" && arguments.size() == 1) {
        const auto n = scalar(*arguments[0], scope);
        if (n.boolean || n.value < 1) fail("index_size_f requires a positive integer");
        int64_t width = 0; for (int64_t v = n.value - 1; v; v >>= 1) ++width;
        return {std::max<int64_t>(1, width), false};
      }
      if (builtin == "sel_natural_f" && arguments.size() == 3) {
        const auto condition = scalar(*arguments[0], scope);
        if (!condition.boolean) fail("sel_natural_f condition must be boolean");
        const auto selected = scalar(*(condition.value ? arguments[1] : arguments[2]), scope);
        if (selected.boolean) fail("sel_natural_f branch must be natural");
        return selected;
      }
      if (expression.kind == Expr::Kind::Indexed && key(*expression.left) == "boolean") {
        if (expression.elements.size() != 1 && !expression.right) fail("boolean conversion requires one argument");
        const auto& argument = expression.right ? *expression.right : *expression.elements.front();
        const auto value = scalar(argument, scope);
        if (value.boolean) return value;
        return {value.value != 0, true};
      }
      if (!scope && expression.kind == Expr::Kind::Indexed && integerTables_.contains(key(*expression.left))) {
        const auto& table = integerTables_.at(key(*expression.left));
        return {table.values.at(table.bounds.position(staticInteger(*expression.right, scope))), false};
      }
      return callStatic(expression, scope);
    } else if (expression.kind == Expr::Kind::IntegerLiteral) {
      std::string digits;
      for (char c : expression.text) if (c != '_') digits += c;
      int64_t value;
      const auto result = std::from_chars(digits.data(), digits.data() + digits.size(), value);
      if (result.ec == std::errc{} && result.ptr == digits.data() + digits.size()) return {value, false};
    } else if (expression.kind == Expr::Kind::Unary || expression.kind == Expr::Kind::Binary) {
      const auto left = scalar(*expression.left, scope);
      const auto& op = expression.text;
      int128_t value;
      if (expression.kind == Expr::Kind::Unary) {
        if (op == "not" && left.boolean) return {!left.value, true};
        if (left.boolean || (op != "+" && op != "-" && op != "abs")) fail("unsupported static unary operand");
        value = left.value;
        if (op == "-" || (op == "abs" && value < 0)) value = -value;
      } else {
        if (left.boolean && (((op == "and" || op == "nand") && !left.value) ||
            ((op == "or" || op == "nor") && left.value))) {
          if (!staticBooleanType(*expression.right, scope)) fail("static operand type mismatch");
          return {op == "nand" || op == "or", true};
        }
        const auto right = scalar(*expression.right, scope);
        if (left.boolean != right.boolean) fail("static operand type mismatch");
        if (op == "=") return {left.value == right.value, true};
        if (op == "/=") return {left.value != right.value, true};
        if (op == "<") return {left.value < right.value, true};
        if (op == "<=") return {left.value <= right.value, true};
        if (op == ">") return {left.value > right.value, true};
        if (op == ">=") return {left.value >= right.value, true};
        if (left.boolean) {
          if (op == "and" || op == "nand") return {(left.value && right.value) != (op == "nand"), true};
          if (op == "or" || op == "nor") return {(left.value || right.value) != (op == "nor"), true};
          if (op == "xor" || op == "xnor") return {(left.value != right.value) != (op == "xnor"), true};
          fail("unsupported static boolean operator");
        }
        const int128_t a = left.value, b = right.value;
        if (op == "+") value = a + b;
        else if (op == "-") value = a - b;
        else if (op == "*") value = a * b;
        else if (op == "/" || op == "mod" || op == "rem") {
          if (!b) fail("division by zero in static expression");
          value = op == "/" ? a / b : a % b;
          if (op == "mod" && value != 0 && ((value < 0) != (b < 0))) value += b;
        } else if (op == "**") {
          if (b < 0 || b > 65536) fail("unsupported static exponent");
          value = 1;
          for (int64_t i = 0; i < right.value; ++i) {
            if (++steps_ > 100000) fail("static expression evaluation limit exceeded");
            value *= a;
            if (value < std::numeric_limits<int64_t>::min() || value > std::numeric_limits<int64_t>::max())
              fail("static integer overflow");
          }
        } else fail("unsupported static integer operator");
      }
      if (value < std::numeric_limits<int64_t>::min() || value > std::numeric_limits<int64_t>::max())
        fail("static integer overflow");
      return {static_cast<int64_t>(value), false};
    }
    fail("expression requires a supported static integer or boolean value");
  }

  int64_t integer(const Expr& expression) {
    return staticInteger(expression, nullptr);
  }

  const vhdl::EnumerationTypeDeclaration* enumIndexType(const Expr& expr) const {
    if (expr.kind != Expr::Kind::Name) return nullptr;
    const auto found = objects_.find(key(expr));
    if (found == objects_.end() || !found->second.value.shape.ranges.empty()) return nullptr;
    return found->second.value.shape.enumeration;
  }

  void checkIndexType(const Value& value, const Range& bounds) {
    if (bounds.indexEnumeration) {
      if (!value.shape.ranges.empty() || value.shape.enumeration != bounds.indexEnumeration)
        fail("array index enumeration type mismatch");
    } else if (!value.shape.integerWidth) fail("array index must have integer type");
  }

  int64_t arrayIndex(const Expr& expr, const Range& bounds) {
    if (!bounds.indexEnumeration) return integer(expr);
    if (!isStaticIndex(expr)) fail("array selection requires a static enumeration index");
    const auto value = expression(expr, State{});
    checkIndexType(value, bounds);
    int64_t ordinal = 0;
    for (auto* bit : value.bits) {
      const auto binary = constantValue(bit);
      if (!binary) fail("array selection requires a static enumeration index");
      ordinal = ordinal * 2 + *binary;
    }
    if (ordinal >= int64_t(bounds.indexEnumeration->literals.size()))
      fail("invalid enumeration index value");
    return ordinal;
  }

  Range enumRange(const vhdl::DiscreteRange& constraint,
                  const vhdl::EnumerationTypeDeclaration* type) {
    if (constraint.attribute || !constraint.leftExpression || !constraint.rightExpression)
      fail("enumeration array bounds require explicit enumeration values");
    Range bounds{0, int64_t(type->literals.size()) - 1, true, type};
    return {arrayIndex(*constraint.leftExpression, bounds), arrayIndex(*constraint.rightExpression, bounds),
            constraint.ascending, type};
  }

  Range arrayBounds(const vhdl::ArrayTypeDeclaration& declaration) {
    if (declaration.indexType) {
      const auto found = enumerations_.find(key(*declaration.indexType));
      if (found == enumerations_.end()) fail("array index type must name a visible enumeration");
      return {0, int64_t(found->second->literals.size()) - 1, true, found->second};
    }
    if (declaration.indexSubtype) {
      const auto found = enumerations_.find(key(*declaration.indexSubtype));
      return {0, -1, true, found == enumerations_.end() ? nullptr : found->second};
    }
    if (declaration.indexRange.leftExpression) {
      if (const auto type = enumIndexType(*declaration.indexRange.leftExpression))
        return enumRange(declaration.indexRange, type);
    }
    return range(declaration.indexRange);
  }

  Range range(const vhdl::DiscreteRange& range) {
    DiagnosticScope location(range.span);
    if (range.attribute) fail("array range attributes are not supported in this elaboration context");
    return {range.leftExpression ? integer(*range.leftExpression) : range.left,
            range.rightExpression ? integer(*range.rightExpression) : range.right,
            range.ascending};
  }

  Shape shape(const vhdl::TypeMark& type) {
    DiagnosticScope location(type.name.span);
    auto name = key(type.name);
    if (stdLogic_ && name == "std_ulogic") name = "std_logic";
    if (stdLogic_ && name == "std_ulogic_vector") name = "std_logic_vector";
    if (const auto found = enumerations_.find(name); found != enumerations_.end()) {
      if (type.constraint) fail("enumeration subtype constraints are not supported");
      if (libraryContext_.precedes(type.name.span.start.offset, found->second->span.start.offset))
        fail("enumeration used before its declaration");
      return {{name}, {}, 0, {}, found->second};
    }
    if (const auto found = records_.find(name); found != records_.end()) {
      if (type.constraint) fail("record type cannot have an array constraint");
      if (libraryContext_.precedes(type.name.span.start.offset, recordOffsets_.at(name)))
        fail("record type is used before its declaration: " + name);
      return found->second;
    }
    if (const auto found = arrays_.find(name); found != arrays_.end()) {
      if (libraryContext_.precedes(type.name.span.start.offset, arrayTypeOffsets_.at(name)))
        fail("array type is used before its declaration: " + name);
      auto result = found->second;
      const auto& declaration = *arrayDeclarations_.at(name);
      if (declaration.indexSubtype) {
        if (!type.constraint) fail("unconstrained array object requires bounds");
        Expr unused;
        result.ranges.front() = constantBounds(declaration, type, unused);
      } else if (type.constraint) fail("reconstraining an array type is not supported");
      return result;
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
        (numeric_ && (name == "unsigned" || name == "signed"))) {
      if (!type.constraint) fail("vector requires a constraint");
      return {{name, name == "bit_vector" ? "bit" : "std_logic"}, {range(*type.constraint)}};
    }
    fail("unsupported or invisible RTL type: " + name);
  }

  void addObject(const vhdl::Name& name, const Shape& type, bool input = false,
                 bool output = false, bool variable = false, bool port = false) {
    const auto id = key(name);
    const auto hasEnumeration = [&](const auto& self, const Shape& shape) -> bool {
      if (shape.enumeration) return true;
      for (const auto& field : shape.fields) if (self(self, field.second)) return true;
      return false;
    };
    if (port && hasEnumeration(hasEnumeration, type)) fail("enumerated ports are not yet supported");
    if (port && (type.ranges.size() > 1 || type.integerWidth))
      fail("RTL ports require scalar logic or one-dimensional logic vectors");
    if (functions_.contains(id) || booleans_.contains(id) || objects_.contains(id) || integers_.contains(id) || (enumerations_.contains(id) || records_.contains(id)) || arrayDeclarations_.contains(id) || integerTables_.contains(id))
      fail("duplicate or shadowing object: " + id);
    Bits bits;
    const auto size = type.size();
    if (variable) bits.resize(size, nullptr);
    else if (!type.ranges.empty() || type.integerWidth || type.enumeration || !type.fields.empty()) {
      const auto r = (type.integerWidth || type.enumeration || !type.fields.empty()) ? Range{int64_t(size - 1), 0, false} : type.ranges.back();
      if (r.left < std::numeric_limits<NLID::Bit>::min() || r.left > std::numeric_limits<NLID::Bit>::max() ||
          r.right < std::numeric_limits<NLID::Bit>::min() || r.right > std::numeric_limits<NLID::Bit>::max())
        fail("hardware vector bounds exceed net index range");
      // Keep each innermost array element as a bus, with its declared bounds.
      for (size_t offset = 0; offset < size; offset += r.size()) {
        auto busName = objectPrefix_ + name.spelling;
        auto position = offset;
        auto stride = size;
        for (size_t dimension = 0; !type.enumeration && type.fields.empty() && dimension + 1 < type.ranges.size(); ++dimension) {
          const auto& bounds = type.ranges[dimension];
          stride /= bounds.size();
          const auto index = static_cast<int64_t>(position / stride);
          busName += "(" + std::to_string(bounds.ascending ? bounds.left + index : bounds.left - index) + ")";
          position %= stride;
        }
        auto* net = SNLBusNet::create(design_, r.left, r.right, NLName(busName));
        for (size_t i = 0; i < r.size(); ++i) bits.push_back(net->getBitAtPosition(i));
        if (port) SNLBusTerm::create(design_, input ? SNLTerm::Direction::Input :
            SNLTerm::Direction::Output, r.left, r.right, NLName(name.spelling))->setNet(net);
      }
    } else {
      for (size_t i = 0; i < size; ++i) {
        auto* bit = SNLScalarNet::create(design_, NLName(objectPrefix_ + name.spelling +
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
  void field(Selection& selected, const std::string& name) {
    if (!selected.shape.ranges.empty() || selected.shape.fields.empty())
      fail("selected prefix is not a record");
    size_t offset = 0;
    for (const auto& [fieldName, type] : selected.shape.fields) {
      if (fieldName == name) {
        auto result = type;
        selected.offset += offset;
        selected.shape = std::move(result);
        return;
      }
      offset += type.size();
    }
    fail("no record field: " + name);
  }
  void index(Selection& selected, const Expr& expression) {
    if (expression.kind == Expr::Kind::Selected) {
      field(selected, key(expression));
      return;
    }
    if (selected.shape.ranges.empty()) fail("cannot index a scalar object");
    if (expression.kind == Expr::Kind::Range) {
      const auto& original = selected.shape.ranges.front();
      Range slice{arrayIndex(*expression.left, original), arrayIndex(*expression.right, original),
                  expression.text == "to", original.indexEnumeration};
      if (slice.ascending != original.ascending || !slice.size()) fail("invalid slice direction or range");
      const auto position = original.position(slice.left);
      original.position(slice.right);
      const auto stride = selected.shape.size() / original.size();
      selected.offset += position * stride;
      selected.shape.ranges.front() = slice;
      return;
    }
    const auto position = selected.shape.ranges.front().position(arrayIndex(expression, selected.shape.ranges.front()));
    selected.shape.ranges.erase(selected.shape.ranges.begin());
    selected.shape.removeOuterType();
    selected.offset += position * selected.shape.size();
  }
  Selection selection(const Expr& expression) {
    if (expression.kind == Expr::Kind::Name) return selection(key(expression));
    if (expression.kind == Expr::Kind::Selected) {
      auto selected = selection(*expression.left);
      field(selected, key(expression));
      return selected;
    }
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
  Bits mux(SNLBitNet* condition, const Bits& yes, const Bits& no) {
    if (auto value = constantValue(condition)) return *value ? yes : no;
    Bits result(yes.size());
    for (size_t first = 0; first < yes.size();) {
      if (yes[first] == no[first] || !yes[first] || !no[first]) {
        // Preserve unavailable variables and avoid muxes for unchanged bits.
        result[first] = yes[first] == no[first] ? yes[first] : nullptr;
        ++first;
        continue;
      }
      size_t end = first + 1;
      while (end < yes.size() && yes[end] && no[end] && yes[end] != no[end]) ++end;
      const auto width = end - first;
      SNLNet* output = width == 1 ? static_cast<SNLNet*>(SNLScalarNet::create(design_))
          : SNLBusNet::create(design_, width - 1, 0);
      // RTL values follow declaration order; canonical primitive inputs are LSB first.
      Bits a(no.begin() + first, no.begin() + end), b(yes.begin() + first, yes.begin() + end);
      std::reverse(a.begin(), a.end());
      std::reverse(b.begin(), b.end());
      SNLRTLPrimitives::createMux(design_, condition, a, b, output);
      for (size_t i = first; i < end; ++i)
        result[i] = width == 1 ? static_cast<SNLBitNet*>(output)
            : static_cast<SNLBusNet*>(output)->getBitAtPosition(i - first);
      first = end;
    }
    return result;
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

  bool isStaticIndex(const Expr& expr) const {
    return isStatic(expr) || (expr.kind == Expr::Kind::Name && enumIndexType(expr) && objects_.at(key(expr)).constant);
  }

  bool isStatic(const Expr& expr) const {
    if (expr.kind == Expr::Kind::IntegerLiteral) return true;
    if (expr.kind == Expr::Kind::Name) return integers_.contains(key(expr)) || booleans_.contains(key(expr)) ||
        functions_.contains(key(expr)) || key(expr) == "true" || key(expr) == "false";
    if ((expr.kind == Expr::Kind::Indexed || expr.kind == Expr::Kind::Call) &&
        expr.left->kind == Expr::Kind::Name && functions_.contains(key(*expr.left))) {
      if (expr.kind == Expr::Kind::Indexed) return isStatic(*expr.right);
      return std::all_of(expr.elements.begin(), expr.elements.end(), [&](const auto& argument) {
        return isStatic(argument->kind == Expr::Kind::Association ? *argument->right : *argument);
      });
    }
    if (expr.kind == Expr::Kind::Indexed && expr.left->kind == Expr::Kind::Name &&
        integerTables_.contains(key(*expr.left))) return isStatic(*expr.right);
    return (expr.kind == Expr::Kind::Unary || expr.kind == Expr::Kind::Binary) &&
        (expr.text == "+" || expr.text == "-" || expr.text == "*" || expr.text == "/" ||
         expr.text == "mod" || expr.text == "rem" || expr.text == "**" || expr.text == "abs" ||
         expr.text == "=" || expr.text == "/=" || expr.text == "<" || expr.text == "<=" ||
         expr.text == ">" || expr.text == ">=" || expr.text == "not" || expr.text == "and" ||
         expr.text == "or" || expr.text == "xor" || expr.text == "nand" || expr.text == "nor" || expr.text == "xnor") &&
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
    if (memories_.contains(name)) {
      Value value{object.value.shape, {}};
      const auto& bounds = value.shape.ranges.front();
      for (size_t i = 0; i < bounds.size(); ++i) {
        Expr index;
        index.kind = Expr::Kind::IntegerLiteral;
        index.text = std::to_string(bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i));
        const auto word = readMemory(name, index, state);
        value.bits.insert(value.bits.end(), word.bits.begin(), word.bits.end());
      }
      return value;
    }
    if (object.output) fail("reading an out port is outside the RTL profile");
    Value value = object.value;
    if (object.variable) value.bits = state.variables.at(name);
    for (auto* bit : value.bits) {
      if (!bit) fail("variable read before definite assignment: " + name);

    }
    if (sensitivity_ && object.variable && !combinational_) fail("variable read outside the clock guard is not supported");
    return value;
  }
  Value readIndex(Value value, const Expr& indexExpr, const State& state) {
    if (value.shape.ranges.empty()) fail("cannot index a scalar object");
    const auto bounds = value.shape.ranges.front();
    const auto stride = value.shape.size() / bounds.size();
    if (indexExpr.kind == Expr::Kind::Range) {
      Range slice{arrayIndex(*indexExpr.left, bounds), arrayIndex(*indexExpr.right, bounds),
                  indexExpr.text == "to", bounds.indexEnumeration};
      if (!slice.size() || slice.ascending != bounds.ascending) fail("invalid slice range");
      const auto offset = bounds.position(slice.left) * stride;
      bounds.position(slice.right);
      value.shape.ranges.front() = slice;
      value.bits = Bits(value.bits.begin() + offset, value.bits.begin() + offset + value.shape.size());
      return value;
    }
    value.shape.ranges.erase(value.shape.ranges.begin());
    value.shape.removeOuterType();
    if (isStaticIndex(indexExpr)) {
      const auto offset = bounds.position(arrayIndex(indexExpr, bounds)) * stride;
      value.bits = Bits(value.bits.begin() + offset, value.bits.begin() + offset + stride);
      return value;
    }
    for (auto* bit : value.bits) readBit(bit);
    auto address = expression(indexExpr, state);
    checkIndexType(address, bounds);
    Bits result(stride, constant(false));
    for (size_t i = 0; i < bounds.size(); ++i) {
      auto index = bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i);
      if (index < 0) fail("dynamic indexing of negative bounds is unsupported");
      const auto width = std::max<size_t>(address.bits.size(), 32);
      auto* select = equal(resize(address.bits, width), number(index, {{"integer"}, {}, width}).bits);
      result = mux(select, Bits(value.bits.begin() + i * stride,
          value.bits.begin() + (i + 1) * stride), result);
    }
    value.bits = std::move(result);
    return value;
  }

  Value readSelected(const Expr& expr, const State& state) {
    if (expr.kind == Expr::Kind::Name) {
      if (!objects_.contains(key(expr))) fail("no declaration for object: " + key(expr));
      return readObject(key(expr), state);
    }
    if (expr.kind == Expr::Kind::Selected) {
      auto value = readSelected(*expr.left, state);
      Selection selected{"", value.shape, 0};
      field(selected, key(expr));
      return {selected.shape, Bits(value.bits.begin() + selected.offset,
          value.bits.begin() + selected.offset + selected.shape.size())};
    }
    if (expr.kind != Expr::Kind::Indexed) fail("indexed prefix must name an object");
    if (expr.left->kind == Expr::Kind::Name && memories_.contains(key(*expr.left)) &&
        expr.right->kind != Expr::Kind::Range)
      return readMemory(key(*expr.left), *expr.right, state);
    return readIndex(readSelected(*expr.left, state), *expr.right, state);
  }

  // Operands are extended to the result width before multiplying. Modulo
  // 2**width multiplication then serves both unsigned and two's-complement bits.
  Bits multiply(const Bits& left, const Bits& right) {
    const auto width = left.size();
    Bits result(width, constant(false));
    for (size_t shift = 0; shift < width; ++shift) {
      Bits row(width, constant(false));
      for (size_t i = shift; i < width; ++i)
        row[width - 1 - i] = gate("and", left[width - 1 - i + shift], right[width - 1 - shift]);
      result = add(result, row, false);
    }
    return result;
  }

  Value expression(const Expr& expr, const State& state, const Shape* expected = nullptr) {
    DiagnosticScope location(expr.span);
    Value value;
    Shape inferred;
    if (isStatic(expr)) {
      const auto result = scalar(expr);
      const Shape type = result.boolean ? Shape{{"boolean"}, {}} :
          expected ? *expected : Shape{{"integer"}, {}, 32};
      if (!result.boolean && !type.integerWidth && !(unsigned_ && type.types.front() == "std_logic_vector"))
        fail("integer literal is incompatible with target");
      value = number(result.value, type);
    } else if (expr.kind == Expr::Kind::Name) {
      const auto name = key(expr);
      if (!objects_.contains(name)) fail("no declaration for object: " + name);
      value = readObject(name, state);
      for (auto* bit : value.bits) readBit(bit);
    } else if (expr.kind == Expr::Kind::Selected) {
      value = readSelected(expr, state);
      for (auto* bit : value.bits) readBit(bit);
    } else if (expected && !expected->fields.empty() && expected->ranges.empty() &&
               (expr.kind == Expr::Kind::Aggregate || expr.kind == Expr::Kind::Others)) {
      value.shape = *expected;
      std::vector<const Expr*> actuals(expected->fields.size(), nullptr);
      if (expr.kind == Expr::Kind::Others) {
        std::fill(actuals.begin(), actuals.end(), expr.left.get());
      } else {
        size_t position = 0;
        bool named = false;
        for (const auto& element : expr.elements) {
          size_t destination = position++;
          const Expr* actual = element.get();
          if (element->kind == Expr::Kind::Association) {
            named = true;
            if (element->left->kind != Expr::Kind::Name) fail("record association must name a field");
            destination = 0;
            while (destination < expected->fields.size() &&
                expected->fields[destination].first != key(*element->left)) ++destination;
            actual = element->right.get();
          } else if (named) fail("positional record element follows named association");
          if (destination >= actuals.size() || actuals[destination]) fail("invalid or duplicate record association");
          actuals[destination] = actual;
        }
      }
      for (size_t i = 0; i < actuals.size(); ++i) {
        if (!actuals[i]) fail("missing record aggregate field");
        auto element = expression(*actuals[i], state, &expected->fields[i].second);
        value.bits.insert(value.bits.end(), element.bits.begin(), element.bits.end());
      }
    } else if (expr.kind == Expr::Kind::Call) {
      if (expr.left->kind == Expr::Kind::Name && key(*expr.left) == "sel_suv_f" &&
          expr.elements.size() == 3) {
        const auto condition = scalar(*expr.elements[0]);
        if (!condition.boolean) fail("sel_suv_f condition must be boolean");
        value = expression(*expr.elements[condition.value ? 1 : 2], state, expected);
      } else if (expr.left->kind != Expr::Kind::Name || key(*expr.left) != "to_unsigned" ||
          !numeric_ || expr.elements.size() != 2)
        fail("unsupported or invisible function call");
      else {
      if (objects_.contains("to_unsigned") || integers_.contains("to_unsigned") ||
          arrayDeclarations_.contains("to_unsigned") || integerTables_.contains("to_unsigned"))
        fail("shadowed to_unsigned function");
      const auto argument = integer(*expr.elements[0]);
      const auto width = integer(*expr.elements[1]);
      if (argument < 0 || argument > INT32_MAX || width < 1 || width > 65536)
        fail("to_unsigned requires a static natural argument and a supported positive size");
      auto type = vectorShape(width);
      type.types.front() = "unsigned";
      value = number(argument, type);
      }
    } else if (expr.kind == Expr::Kind::Indexed) {
      if (expr.left->kind == Expr::Kind::Name &&
          (key(*expr.left) == "std_logic_vector" || key(*expr.left) == "unsigned" || key(*expr.left) == "signed")) {
        const auto target = key(*expr.left);
        if (!numeric_ || (target == "std_logic_vector" && !stdLogic_))
          fail("vector conversion requires visible numeric_std and std_logic_1164 types");
        value = expression(*expr.right, state);
        const auto source = value.shape.types.front();
        if (value.shape.ranges.size() != 1 || (source != "unsigned" && source != "signed" && source != "std_logic_vector"))
          fail("unsupported vector conversion operand");
        value.shape.types.front() = target;
      } else if (expr.left->kind == Expr::Kind::Name && key(*expr.left) == "conv_integer") {
        if (!unsigned_ || !arith_) fail("conv_integer requires std_logic_arith and std_logic_unsigned");
        value = expression(*expr.right, state);
        if (value.shape.types.front() != "std_logic_vector" || value.bits.size() > 31)
          fail("unsupported conv_integer argument");
        value.shape = {{"integer"}, {}, value.bits.size()};
      } else value = readSelected(expr, state);
      for (auto* bit : value.bits) readBit(bit);
    } else if (expr.kind == Expr::Kind::Conditional) {
      const auto condition = expression(*expr.condition, state);
      if (condition.shape.types != std::vector<std::string>{"boolean"}) fail("condition must be boolean");
      auto yes = expression(*expr.left, state, expected);
      auto no = expression(*expr.right, state, &yes.shape);
      if (!compatible(yes.shape, no.shape)) fail("conditional type mismatch");
      value.shape = yes.shape;
      value.bits = mux(condition.bits.front(), yes.bits, no.bits);
    } else if (expr.kind == Expr::Kind::Aggregate) {
      if (!expected || expected->ranges.empty() || expr.elements.size() != expected->ranges.front().size())
        fail("positional aggregate length mismatch");
      value.shape = *expected;
      auto element = *expected;
      element.removeOuterType();
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
        element.removeOuterType();
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
      if (expr.text != "not" || value.shape.integerWidth || value.shape.enumeration || !value.shape.fields.empty())
        fail("unsupported hardware unary operator");
      for (auto*& bit : value.bits) bit = gate("not", bit);
    } else if (expr.kind == Expr::Kind::Binary) {
      const bool comparison = expr.text == "=" || expr.text == "/=";
      const bool arithmetic = expr.text == "+" || expr.text == "-";
      if (expr.text == "&") {
        auto left = expression(*expr.left, state);
        auto right = expression(*expr.right, state);
        if (left.shape.enumeration || right.shape.enumeration || !left.shape.fields.empty() || !right.shape.fields.empty() || left.shape.integerWidth || right.shape.integerWidth || left.shape.ranges.size() > 1 ||
            right.shape.ranges.size() > 1 || left.shape.types.back() != right.shape.types.back())
          fail("unsupported concatenation operands");
        value.bits = left.bits;
        value.bits.insert(value.bits.end(), right.bits.begin(), right.bits.end());
        value.shape = vectorShape(value.bits.size(), left.shape.types.back());
        for (const auto* numericType : {"unsigned", "signed"}) {
          if (left.shape.types.front() == numericType || right.shape.types.front() == numericType) {
            if ((left.shape.ranges.size() && left.shape.types.front() != numericType) ||
                (right.shape.ranges.size() && right.shape.types.front() != numericType))
              fail("concatenation operand type mismatch");
            value.shape.types.front() = numericType;
          }
        }
      } else {
        const bool numericOperation = (numeric_ || signed_) && (arithmetic || expr.text == "*" || comparison);
        auto left = expression(*expr.left, state, numericOperation || comparison ? nullptr : expected);
        const bool unsignedOperands = numeric_ && left.shape.types.front() == "unsigned";
        const bool numericSigned = numeric_ && left.shape.types.front() == "signed";
        const bool signedOperands = numericSigned || (signed_ && left.shape.types.front() == "std_logic_vector");
        if (signedOperands && !numericSigned && unsigned_ && (arithmetic || comparison || expr.text == "*"))
          fail("ambiguous signed/unsigned vector operators");
        const bool rightLiteral = expr.right->kind == Expr::Kind::StringLiteral ||
            expr.right->kind == Expr::Kind::BitStringLiteral || expr.right->kind == Expr::Kind::Others;
        auto right = expression(*expr.right, state,
            ((unsignedOperands || signedOperands) && !rightLiteral) || (comparison && left.shape.integerWidth) ? nullptr : &left.shape);
        if (signedOperands && (arithmetic || comparison || expr.text == "*")) {
          if (right.shape.types.front() != left.shape.types.front()) fail("signed vector operand type mismatch");
          const auto width = expr.text == "*" ? left.bits.size() + right.bits.size() :
              std::max(left.bits.size(), right.bits.size());
          left.bits.insert(left.bits.begin(), width - left.bits.size(), left.bits.front());
          right.bits.insert(right.bits.begin(), width - right.bits.size(), right.bits.front());
          value.shape = vectorShape(width);
          value.shape.types.front() = left.shape.types.front();
          if (comparison) {
            auto* result = equal(left.bits, right.bits);
            value = {{{"boolean"}, {}}, {expr.text == "=" ? result : gate("not", result)}};
          } else if (expr.text == "*") {
            value.bits = multiply(left.bits, right.bits);
          } else value.bits = add(left.bits, right.bits, expr.text == "-");
          if (expected && !compatible(value.shape, *expected)) fail("assignment or expression type/length mismatch");
          return value;
        }
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
            value.bits = multiply(left.bits, right.bits);
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
        if ((left.shape.enumeration || right.shape.enumeration) && !comparison)
          fail("enumeration operators currently support only equality and inequality");
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
          if (left.shape.ranges.size() > 1 || left.shape.integerWidth || !left.shape.fields.empty())
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
    DiagnosticScope location(assignment.span);
    auto target = selection(assignment);
    const auto& object = objects_.at(target.name);
    if (object.input || object.constant || object.variable || assignment.kind != vhdl::AssignmentKind::Signal)
      fail("concurrent assignment must drive a writable signal");
    Value value;
    if (assignment.selector) {
      const auto selector = expression(*assignment.selector, state);
      const auto matches = selectionMatches(selector, assignment.choices);
      value = expression(*assignment.alternatives.back(), state, &target.shape);
      for (size_t i = assignment.alternatives.size() - 1; i-- > 0;) {
        const auto branch = expression(*assignment.alternatives[i], state, &target.shape);
        value.bits = mux(matches[i], branch.bits, value.bits);
      }
    } else value = expression(*assignment.value, state, &target.shape);
    for (size_t i = 0; i < value.bits.size(); ++i) drive(object.value.bits[target.offset + i], value.bits[i]);
  }

  void assign(const vhdl::Assignment& assignment, State& state) {
    DiagnosticScope location(assignment.span);
    if (memories_.contains(key(assignment.target))) {
      const auto name = key(assignment.target);
      auto& write = state.memoryWrites.at(name);
      auto element = objects_.at(name).value.shape;
      element.ranges.erase(element.ranges.begin());
      element.removeOuterType();
      auto address = expression(*assignment.indices.front(), state);
      if (!address.shape.integerWidth) fail("array index must be integer");
      auto [bits, valid] = memoryAddress(name, std::move(address.bits));
      write.address = std::move(bits);
      write.data = expression(*assignment.value, state, &element).bits;
      write.enable = valid;
      write.assigned = true;
      return;
    }
    if (!assignment.indices.empty() && assignment.indices.front()->kind != Expr::Kind::Range &&
        assignment.indices.front()->kind != Expr::Kind::Selected && !isStaticIndex(*assignment.indices.front())) {
      const auto name = key(assignment.target);
      const auto base = selection(name);
      if (base.shape.ranges.empty()) fail("dynamic target is not an array");
      const auto& object = objects_.at(name);
      if (object.input || object.constant || object.variable || assignment.kind != vhdl::AssignmentKind::Signal)
        fail("dynamic assignment requires a writable signal array");
      const auto bounds = base.shape.ranges.front();
      auto element = base.shape;
      element.ranges.erase(element.ranges.begin());
      element.removeOuterType();
      const auto stride = element.size();
      const auto address = expression(*assignment.indices.front(), state);
      checkIndexType(address, bounds);
      Selection selected{name, element, 0};
      for (size_t j = 1; j < assignment.indices.size(); ++j) index(selected, *assignment.indices[j]);
      const auto value = expression(*assignment.value, state, &selected.shape);
      auto& bits = state.scheduled.at(name);
      for (size_t i = 0; i < bounds.size(); ++i) {
        const auto indexValue = bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i);
        const auto width = std::max<size_t>(32, address.bits.size());
        auto* condition = equal(resize(address.bits, width), number(indexValue, {{"integer"}, {}, width}).bits);
        const auto start = i * stride + selected.offset;
        const auto merged = mux(condition, value.bits,
            Bits(bits.begin() + start, bits.begin() + start + value.bits.size()));
        for (size_t j = 0; j < value.bits.size(); ++j) {
          const auto offset = start + j;
          bits[offset] = merged[j];
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
    for (auto& [name, write] : state.memoryWrites) {
      const auto& a = yes.memoryWrites.at(name);
      const auto& b = no.memoryWrites.at(name);
      // Address and data are don't-care on branches with no write.
      write.address = !a.assigned ? b.address : !b.assigned ? a.address : mux(condition, a.address, b.address);
      write.data = !a.assigned ? b.data : !b.assigned ? a.data : mux(condition, a.data, b.data);
      write.enable = mux(condition, {a.enable}, {b.enable}).front();
      write.assigned = a.assigned || b.assigned;
    }
    for (auto& [name, bits] : state.variables)
      bits = mux(condition, yes.variables.at(name), no.variables.at(name));
    for (auto& [name, bits] : state.scheduled) {
      bits = mux(condition, yes.scheduled.at(name), no.scheduled.at(name));
      for (size_t i = 0; i < bits.size(); ++i) {
        state.written.at(name)[i] = yes.written.at(name)[i] || no.written.at(name)[i];
      }
    }
  }

  std::vector<SNLBitNet*> selectionMatches(const Value& selector,
      const std::vector<std::vector<std::unique_ptr<Expr>>>& alternatives) {
    const auto& type = selector.shape;
    if (!type.fields.empty() || type.ranges.size() > 1 || (type.enumeration && !type.ranges.empty()) ||
        (!type.enumeration && !type.integerWidth && type.types.back() != "bit" && type.types.back() != "std_logic" &&
         type.types.back() != "boolean")) fail("unsupported case selector type");
    std::set<std::string> seen;
    std::vector<SNLBitNet*> matches;
    const bool otherwise = alternatives.back().empty();
    for (const auto& choices : alternatives) {
      auto* match = constant(false);
      const auto add = [&](const Value& choice) {
        if (seen.size() >= 4096) fail("case choice expansion limit exceeded");
        std::string bits;
        for (auto* bit : choice.bits) {
          const auto value = constantValue(bit);
          if (!value) fail("case choices must be static binary values");
          bits += *value ? '1' : '0';
        }
        if (!seen.insert(bits).second) fail("duplicate or overlapping case choices");
        match = gate("or", match, equal(selector.bits, choice.bits));
      };
      for (const auto& choice : choices) {
        const auto requireStatic = [&](const auto& self, const Expr& value) -> void {
          if (value.kind == Expr::Kind::Name && objects_.contains(key(value)) &&
              !objects_.at(key(value)).constant) fail("case choices must be static");
          if (value.left) self(self, *value.left);
          if (value.right && value.kind != Expr::Kind::Selected) self(self, *value.right);
          if (value.condition) self(self, *value.condition);
          for (const auto& element : value.elements) self(self, *element);
        };
        requireStatic(requireStatic, *choice);
        if (choice->kind == Expr::Kind::Range) {
          if (!type.integerWidth) fail("case ranges require an integer selector");
          const Range bounds{integer(*choice->left), integer(*choice->right), choice->text == "to"};
          if (bounds.size() > 4096) fail("case choice expansion limit exceeded");
          for (size_t i = 0; i < bounds.size(); ++i) {
            const auto value = bounds.ascending ? bounds.left + int64_t(i) : bounds.left - int64_t(i);
            if (value < 0 || (type.integerWidth < 63 && uint64_t(value) >= (uint64_t(1) << type.integerWidth)))
              fail("case integer choice is outside the supported selector range");
            add(number(value, type));
          }
        } else {
          if (type.integerWidth) {
            const auto value = integer(*choice);
            if (value < 0 || (type.integerWidth < 63 && uint64_t(value) >= (uint64_t(1) << type.integerWidth)))
              fail("case integer choice is outside the supported selector range");
          }
          add(expression(*choice, State{}, &type));
        }
      }
      matches.push_back(match);
    }
    if (!otherwise && (type.integerWidth || selector.bits.size() > 12 ||
        seen.size() != (type.enumeration ? type.enumeration->literals.size() : (size_t(1) << selector.bits.size()))))
      fail("case alternatives must cover every selector value; add others");
    return matches;
  }

  void caseStatement(const Statement& statement, State& state) {
    const auto selector = expression(*statement.condition, state);
    const auto matches = selectionMatches(selector, statement.choices);
    if (std::all_of(selector.bits.begin(), selector.bits.end(),
        [&](auto* bit) { return constantValue(bit).has_value(); })) {
      for (size_t i = 0; i < matches.size(); ++i) {
        if ((statement.choices[i].empty()) || constantValue(matches[i]).value_or(false)) {
          this->statements(statement.caseBodies[i], state);
          return;
        }
      }
    }
    // A proven exhaustive last alternative is a valid default, avoiding a
    // fictitious unassigned path in combinational definite-assignment checks.
    auto result = state;
    this->statements(statement.caseBodies.back(), result);
    for (size_t i = statement.caseBodies.size() - 1; i-- > 0;) {
      auto branch = state;
      this->statements(statement.caseBodies[i], branch);
      auto merged = state;
      merge(merged, branch, result, matches[i]);
      result = std::move(merged);
    }
    state = std::move(result);
  }

  void statements(const std::vector<Statement>& statements, State& state) {
    for (const auto& statement : statements) {
      DiagnosticScope location(statement.span);
      if (++steps_ > 100000) fail("static process elaboration limit exceeded");
      if (statement.kind == Statement::Kind::Null) continue;
      if (statement.kind == Statement::Kind::Case) caseStatement(statement, state);
      else if (statement.kind == Statement::Kind::Assignment) assign(statement.assignment, state);
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
      } else if (statement.kind == Statement::Kind::For) {
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
      } else fail("return is only supported inside functions");
    }
  }

  void readBit(SNLBitNet* bit) {
    if (sensitivity_ && sensitivity_->signals.contains(bit) && !sensitivity_->bits.contains(bit))
      fail("process sensitivity omits a signal bit read outside the clock guard: " + sensitivity_->signals.at(bit));
    reads_.insert(bit);
    if (processReads_) processReads_->insert(bit);
  }

  void lowerCombinationalProcess(const vhdl::ClockedProcess& process) {
    const auto sensitivity = processSensitivity(process);
    for (const auto& variable : process.variables) {
      if (variable.initializer) fail("combinational variable initialization is unsupported; assign in the body");
      const auto type = shape(variable.type);
      for (const auto& name : variable.names) addObject(name, type, false, false, true);
    }
    State state;
    for (const auto& [name, object] : objects_) {
      if (object.variable) state.variables[name] = object.value.bits;
      else if (!object.input && !object.constant && !memories_.contains(name)) {
        state.scheduled[name] = Bits(object.value.bits.size(), nullptr);
        state.written[name].resize(object.value.bits.size(), false);
      }
    }
    std::set<SNLBitNet*> processReads;
    processReads_ = &processReads;
    combinational_ = true;
    sensitivity_ = process.allSensitivity ? nullptr : &sensitivity;
    statements(process.statements, state);
    sensitivity_ = nullptr;
    combinational_ = false;
    processReads_ = nullptr;
    for (const auto& [name, written] : state.written) {
      const auto& values = state.scheduled.at(name);
      const auto& targets = objects_.at(name).value.bits;
      for (size_t i = 0; i < written.size(); ++i) if (written[i]) {
        if (!values[i]) fail("incomplete combinational assignment would infer a latch: " + name);
        if (processReads.contains(targets[i]))
          fail("reading a signal written by the same combinational process is unsupported: " + name);
        drive(targets[i], values[i]);
      }
    }
    for (const auto& variable : process.variables)
      for (const auto& name : variable.names) objects_.erase(key(name));
  }

  void lowerProcess(const vhdl::ClockedProcess& process) {
    DiagnosticScope location(process.span);
    if (process.combinational) { lowerCombinationalProcess(process); return; }
    const auto clockName = key(process.eventSignal);
    auto clock = selection(clockName);
    if (!objects_.at(clockName).input || clock.shape.size() != 1 ||
        !clock.shape.ranges.empty() || process.level != "'1'")
      fail("clock must name a scalar input with a positive edge");
    const auto sensitivity = processSensitivity(process);
    if (!sensitivity.names.contains(clockName)) fail("clock is absent from process sensitivity");
    if (key(process.levelSignal) != clockName) fail("clock event and level must match");
    if (process.asynchronousReset) {
      const auto name = key(*process.resetSignal);
      auto reset = selection(name);
      if (name == clockName || !sensitivity.names.contains(name))
        fail("asynchronous reset must be distinct from the clock and present in sensitivity");
      if (!reset.shape.ranges.empty() || reset.shape.size() != 1 ||
          (reset.shape.types != std::vector<std::string>{"bit"} &&
           reset.shape.types != std::vector<std::string>{"std_logic"}))
        fail("asynchronous reset must name a scalar logic signal");
      if (!process.assignments.empty())
        fail("assignments outside an asynchronous clock/reset guard are unsupported");
    }
    for (const auto& variable : process.variables) {
      const auto type = shape(variable.type);
      for (const auto& name : variable.names) addObject(name, type, false, false, true);
    }
    State state;
    for (const auto& [name, object] : objects_) {
      if (const auto memory = memories_.find(name); memory != memories_.end()) {
        if (memory->second.process == &process) {
          const auto& signature = memory->second.signature;
          state.memoryWrites[name] = {Bits(signature.abits, constant(false)),
              Bits(signature.width, constant(false)), constant(false), false};
        }
      } else if (object.variable) state.variables[name] = object.value.bits;
      else if (!object.input && !object.constant) {
        state.scheduled[name] = object.value.bits;
        state.written[name].resize(object.value.bits.size(), false);
      }
    }
    auto resetState = state;
    SNLBitNet* resetActive = nullptr;
    if (process.asynchronousReset) {
      resetActive = control(*process.resetSignal, process.resetLevel, state);
      statements(process.resetStatements, resetState);
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
    std::map<SNLBitNet*, bool> asynchronousValues;
    if (process.asynchronousReset) {
      for (auto& [name, bits] : state.scheduled) {
        const auto& targets = objects_.at(name).value.bits;
        for (size_t i = 0; i < bits.size(); ++i) {
          if (resetState.written.at(name)[i]) {
            const auto resetValue = constantValue(resetState.scheduled.at(name)[i]);
            if (!resetValue) fail("asynchronous reset assignments must produce constant binary values");
            asynchronousValues.emplace(targets[i], *resetValue);
            state.written.at(name)[i] = true;
          } else if (state.written.at(name)[i]) {
            // Bits omitted from the reset branch hold even on a clock edge
            // while reset is asserted. They do not acquire an invented reset.
            bits[i] = mux(resetActive, {targets[i]}, {bits[i]}).front();
          }
        }
      }
    }
    const auto resetKind = [&](SNLBitNet* net) {
      const auto found = asynchronousValues.find(net);
      return found == asynchronousValues.end() ? -1 : int(found->second);
    };
    size_t writes = 0;
    for (const auto& [name, write] : state.memoryWrites) {
      auto& memory = memories_.at(name);
      memory.write = write;
      memory.clock = objects_.at(clockName).value.bits.front();
      writes += write.assigned;
    }
    for (const auto& [name, bits] : state.scheduled) {
      const auto& targets = objects_.at(name).value.bits;
      const auto& written = state.written.at(name);
      for (size_t first = 0; first < bits.size();) {
        if (!written[first]) { ++first; continue; }
        const bool initialized = initialValues_.contains(targets[first]);
        const auto asynchronousKind = resetKind(targets[first]);
        auto* busBit = dynamic_cast<SNLBusNetBit*>(targets[first]);
        size_t end = first + 1;
        while (busBit && end < bits.size() && written[end] &&
               initialValues_.contains(targets[end]) == initialized &&
               resetKind(targets[end]) == asynchronousKind) {
          auto* next = dynamic_cast<SNLBusNetBit*>(targets[end]);
          if (!next || next->getBus() != busBit->getBus()) break;
          ++end;
        }
        const auto width = end - first;
        auto* model = asynchronousKind < 0 ? NLDB0::getOrCreateDFF(width) :
            asynchronousKind == 1 ? NLDB0::getOrCreateDFFS(width) :
            process.resetLevel == "'0'" ? NLDB0::getOrCreateDFFRN(width) : NLDB0::getOrCreateDFFR(width);
        auto* flop = SNLInstance::create(design_, model);
        flop->setTermNet(model->getScalarTerm(NLName("C")), objects_.at(clockName).value.bits.front());
        if (asynchronousKind >= 0) {
          const auto* pin = asynchronousKind == 1 ? "S" : process.resetLevel == "'0'" ? "RN" : "R";
          auto* reset = asynchronousKind == 0 && process.resetLevel == "'0'" ?
              objects_.at(key(*process.resetSignal)).value.bits.front() : resetActive;
          flop->setTermNet(model->getScalarTerm(NLName(pin)), reset);
        }
        std::string initialBits;
        for (size_t i = first; i < end; ++i) {
          auto* target = targets[i];
          if (!drivers_.insert(target).second) fail("multiple drivers for clocked signal");
          const auto position = end - 1 - i;
          SNLBitTerm* dataTerm = width == 1 ? static_cast<SNLBitTerm*>(model->getScalarTerm(NLName("D")))
              : model->getBusTerm(NLName("D"))->getBit(position);
          SNLBitTerm* outputTerm = width == 1 ? static_cast<SNLBitTerm*>(model->getScalarTerm(NLName("Q")))
              : model->getBusTerm(NLName("Q"))->getBit(position);
          flop->setTermNet(dataTerm, bits[i]);
          flop->setTermNet(outputTerm, target);
          if (initialized) {
            initialBits += initialValues_.at(target) ? '1' : '0';
            initializedFlops_.insert(target);
          }
          ++writes;
        }
        if (initialized)
          SNLInstParameter::create(flop, model->getParameter(NLName("INIT")),
              NLDB0::formatDFFInitValue(width, initialBits));
        first = end;
      }
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
  const LibraryContext& libraryContext_;
  NLLibrary* work_;
  GenericValues genericOverrides_;
  std::function<GenericValue(const std::string&, const Shape*)> genericActual_;
  std::function<SNLDesign*(NLLibrary*, const std::string&, const GenericValues&)> build_;
  std::set<std::pair<NLLibrary*, std::string>> imported_;
  std::map<std::string, const vhdl::EntityDeclaration*> components_;
  bool unsigned_ = false, arith_ = false, numeric_ = false, signed_ = false;
  struct IntegerTable { Range bounds; std::vector<int64_t> values; };
  std::map<std::string, IntegerTable> integerTables_;
  std::map<std::string, const vhdl::ArrayTypeDeclaration*> arrayDeclarations_;
  std::set<std::string> instanceNames_;
  std::map<std::string, int64_t> integers_;
  std::set<std::string> locallyStaticIntegers_;
  std::map<std::string, FunctionBinding> functions_;
  std::map<const vhdl::FunctionDeclaration*, FunctionEnvironment> localFunctionEnvironments_;
  std::map<std::string, bool> booleans_;
  std::set<const vhdl::ConstantDeclaration*> activeStaticConstants_;
  size_t staticDepth_ = 0;
  std::map<std::string, Shape> arrays_, records_;
  std::map<std::string, const vhdl::EnumerationTypeDeclaration*> enumerations_;
  std::map<std::string, size_t> recordOffsets_;
  std::map<std::string, size_t> arrayTypeOffsets_;
  std::map<std::string, Object> objects_;
  std::string objectPrefix_;
  std::vector<std::pair<std::string, Object>> generatedObjects_;
  std::map<std::string, Memory> memories_;
  std::set<SNLBitNet*> drivers_, reads_;
  std::map<SNLBitNet*, bool> initialValues_;
  std::set<SNLBitNet*> initializedFlops_;
  std::set<std::string> libraries_{"std", "work"};
  const Sensitivity* sensitivity_ = nullptr;
  std::set<SNLBitNet*>* processReads_ = nullptr;
  bool combinational_ = false;
  bool stdLogic_ = false;
  size_t steps_ = 0;
  SNLBitNet* zero_ = nullptr;
  SNLBitNet* one_ = nullptr;
};
}

bool requiresVHDLRTL(const vhdl::DesignFile& syntax) {
  if (!syntax.packages.empty()) return true;
  const auto rtlContext = [](const vhdl::ContextClause& context) {
    return std::any_of(context.uses.begin(), context.uses.end(), [](const auto& use) {
      return use.selectedName.size() == 3 && (key(use.selectedName[0]) != "ieee" ||
          key(use.selectedName[1]) == "std_logic_signed");
    });
  };
  for (const auto& entity : syntax.entities) {
    if (rtlContext(entity.context)) return true;
    for (const auto& generic : entity.generics)
      if (generic.type.constraint || !generic.defaultValue || key(generic.type.name) == "boolean" ||
          key(generic.type.name) == "bit_vector" || key(generic.type.name) == "std_logic_vector" ||
          key(generic.type.name) == "std_ulogic_vector" || key(generic.type.name) == "unsigned" ||
          key(generic.type.name) == "signed") return true;
    for (const auto& port : entity.ports) if (port.defaultValue || key(port.type.name) == "std_ulogic" || key(port.type.name) == "std_ulogic_vector" || key(port.type.name) == "unsigned" || key(port.type.name) == "signed") return true;
  }
  const auto extendedExpression = [](const auto& self, const Expr* expression) -> bool {
    return expression && (expression->kind == Expr::Kind::Selected || expression->kind == Expr::Kind::Indexed || expression->kind == Expr::Kind::Call ||
        expression->kind == Expr::Kind::Others || expression->kind == Expr::Kind::Aggregate ||
        expression->kind == Expr::Kind::BitStringLiteral || self(self, expression->left.get()) ||
        self(self, expression->right.get()) || self(self, expression->condition.get()));
  };
  const auto extendedAssignment = [&](const vhdl::Assignment& assignment) {
    return assignment.selector || !assignment.indices.empty() || extendedExpression(extendedExpression, assignment.value.get());
  };
  for (const auto& architecture : syntax.architectures) {
    if (!architecture.functions.empty()) return true;
    if (!architecture.recordTypes.empty() || !architecture.enumerationTypes.empty()) return true;
    if (rtlContext(architecture.context)) return true;
    if (!architecture.generates.empty() || !architecture.components.empty() || !architecture.constants.empty()) return true;
    for (const auto& signal : architecture.signals) if (signal.initializer || key(signal.type.name) == "std_ulogic" || key(signal.type.name) == "std_ulogic_vector" || key(signal.type.name) == "unsigned" || key(signal.type.name) == "signed") return true;
    for (const auto& instance : architecture.instantiations)
      if (key(instance.library) != "work" || std::any_of(instance.actualOpen.begin(), instance.actualOpen.end(),
          [](bool open) { return open; }) || std::any_of(instance.actualLiterals.begin(), instance.actualLiterals.end(),
          [](const auto& literal) { return bool(literal); }) || std::any_of(instance.actualIndices.begin(), instance.actualIndices.end(),
          [](const auto& indices) { return !indices.empty(); }) || instance.component || std::any_of(instance.formals.begin(), instance.formals.end(),
          [](const auto& formal) { return formal.has_value(); })) return true;
    for (const auto& assignment : architecture.assignments)
      if (extendedAssignment(assignment)) return true;
    for (const auto& process : architecture.processes) {
      if (process.combinational || !process.sensitivityList.empty()) return true;
      for (const auto& assignment : process.assignments)
        if (extendedAssignment(assignment)) return true;
      for (const auto& assignment : process.resetAssignments)
        if (extendedAssignment(assignment)) return true;
    }
  }
  return false;
}

SNLDesign* constructVHDLRTL(NLLibrary* library, const vhdl::DesignFile& syntax,
                          std::string_view top, std::string_view source,
                          const std::vector<VHDLLibrarySource>& libraries) {
  const LibraryContext context{library, libraries};
  std::set<std::pair<NLLibrary*, std::string>> packages, bodies;
  for (const auto& package : syntax.packages) {
    DiagnosticScope location(package.name.span);
    const auto name = std::make_pair(context.owner(package.name.span.start.offset), key(package.name));
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
    std::set<std::string> candidates;
    for (const auto& entity : syntax.entities)
      if (context.owner(entity.span.start.offset) == library) candidates.insert(key(entity.name));
    const auto removeChildren = [&](const auto& self, const auto& scope) -> void {
      for (const auto& instance : scope.instantiations)
        if (key(instance.library) == "work" ||
            context.resolve(library, key(instance.library)) == library) candidates.erase(key(instance.entity));
      for (const auto& generate : scope.generates) self(self, generate);
      if constexpr (requires { scope.alternatives; })
        for (const auto& branch : scope.alternatives) self(self, branch);
    };
    for (const auto& architecture : syntax.architectures)
      if (context.owner(architecture.span.start.offset) == library) removeChildren(removeChildren, architecture);
    if (candidates.size() != 1)
      fail("cannot infer a unique RTL top entity; specify an explicit top");
    selected = *candidates.begin();
  }
  std::vector<SNLDesign*> created;
  std::set<std::pair<NLLibrary*, std::string>> active;
  std::function<SNLDesign*(NLLibrary*, const std::string&, const GenericValues&)> build;
  build = [&](NLLibrary* owner, const std::string& name, const GenericValues& overrides) -> SNLDesign* {
    if (!active.insert({owner, name}).second) fail("recursive RTL hierarchy");
    const vhdl::EntityDeclaration* entity = nullptr;
    const vhdl::ArchitectureBody* architecture = nullptr;
    for (const auto& candidate : syntax.entities) if (context.owner(candidate.span.start.offset) == owner && key(candidate.name) == name) {
      DiagnosticScope location(candidate.span);
      if (entity) fail("duplicate RTL entity");
      entity = &candidate;
    }
    for (const auto& candidate : syntax.architectures) if (context.owner(candidate.span.start.offset) == owner && key(candidate.entity) == name) {
      DiagnosticScope location(candidate.span);
      if (architecture) fail("multiple RTL architectures are unsupported");
      architecture = &candidate;
    }
    if (!entity || !architecture) fail("missing RTL entity or architecture: " + name);
    std::string modelName = entity->name.spelling;
    for (const auto& [generic, value] : overrides)
      modelName += "__" + generic + "_" +
          (value.bits ? "v" + *value.bits : value.scalar.boolean ?
            (value.scalar.value ? "true" : "false") : std::to_string(value.scalar.value));
    const auto signature = std::string(source.substr(context.start(owner),
        std::max(entity->span.end.offset, architecture->span.end.offset) - context.start(owner))) + modelName;
    if (auto* existing = owner->getSNLDesign(NLName(modelName))) {
      auto* property = dynamic_cast<RTLSourceProperty*>(existing->getProperty("VHDLRTLSource"));
      if (!property || property->signature != signature) fail("conflicting RTL design: " + modelName);
      active.erase({owner, name});
      return existing;
    }
    auto* design = SNLDesign::create(owner, NLName(modelName));
    created.push_back(design);
    RTLConstructor(design, *entity, *architecture, syntax, context, overrides, build).run();
    RTLSourceProperty::attach(design, signature);
    active.erase({owner, name});
    return design;
  };
  try {
    return build(library, selected, {});
  } catch (...) {
    for (auto* design : created) design->destroy();
    throw;
  }
}
}
