// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLLibertyConstructor.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "YosysLibertyParser.h"
#include "YosysLibertyException.h"

#include "NajaPerf.h"

#include "NLLibrary.h"
#include "NLLibraryTruthTables.h"

#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBundleTerm.h"
#include "SNLBooleanTree.h"
#include "SNLDesignModeling.h"
#include "SNLLibertyConstructorException.h"

#include <zlib.h>

namespace {

using namespace naja::NL;

SNLTerm::Direction getSNLDirection(const std::string& direction) {
  if (direction == "input") {
    return SNLTerm::Direction::Input;
  } else if (direction == "output") {
    return SNLTerm::Direction::Output;
  } else if (direction == "inout") {
    return SNLTerm::Direction::InOut;
  } else {
    std::ostringstream reason;
    reason << "Unknown direction: " << direction;
    throw SNLLibertyConstructorException(reason.str());
  }
}

const Yosys::LibertyAst* findDirectChild(const Yosys::LibertyAst* node, const std::string& id) {
  for (auto child: node->children) {
    if (child->id == id) {
      return child;
    }
  }
  return nullptr;
}

const Yosys::LibertyAst* findDirectionNode(const Yosys::LibertyAst* node) {
  auto directionNode = findDirectChild(node, "direction");
  if (directionNode != nullptr or node->id != "bus") {
    return directionNode;
  }

  const Yosys::LibertyAst* inferredDirectionNode = nullptr;
  for (auto child: node->children) {
    if (child->id != "pin") {
      continue;
    }
    auto childDirectionNode = findDirectChild(child, "direction");
    if (childDirectionNode == nullptr) {
      continue;
    }
    if (inferredDirectionNode == nullptr) {
      inferredDirectionNode = childDirectionNode;
      continue;
    }
    if (inferredDirectionNode->value != childDirectionNode->value) {
      std::ostringstream reason;
      reason << "Inconsistent child pin directions for bus " << node->args[0];
      throw SNLLibertyConstructorException(reason.str());
    }
  }
  return inferredDirectionNode;
}

struct BusType {
  std::string name  {};
  int msb           {0};
  int lsb           {0};
};

using TermFunctions = std::map<SNLScalarTerm*, std::string, SNLScalarTerm::PointerLess>;
using TermPins = std::map<SNLScalarTerm*, std::string, SNLScalarTerm::PointerLess>;
using SequentialTermClocks = std::map<SNLBitTerm*, std::string, SNLBitTerm::InDesignLess>;

struct ConstructedTerm {
  SNLScalarTerm* scalar{nullptr};
  SNLBusTerm* bus{nullptr};
};

struct LibertyMemoryReadPort {
  SNLBusTerm* dataBus{nullptr};
  std::string addressName {};
  std::string clockName   {};
};

struct LibertyMemoryWriteBus {
  SNLBusTerm* dataBus{nullptr};
  std::string addressName {};
  std::string clockName   {};
};

struct LibertyMemoryInterface {
  bool present {false};
  size_t wordWidth {0};
  size_t addressWidth {0};
  std::vector<LibertyMemoryReadPort> readPorts {};
  std::vector<LibertyMemoryWriteBus> writeBuses {};
};

BusType findBusType(const Yosys::LibertyAst* ast, const std::string& busType) {
  for (auto child: ast->children) {
    if (child->id == "type" and not child->args.empty() and child->args[0] == busType) {
      int from = 0;
      int to = 0;
      bool downto = true;
      for (auto subChild: child->children) {
        if (subChild->id == "bit_from") {
          from = std::stoi(subChild->value);
        } else if (subChild->id == "bit_to") {
          to = std::stoi(subChild->value);
        } else if (subChild->id == "downto") {
          downto = subChild->value == "true";
        }
      }
      return BusType{busType, downto ? from : to, downto ? to : from};
    }
  }
  return BusType();
}

enum class FunctionParsingType {
  Ignore,
  Sequential,
  Combinational
};

std::string buildFunctionParseErrorReason(
  const std::string& pinName,
  const std::string& cellName,
  const std::filesystem::path& sourcePath,
  const std::string& parserReason) {
  std::ostringstream reason;
  reason << "While parsing function for pin `";
  reason << pinName;
  reason << "` in cell `";
  reason << cellName;
  reason << "` from file `";
  reason << sourcePath.string();
  reason << "`: ";
  reason << parserReason;
  return reason.str();
}

std::string buildParserFileErrorReason(
  const std::filesystem::path& sourcePath,
  const std::string& parserReason) {
  std::ostringstream reason;
  reason << "Liberty parser error in file `";
  reason << sourcePath.string();
  reason << "`: ";
  reason << parserReason;
  return reason.str();
}

BusType findChildBusType(
  const Yosys::LibertyAst* top,
  const Yosys::LibertyAst* child,
  const SNLDesign* primitive) {
  BusType busType;
  if (child->id != "bus") {
    return busType;
  }
  for (auto busTypeChild: child->children) {
    if (busTypeChild->id == "bus_type") {
      auto busTypeName = busTypeChild->value;
      busType = findBusType(top, busTypeName);
      if (busType.name.empty()) {
        std::string message;
        message += "While constructing " + primitive->getName().getString() + " interface,";
        message += " cannot find bus type: " + busTypeName;
        throw SNLLibertyConstructorException(message);
      }
    }
  }
  return busType;
}

ConstructedTerm constructTerm(
  SNLDesign* primitive,
  const Yosys::LibertyAst* top,
  const Yosys::LibertyAst* child,
  SNLBundleTerm* bundleTerm = nullptr,
  std::optional<SNLTerm::Direction> inheritedDirection = std::nullopt) {
  auto pinName = child->args[0];
  auto busType = findChildBusType(top, child, primitive);
  auto directionNode = findDirectionNode(child);
  auto snlDirection = inheritedDirection;
  if (directionNode) {
    auto direction = directionNode->value;
    if (direction == "internal") {
      return {};
    }
    snlDirection = getSNLDirection(direction);
  }
  if (not snlDirection) {
    std::ostringstream reason;
    reason << "Direction not found for " << child->id << " " << pinName;
    throw SNLLibertyConstructorException(reason.str());
  }
  ConstructedTerm term;
  if (busType.name.empty()) {
    term.scalar = bundleTerm
      ? SNLScalarTerm::create(bundleTerm, *snlDirection, NLName(pinName))
      : SNLScalarTerm::create(primitive, *snlDirection, NLName(pinName));
  } else {
    term.bus = bundleTerm
      ? SNLBusTerm::create(bundleTerm, *snlDirection, busType.msb, busType.lsb, NLName(pinName))
      : SNLBusTerm::create(primitive, *snlDirection, busType.msb, busType.lsb, NLName(pinName));
  }
  return term;
}

bool isSequentialTimingType(const std::string& timingType) {
  return timingType == "hold_rising" || timingType == "setup_rising" ||
         timingType == "rising_edge";
}

void registerSequentialClockForBitTerm(
    SNLBitTerm* term,
    const std::string& clockName,
    SequentialTermClocks& seqTermClocks) {
  const auto [it, inserted] = seqTermClocks.emplace(term, clockName);
  if (!inserted && it->second != clockName) {
    std::ostringstream reason;
    reason << "Conflicting sequential clocks `" << it->second << "` and `"
           << clockName << "` for term " << term->getName().getString();
    throw SNLLibertyConstructorException(reason.str());
  }
}

void registerSequentialClockFromTiming(
    const Yosys::LibertyAst* child,
    SNLBitTerm* term,
    SequentialTermClocks& seqTermClocks) {
  for (auto timingNode : child->children) {
    if (timingNode->id != "timing") {
      continue;
    }
    auto timingTypeNode = timingNode->find("timing_type");
    if (timingTypeNode == nullptr ||
        !isSequentialTimingType(timingTypeNode->value)) {
      continue;
    }
    auto relatedPin = timingNode->find("related_pin");
    if (relatedPin == nullptr) {
      continue;
    }
    registerSequentialClockForBitTerm(term, relatedPin->value, seqTermClocks);
  }
}

std::string getChildValueOrThrow(
    const Yosys::LibertyAst* node,
    const std::string& childID,
    const std::string& context) {
  auto child = findDirectChild(node, childID);
  if (child == nullptr) {
    std::ostringstream reason;
    reason << "Missing `" << childID << "` while processing " << context;
    throw SNLLibertyConstructorException(reason.str());
  }
  return child->value;
}

std::optional<LibertyMemoryReadPort> extractMemoryReadPort(
    const Yosys::LibertyAst* child,
    SNLBusTerm* constructedBusTerm) {
  if (constructedBusTerm == nullptr) {
    return std::nullopt;
  }
  auto memoryRead = findDirectChild(child, "memory_read");
  if (memoryRead == nullptr) {
    return std::nullopt;
  }
  LibertyMemoryReadPort port;
  port.dataBus = constructedBusTerm;
  port.addressName = getChildValueOrThrow(
      memoryRead, "address", "Liberty memory_read for `" + child->args[0] + "`");
  for (auto timingNode : child->children) {
    if (timingNode->id != "timing") {
      continue;
    }
    auto timingTypeNode = timingNode->find("timing_type");
    if (timingTypeNode == nullptr || timingTypeNode->value != "rising_edge") {
      continue;
    }
    auto relatedPin = timingNode->find("related_pin");
    if (relatedPin != nullptr) {
      port.clockName = relatedPin->value;
      break;
    }
  }
  return port;
}

std::optional<LibertyMemoryWriteBus> extractMemoryWriteBus(
    const Yosys::LibertyAst* child,
    SNLBusTerm* constructedBusTerm) {
  if (constructedBusTerm == nullptr) {
    return std::nullopt;
  }
  auto memoryWrite = findDirectChild(child, "memory_write");
  if (memoryWrite == nullptr) {
    return std::nullopt;
  }
  LibertyMemoryWriteBus bus;
  bus.dataBus = constructedBusTerm;
  bus.addressName = getChildValueOrThrow(
      memoryWrite, "address", "Liberty memory_write for `" + child->args[0] + "`");
  bus.clockName = getChildValueOrThrow(
      memoryWrite, "clocked_on", "Liberty memory_write for `" + child->args[0] + "`");
  return bus;
}

void populateMemoryInterfaceHeader(
    const Yosys::LibertyAst* cell,
    LibertyMemoryInterface& memoryInterface) {
  auto memoryNode = findDirectChild(cell, "memory");
  if (memoryNode == nullptr) {
    return;
  }
  memoryInterface.present = true;
  auto addressWidthNode = findDirectChild(memoryNode, "address_width");
  auto wordWidthNode = findDirectChild(memoryNode, "word_width");
  if (addressWidthNode != nullptr) {
    memoryInterface.addressWidth = static_cast<size_t>(
        std::stoull(addressWidthNode->value));
  }
  if (wordWidthNode != nullptr) {
    memoryInterface.wordWidth = static_cast<size_t>(
        std::stoull(wordWidthNode->value));
  }
}

void registerConstructedTermModeling(
  const Yosys::LibertyAst* child,
  const std::string& pinName,
  FunctionParsingType functionParsingType,
  SNLScalarTerm* constructedScalarTerm,
  SNLBusTerm* constructedBusTerm,
  TermFunctions& termFunctions,
  TermPins& termFunctionPins,
  SequentialTermClocks& seqTermClocks,
  LibertyMemoryInterface& memoryInterface) {
  if (functionParsingType == FunctionParsingType::Sequential) {
    if (constructedScalarTerm) {
      registerSequentialClockFromTiming(child, constructedScalarTerm, seqTermClocks);
    } else if (constructedBusTerm) {
      for (auto* bit : constructedBusTerm->getBits()) {
        registerSequentialClockFromTiming(child, bit, seqTermClocks);
      }
    }
  }

  if (memoryInterface.present) {
    if (auto readPort = extractMemoryReadPort(child, constructedBusTerm);
        readPort.has_value()) {
      memoryInterface.readPorts.push_back(*readPort);
    }
    if (auto writeBus = extractMemoryWriteBus(child, constructedBusTerm);
        writeBus.has_value()) {
      memoryInterface.writeBuses.push_back(*writeBus);
    }
  }
  if (functionParsingType == FunctionParsingType::Combinational
    and constructedScalarTerm
    and constructedScalarTerm->getDirection() == SNLTerm::Direction::Output) {
    termFunctions[constructedScalarTerm] = pinName;
    termFunctionPins[constructedScalarTerm] = pinName;
    auto functionNode = const_cast<Yosys::LibertyAst*>(child)->find("function");
    if (functionNode) {
      termFunctions[constructedScalarTerm] = functionNode->value;
    }
  } else if (functionParsingType != FunctionParsingType::Ignore
    and constructedBusTerm
    and constructedBusTerm->getDirection() == SNLTerm::Direction::Output
    and const_cast<Yosys::LibertyAst*>(child)->find("function") != nullptr) {
    std::ostringstream reason;
    reason << "No support for function for bus term while processing " << child->id << " " << pinName;
    throw SNLLibertyConstructorException(reason.str());
  }
}

std::vector<std::string> getBundleMembers(const Yosys::LibertyAst* bundleNode) {
  auto membersNode = findDirectChild(bundleNode, "members");
  if (membersNode == nullptr or membersNode->args.empty()) {
    std::ostringstream reason;
    reason << "Bundle " << bundleNode->args[0] << " does not define members(...)";
    throw SNLLibertyConstructorException(reason.str());
  }
  return membersNode->args;
}

std::map<std::string, const Yosys::LibertyAst*> collectBundleMemberDefinitions(
  const Yosys::LibertyAst* bundleNode) {
  std::map<std::string, const Yosys::LibertyAst*> members;
  for (auto child: bundleNode->children) {
    if (child->id == "bundle") {
      std::ostringstream reason;
      reason << "Nested Liberty bundles are not supported in bundle " << bundleNode->args[0];
      throw SNLLibertyConstructorException(reason.str());
    }
    if (child->id != "pin" and child->id != "bus") {
      continue;
    }
    if (child->args.empty()) {
      std::ostringstream reason;
      reason << "Malformed Liberty bundle member definition in bundle " << bundleNode->args[0];
      throw SNLLibertyConstructorException(reason.str());
    }
    auto inserted = members.emplace(child->args[0], child);
    if (not inserted.second) {
      std::ostringstream reason;
      reason << "Duplicate Liberty bundle member definition " << child->args[0]
             << " in bundle " << bundleNode->args[0];
      throw SNLLibertyConstructorException(reason.str());
    }
  }
  return members;
}

SNLTerm::Direction inferBundleDirection(
  const Yosys::LibertyAst* bundleNode,
  const std::vector<std::string>& orderedMembers,
  const std::map<std::string, const Yosys::LibertyAst*>& memberDefinitions) {
  auto bundleDirectionNode = findDirectChild(bundleNode, "direction");
  if (bundleDirectionNode != nullptr) {
    for (const auto& memberName: orderedMembers) {
      auto it = memberDefinitions.find(memberName);
      if (it == memberDefinitions.end()) {
        std::ostringstream reason;
        reason << "Bundle " << bundleNode->args[0] << " lists missing member " << memberName;
        throw SNLLibertyConstructorException(reason.str());
      }
      auto directionNode = findDirectionNode(it->second);
      if (directionNode != nullptr and directionNode->value != bundleDirectionNode->value) {
        std::ostringstream reason;
        reason << "Inconsistent child directions for bundle " << bundleNode->args[0];
        throw SNLLibertyConstructorException(reason.str());
      }
    }
    return getSNLDirection(bundleDirectionNode->value);
  }

  const Yosys::LibertyAst* inferredDirectionNode = nullptr;
  for (const auto& memberName: orderedMembers) {
    auto it = memberDefinitions.find(memberName);
    if (it == memberDefinitions.end()) {
      std::ostringstream reason;
      reason << "Bundle " << bundleNode->args[0] << " lists missing member " << memberName;
      throw SNLLibertyConstructorException(reason.str());
    }
    auto directionNode = findDirectionNode(it->second);
    if (directionNode == nullptr) {
      std::ostringstream reason;
      reason << "Direction not found for bundle member " << memberName;
      throw SNLLibertyConstructorException(reason.str());
    }
    if (inferredDirectionNode == nullptr) {
      inferredDirectionNode = directionNode;
      continue;
    }
    if (inferredDirectionNode->value != directionNode->value) {
      std::ostringstream reason;
      reason << "Inconsistent child directions for bundle " << bundleNode->args[0];
      throw SNLLibertyConstructorException(reason.str());
    }
  }
  return getSNLDirection(inferredDirectionNode->value);
}

void parseTerms(
  SNLDesign* primitive,
  const Yosys::LibertyAst* top,
  const Yosys::LibertyAst* cell,
  const std::filesystem::path& sourcePath,
  FunctionParsingType functionParsingType = FunctionParsingType::Ignore) {
  TermFunctions termFunctions;
  TermPins termFunctionPins;
  SequentialTermClocks seqTermClocks;
  LibertyMemoryInterface memoryInterface;
  populateMemoryInterfaceHeader(cell, memoryInterface);
  for (auto child: cell->children) {
    if (child->id == "pin" or child->id == "bus") {
      auto constructedTerm = constructTerm(primitive, top, child);
      registerConstructedTermModeling(
        child,
        child->args[0],
        functionParsingType,
        constructedTerm.scalar,
        constructedTerm.bus,
        termFunctions,
        termFunctionPins,
        seqTermClocks,
        memoryInterface);
    } else if (child->id == "bundle") {
      auto bundleDirectionNode = findDirectionNode(child);
      if (bundleDirectionNode != nullptr and bundleDirectionNode->value == "internal") {
        continue;
      }
      auto orderedMembers = getBundleMembers(child);
      auto memberDefinitions = collectBundleMemberDefinitions(child);
      std::set<std::string> orderedMemberSet(orderedMembers.begin(), orderedMembers.end());
      for (const auto& [memberName, _]: memberDefinitions) {
        if (orderedMemberSet.find(memberName) == orderedMemberSet.end()) {
          std::ostringstream reason;
          reason << "Bundle " << child->args[0] << " defines extra member " << memberName;
          throw SNLLibertyConstructorException(reason.str());
        }
      }
      auto bundleDirection = inferBundleDirection(child, orderedMembers, memberDefinitions);
      auto bundleTerm = SNLBundleTerm::create(primitive, bundleDirection, NLName(child->args[0]));
      for (const auto& memberName: orderedMembers) {
        auto memberIt = memberDefinitions.find(memberName);
        auto memberNode = memberIt->second;
        auto constructedTerm = constructTerm(primitive, top, memberNode, bundleTerm, bundleDirection);
        registerConstructedTermModeling(
          memberNode,
          memberName,
          functionParsingType,
          constructedTerm.scalar,
          constructedTerm.bus,
          termFunctions,
          termFunctionPins,
          seqTermClocks,
          memoryInterface);
      }
    }
  }
  std::vector<SNLBitTerm*> outputs;
  for (auto term: primitive->getBitTerms()) {
    if (term->getDirection() != SNLTerm::Direction::Input) {
      outputs.push_back(term);
    }
  }
  if (seqTermClocks.size() > 0) {
    for (const auto& pair: seqTermClocks) {
      auto term = pair.first;
      const auto& clockName = pair.second;
      auto clock = dynamic_cast<SNLScalarTerm*>(primitive->getTerm(NLName(clockName)));
      if (clock) {
        if (term->getDirection() == SNLTerm::Direction::Input) {
          SNLDesignModeling::addInputsToClockArcs({term}, clock);
        } else if (term->getDirection() == SNLTerm::Direction::Output) {
          SNLDesignModeling::addClockToOutputsArcs(clock, {term});
        }
      }
    }
    if (memoryInterface.present) {
      // Liberty describes memories with separate memory_read/memory_write
      // attributes on data buses plus timing arcs on address/control pins.
      // Normalize those pieces into SNLDesignModeling::MemoryInterface so
      // later passes can discover memories through one generic API.
      SNLDesignModeling::MemoryInterface modelingInterface;
      modelingInterface.width = memoryInterface.wordWidth;
      modelingInterface.abits = memoryInterface.addressWidth;
      modelingInterface.depth = memoryInterface.addressWidth == 0
          ? 0
          : (static_cast<size_t>(1) << memoryInterface.addressWidth);
      if (!memoryInterface.readPorts.empty()) {
        const auto& firstReadPort = memoryInterface.readPorts.front();
        if (!firstReadPort.clockName.empty()) {
          // LCOV_EXCL_START
          modelingInterface.clock = dynamic_cast<SNLScalarTerm*>(
              primitive->getTerm(NLName(firstReadPort.clockName)));
          // LCOV_EXCL_STOP
        }
      }

      std::set<SNLBitTerm*, SNLBitTerm::InDesignLess> usedInputTerms;
      for (const auto& readPort : memoryInterface.readPorts) {
        SNLDesignModeling::MemoryReadPort modelingReadPort;
        auto* addressTerm = primitive->getTerm(NLName(readPort.addressName));
        auto* addressBus = dynamic_cast<SNLBusTerm*>(addressTerm);
        if (addressBus == nullptr) {
          std::ostringstream reason;
          reason << "Memory read address `" << readPort.addressName
                 << "` is not a bus in cell `" << primitive->getName().getString()
                 << "`";
          throw SNLLibertyConstructorException(reason.str());
        }
        for (auto* bit : addressBus->getBits()) {
          modelingReadPort.address.push_back(bit);
          usedInputTerms.insert(bit);
        }
        for (auto* bit : readPort.dataBus->getBits()) {
          modelingReadPort.data.push_back(bit);
        }
        SNLDesignModeling::addCombinatorialArcs(
            modelingReadPort.address, modelingReadPort.data);
        modelingInterface.readPorts.push_back(std::move(modelingReadPort));
      }

      struct WritePortGroupKey {
        std::string addressName;
        std::string clockName;

        bool operator<(const WritePortGroupKey& other) const {
          return std::tie(addressName, clockName) <
                 std::tie(other.addressName, other.clockName);
        }
      };
      std::map<WritePortGroupKey, std::vector<SNLBusTerm*>> groupedWriteBuses;
      for (const auto& writeBus : memoryInterface.writeBuses) {
        groupedWriteBuses[{writeBus.addressName, writeBus.clockName}].push_back(
            writeBus.dataBus);
      }
      for (const auto& [key, buses] : groupedWriteBuses) {
        SNLDesignModeling::MemoryWritePort modelingWritePort;
        auto* addressTerm = primitive->getTerm(NLName(key.addressName));
        auto* addressBus = dynamic_cast<SNLBusTerm*>(addressTerm);
        if (addressBus == nullptr) {
          std::ostringstream reason;
          reason << "Memory write address `" << key.addressName
                 << "` is not a bus in cell `" << primitive->getName().getString()
                 << "`";
          throw SNLLibertyConstructorException(reason.str());
        }
        for (auto* bit : addressBus->getBits()) {
          modelingWritePort.address.push_back(bit);
          usedInputTerms.insert(bit);
        }
        if (!buses.empty()) {
          for (auto* bit : buses.front()->getBits()) {
            modelingWritePort.data.push_back(bit);
            usedInputTerms.insert(bit);
          }
        }
        if (buses.size() >= 2) {
          for (auto* bit : buses[1]->getBits()) {
            modelingWritePort.mask.push_back(bit);
            usedInputTerms.insert(bit);
          }
        }
        for (size_t busIndex = 2; busIndex < buses.size(); ++busIndex) {
          SNLDesignModeling::BitTerms extraWriteInput;
          for (auto* bit : buses[busIndex]->getBits()) {
            extraWriteInput.push_back(bit);
            usedInputTerms.insert(bit);
          }
          modelingWritePort.extraWriteInputs.push_back(std::move(extraWriteInput));
        }
        for (const auto& [timedTerm, timedClockName] : seqTermClocks) {
          if (timedClockName != key.clockName ||
              timedTerm->getDirection() == SNLTerm::Direction::Output ||
              usedInputTerms.find(timedTerm) != usedInputTerms.end()) {
            continue;
          }
          modelingWritePort.enables.push_back(timedTerm);
        }
        modelingInterface.writePorts.push_back(std::move(modelingWritePort));
      }

      if (modelingInterface.clock != nullptr && modelingInterface.isValid()) {
        SNLDesignModeling::setMemoryInterface(primitive, modelingInterface);
      }
    }
  } else if (termFunctions.size() == 1 && outputs.size() == 1) {
    auto outputTerm = termFunctions.begin()->first;
    auto pinName = termFunctionPins[outputTerm];
    auto cellName = primitive->getName().getString();
    auto function = termFunctions.begin()->second;
    auto tree = std::make_unique<naja::NL::SNLBooleanTree>();
    try {
      //std::cerr << "Parsing function: " << function << std::endl;
      tree->parse(primitive, function);
      naja::NL::SNLBooleanTree::Terms terms;
      for (auto term: primitive->getBitTerms()) {
        if (term->getDirection() == SNLTerm::Direction::Input) {
          terms.push_back(term);
        }
      }
      std::reverse(terms.begin(), terms.end());
      auto truthTable = tree->getTruthTable(terms);
      naja::NL::SNLDesignModeling::setTruthTable(primitive, truthTable);
    } catch (const SNLLibertyConstructorException& e) {
      auto reason = buildFunctionParseErrorReason(
        pinName, cellName, sourcePath, e.getReason());
      throw SNLLibertyConstructorException(reason);
    }
  } else if (termFunctions.size() > 0) {  
    std::vector<SNLTruthTable> truthTables;
    auto cellName = primitive->getName().getString();
    // Assuming termFunctions is ordered based on termIDs!
    for (auto term: primitive->getBitTerms()) {
      if (term->getDirection() == SNLTerm::Direction::Input) {
        continue;
      }
      SNLScalarTerm* scalarTerm = dynamic_cast<SNLScalarTerm*>(term);
      if (scalarTerm == nullptr) {
        truthTables.push_back(SNLTruthTable()); //push empty table for non-scalar terms
        continue;
      }
      if (termFunctions.find(scalarTerm) == termFunctions.end()) {
        truthTables.push_back(SNLTruthTable()); //push empty table for terms without function
        continue;
      }
      auto& function = termFunctions[scalarTerm];
      auto pinName = termFunctionPins[scalarTerm];
      auto tree = std::make_unique<naja::NL::SNLBooleanTree>();
      try {
        //std::cerr << "Parsing function: " << function << std::endl;
        tree->parse(primitive, function);
        naja::NL::SNLBooleanTree::Terms terms;
        for (auto term: primitive->getBitTerms()) {
          if (term->getDirection() == SNLTerm::Direction::Input) {
            terms.push_back(term);
          }
        }
        std::reverse(terms.begin(), terms.end());
        auto truthTable = tree->getTruthTable(terms);
        truthTables.push_back(truthTable);
      } catch (const SNLLibertyConstructorException& e) {
        auto reason = buildFunctionParseErrorReason(
          pinName, cellName, sourcePath, e.getReason());
        throw SNLLibertyConstructorException(reason);
      }
    }
    naja::NL::SNLDesignModeling::setTruthTables(primitive, truthTables);
  }
}

void parseCell(
  NLLibrary* library,
  const Yosys::LibertyAst* top,
  Yosys::LibertyAst* cell,
  const std::filesystem::path& sourcePath) {
  auto cellName = cell->args[0];
  auto primitive = SNLDesign::create(library, SNLDesign::Type::Primitive, NLName(cellName));
  //std::cerr << "Parse cell: " << cellName << std::endl;
  FunctionParsingType type = FunctionParsingType::Combinational;
  if (cell->find("ff") || cell->find("memory")) {
    type = FunctionParsingType::Sequential;
  } else if (cell->find("latch")) {
    type = FunctionParsingType::Ignore; //LCOV_EXCL_LINE
  }
  parseTerms(primitive, top, cell, sourcePath, type);
}

void parseCells(
  NLLibrary* library,
  const Yosys::LibertyAst* ast,
  const std::filesystem::path& sourcePath) {
  for (auto child: ast->children) {
    if (child->id == "cell") {
      parseCell(library, ast, child, sourcePath);
    }
  }
}

class GzipStreamBuf final : public std::streambuf {
  public:
    explicit GzipStreamBuf(const std::filesystem::path& path) {
      file_ = gzopen(path.string().c_str(), "rb");
      if (file_ == nullptr) {
        std::string reason("Liberty parser: failed to open gzip file: " + path.string());
        throw SNLLibertyConstructorException(reason);
      }
      setg(buffer_, buffer_ + kBufferSize, buffer_ + kBufferSize);
    }
    ~GzipStreamBuf() override {
      if (file_ != nullptr) {
        gzclose(file_);
      }
    }
  protected:
    int_type underflow() override {
      if (gptr() < egptr()) {
        return traits_type::to_int_type(*gptr()); // LCOV_EXCL_LINE
      }
      if (file_ == nullptr) {
        return traits_type::eof(); // LCOV_EXCL_LINE
      }
      int bytesRead = gzread(file_, buffer_, kBufferSize);
      if (bytesRead == 0) {
        return traits_type::eof();
      }
      if (bytesRead < 0) {
        int errnum = 0;
        const char* errorMessage = gzerror(file_, &errnum);
        std::ostringstream reason;
        reason << "Liberty parser: failed to read gzip file";
        if (errorMessage != nullptr and errnum != Z_OK) {
          reason << " (" << errorMessage << ")";
        }
        throw SNLLibertyConstructorException(reason.str());
      }
      setg(buffer_, buffer_, buffer_ + bytesRead);
      return traits_type::to_int_type(*gptr());
    }
  private:
    static constexpr int kBufferSize = 64 * 1024;
    gzFile file_{nullptr};
    char buffer_[kBufferSize];
};

class GzipIStream final : public std::istream {
  public:
    explicit GzipIStream(const std::filesystem::path& path)
      : std::istream(nullptr), buffer_(path) {
      rdbuf(&buffer_);
    }
  private:
    GzipStreamBuf buffer_;
};

std::vector<unsigned char> readFileHeader(
  const std::filesystem::path& path,
  size_t size) {
  std::ifstream input(path, std::ios::binary);
  if (not input.is_open()) {
    std::string reason("Liberty parser: failed to open file: " + path.string());
    throw SNLLibertyConstructorException(reason);
  }
  std::vector<unsigned char> header(size);
  input.read(reinterpret_cast<char*>(header.data()), header.size());
  header.resize(input.gcount());
  return header;
}

bool hasGzipSignature(const std::vector<unsigned char>& header) {
  return header.size() >= 2
    and header[0] == 0x1F
    and header[1] == 0x8B;
}

bool hasZipSignature(const std::vector<unsigned char>& header) {
  return header.size() >= 4
    and header[0] == 'P'
    and header[1] == 'K'
    and header[2] == 0x03
    and header[3] == 0x04;
}

std::vector<unsigned char> readBinaryFile(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (not input.is_open()) {
    std::string reason("Liberty parser: failed to open zip file: " + path.string());
    throw SNLLibertyConstructorException(reason);
  }
  std::string contents(
    (std::istreambuf_iterator<char>(input)),
    std::istreambuf_iterator<char>());
  return std::vector<unsigned char>(contents.begin(), contents.end());
}

uint16_t readLE16(const std::vector<unsigned char>& bytes, size_t offset) {
  if (offset + 2 > bytes.size()) {
    throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
  }
  return static_cast<uint16_t>(bytes[offset])
    | (static_cast<uint16_t>(bytes[offset + 1]) << 8);
}

uint32_t readLE32(const std::vector<unsigned char>& bytes, size_t offset) {
  if (offset + 4 > bytes.size()) {
    throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
  }
  return static_cast<uint32_t>(bytes[offset])
    | (static_cast<uint32_t>(bytes[offset + 1]) << 8)
    | (static_cast<uint32_t>(bytes[offset + 2]) << 16)
    | (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

struct ZipEntry {
  std::string name {};
  uint16_t flags {0};
  uint16_t method {0};
  uint32_t compressedSize {0};
  uint32_t uncompressedSize {0};
  uint32_t localHeaderOffset {0};

  bool isDirectory() const {
    return not name.empty() and name.back() == '/';
  }
};

size_t findEndOfCentralDirectory(const std::vector<unsigned char>& bytes) {
  constexpr uint32_t eocdSignature = 0x06054B50;
  constexpr size_t minEocdSize = 22;
  constexpr size_t maxCommentSize = 0xFFFF;
  if (bytes.size() < minEocdSize) {
    throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
  }
  auto minPos = bytes.size() > minEocdSize + maxCommentSize
    ? bytes.size() - (minEocdSize + maxCommentSize)
    : 0;
  for (size_t pos = bytes.size() - minEocdSize; pos + 1 > minPos; --pos) {
    if (readLE32(bytes, pos) == eocdSignature) {
      auto commentSize = readLE16(bytes, pos + 20);
      if (pos + minEocdSize + commentSize == bytes.size()) {
        return pos;
      }
    }
    if (pos == minPos) {
      break;
    }
  }
  throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
}

std::vector<ZipEntry> readZipEntries(const std::vector<unsigned char>& bytes) {
  constexpr uint32_t centralDirectorySignature = 0x02014B50;
  constexpr uint32_t zip64Marker = 0xFFFFFFFF;
  constexpr uint16_t zip64EntryMarker = 0xFFFF;

  auto eocdOffset = findEndOfCentralDirectory(bytes);
  auto entriesCount = readLE16(bytes, eocdOffset + 10);
  auto centralDirectorySize = readLE32(bytes, eocdOffset + 12);
  auto centralDirectoryOffset = readLE32(bytes, eocdOffset + 16);
  if (entriesCount == zip64EntryMarker
      or centralDirectorySize == zip64Marker
      or centralDirectoryOffset == zip64Marker) {
    throw SNLLibertyConstructorException("Liberty parser: zip64 archives are not supported");
  }
  auto centralDirectoryOffsetSize = static_cast<size_t>(centralDirectoryOffset);
  auto centralDirectorySizeSize = static_cast<size_t>(centralDirectorySize);
  if (centralDirectoryOffsetSize > bytes.size()
      or centralDirectorySizeSize > bytes.size() - centralDirectoryOffsetSize) {
    throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
  }

  std::vector<ZipEntry> entries;
  auto offset = centralDirectoryOffsetSize;
  auto centralDirectoryEnd = offset + centralDirectorySizeSize;
  for (uint16_t entryIndex = 0; entryIndex < entriesCount; ++entryIndex) {
    if (offset + 46 > centralDirectoryEnd
        or readLE32(bytes, offset) != centralDirectorySignature) {
      throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
    }

    auto fileNameSize = readLE16(bytes, offset + 28);
    auto extraSize = readLE16(bytes, offset + 30);
    auto commentSize = readLE16(bytes, offset + 32);
    auto nextOffset = offset + 46 + fileNameSize + extraSize + commentSize;
    if (nextOffset > centralDirectoryEnd) {
      throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
    }

    ZipEntry entry;
    entry.flags = readLE16(bytes, offset + 8);
    entry.method = readLE16(bytes, offset + 10);
    entry.compressedSize = readLE32(bytes, offset + 20);
    entry.uncompressedSize = readLE32(bytes, offset + 24);
    entry.localHeaderOffset = readLE32(bytes, offset + 42);
    entry.name = std::string(
      bytes.begin() + static_cast<std::ptrdiff_t>(offset + 46),
      bytes.begin() + static_cast<std::ptrdiff_t>(offset + 46 + fileNameSize));
    entries.push_back(std::move(entry));
    offset = nextOffset;
  }

  return entries;
}

bool hasLibertyName(const std::string& name) {
  return name.find(".lib") != std::string::npos;
}

const ZipEntry* selectZipEntry(const std::vector<ZipEntry>& entries) {
  const ZipEntry* firstRegularEntry = nullptr;
  size_t regularEntries = 0;
  for (const auto& entry: entries) {
    if (entry.isDirectory()) {
      continue;
    }
    ++regularEntries;
    if (firstRegularEntry == nullptr) {
      firstRegularEntry = &entry;
    }
    if (hasLibertyName(entry.name)) {
      return &entry;
    }
  }
  if (regularEntries == 1) {
    return firstRegularEntry;
  }
  return nullptr;
}

std::string inflateZipEntry(
  const std::vector<unsigned char>& compressedBytes,
  uint32_t uncompressedSize) {
  std::string output(uncompressedSize, '\0');
  if (uncompressedSize == 0) {
    return output;
  }

  z_stream stream {};
  int initStatus = inflateInit2(&stream, -MAX_WBITS);
  if (initStatus != Z_OK) {
    throw SNLLibertyConstructorException("Liberty parser: failed to initialize zip inflater");
  }

  stream.next_in = const_cast<Bytef*>(
    reinterpret_cast<const Bytef*>(compressedBytes.data()));
  stream.avail_in = static_cast<uInt>(compressedBytes.size());
  stream.next_out = reinterpret_cast<Bytef*>(output.data());
  stream.avail_out = static_cast<uInt>(output.size());

  int inflateStatus = inflate(&stream, Z_FINISH);
  auto producedBytes = stream.total_out;
  inflateEnd(&stream);
  if (inflateStatus != Z_STREAM_END or producedBytes != uncompressedSize) {
    throw SNLLibertyConstructorException("Liberty parser: failed to inflate zip entry");
  }
  return output;
}

std::string extractZipEntry(
  const std::vector<unsigned char>& bytes,
  const ZipEntry& entry) {
  constexpr uint32_t localFileHeaderSignature = 0x04034B50;
  constexpr uint16_t storedMethod = 0;
  constexpr uint16_t deflateMethod = 8;
  constexpr uint16_t encryptedFlag = 1;

  if ((entry.flags & encryptedFlag) != 0) {
    throw SNLLibertyConstructorException("Liberty parser: encrypted zip entries are not supported");
  }
  auto localHeaderOffset = static_cast<size_t>(entry.localHeaderOffset);
  if (localHeaderOffset + 30 > bytes.size()
      or readLE32(bytes, localHeaderOffset) != localFileHeaderSignature) {
    throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
  }
  auto fileNameSize = readLE16(bytes, localHeaderOffset + 26);
  auto extraSize = readLE16(bytes, localHeaderOffset + 28);
  auto dataOffset = localHeaderOffset + 30 + fileNameSize + extraSize;
  auto compressedSize = static_cast<size_t>(entry.compressedSize);
  if (dataOffset + compressedSize > bytes.size()) {
    throw SNLLibertyConstructorException("Liberty parser: malformed zip archive");
  }
  auto dataBegin = bytes.begin() + static_cast<std::ptrdiff_t>(dataOffset);
  auto dataEnd = dataBegin + static_cast<std::ptrdiff_t>(compressedSize);
  std::vector<unsigned char> compressedBytes(dataBegin, dataEnd);

  if (entry.method == storedMethod) {
    if (entry.compressedSize != entry.uncompressedSize) {
      throw SNLLibertyConstructorException("Liberty parser: malformed stored zip entry");
    }
    return std::string(compressedBytes.begin(), compressedBytes.end());
  }
  if (entry.method == deflateMethod) {
    return inflateZipEntry(compressedBytes, entry.uncompressedSize);
  }

  std::ostringstream reason;
  reason << "Liberty parser: unsupported zip compression method "
         << entry.method << " for " << entry.name;
  throw SNLLibertyConstructorException(reason.str());
}

std::string extractLibertyFromZip(const std::filesystem::path& path) {
  auto bytes = readBinaryFile(path);
  auto entries = readZipEntries(bytes);
  auto selectedEntry = selectZipEntry(entries);
  if (selectedEntry == nullptr) {
    std::string reason("Liberty parser: no unambiguous Liberty file in zip archive: " + path.string());
    throw SNLLibertyConstructorException(reason);
  }
  return extractZipEntry(bytes, *selectedEntry);
}

std::unique_ptr<std::istream> openLibertyStream(const std::filesystem::path& path) {
  auto header = readFileHeader(path, 4);
  if (hasGzipSignature(header)) {
    return std::make_unique<GzipIStream>(path);
  }
  if (hasZipSignature(header)) {
    auto zipContents = extractLibertyFromZip(path);
    auto zipStream = std::make_unique<std::istringstream>();
    zipStream->str(std::move(zipContents));
    return zipStream;
  }
  auto inFile = std::make_unique<std::ifstream>(path);
  if (not inFile->is_open()) {
    std::string reason("Liberty parser: failed to open file: " + path.string());
    throw SNLLibertyConstructorException(reason);
  }
  return inFile;
}

}

namespace naja::NL {

SNLLibertyConstructor::SNLLibertyConstructor(NLLibrary* library):
  library_(library)
{}

bool SNLLibertyConstructor::hasLibertyExtension(const std::filesystem::path& path) {
  auto remaining = path.filename();
  while (not remaining.empty() and not remaining.extension().empty()) {
    if (remaining.extension().string().starts_with(".lib")) {
      return true;
    }
    remaining = remaining.stem();
  }
  return false;
}

bool SNLLibertyConstructor::hasCompressedLibertySignature(
  const std::filesystem::path& path) {
  try {
    auto header = readFileHeader(path, 4);
    return hasGzipSignature(header) or hasZipSignature(header);
  } catch (const SNLLibertyConstructorException&) {
    return false;
  }
}

bool SNLLibertyConstructor::isLibertyPath(const std::filesystem::path& path) {
  auto extension = path.extension();
  return hasLibertyExtension(path)
    or extension == ".gz"
    or extension == ".zip"
    or hasCompressedLibertySignature(path);
}

void SNLLibertyConstructor::construct(const std::filesystem::path& path) {
  construct(Paths{path});
}

void SNLLibertyConstructor::construct(const Paths& paths) {
  NajaPerf::Scope scope("Liberty construct");
  for (const auto& path : paths) {
    if (not std::filesystem::exists(path)) {
      std::string reason("Liberty parser: " + path.string() + " does not exist");
      throw SNLLibertyConstructorException(reason);
    }
    auto inStream = openLibertyStream(path);
    std::unique_ptr<Yosys::LibertyParser> parser;
    try {
      parser = std::make_unique<Yosys::LibertyParser>(*inStream);
    } catch (const naja::liberty::YosysLibertyException& e) {
      auto reason = buildParserFileErrorReason(path, e.getReason());
      throw SNLLibertyConstructorException(reason);
    }
    auto ast = parser->ast;
    //LCOV_EXCL_START
    if (ast == nullptr) {
      std::string reason("Liberty parser: failed to parse the file: " + path.string());
      throw SNLLibertyConstructorException(reason);
    }
    //LCOV_EXCL_STOP
    auto libraryName = ast->args[0];
    //find a policy for multiple libs
    library_->setName(NLName(libraryName));
    parseCells(library_, ast, path);
  }
  NLLibraryTruthTables::construct(library_);
}

}  // namespace naja::NL
