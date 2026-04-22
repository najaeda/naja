// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NLDB0.h"

#include "NLUniverse.h"
#include "NLBitDependencies.h"
#include "NLException.h"

#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLParameter.h"
#include "SNLInstance.h"
#include "SNLDesignModeling.h"

#include <cstdint>
#include <limits>
#include <sstream>
#include <vector>
#if defined(_MSC_VER)
  #include <intrin.h>
#endif
namespace {
  constexpr const char* MemoryPrefix = "naja_mem__";
  constexpr const char* Mux2Prefix = "naja_mux2__w";
  constexpr const char* DFFEName = "naja_dffe";
  constexpr const char* DFFREName = "naja_dffre";
  constexpr const char* DFFSEName = "naja_dffse";

  naja::NL::SNLTruthTable getMux2TruthTable() {
    uint64_t bits = 0;
    for (uint64_t i = 0; i < 8; ++i) {
      bool a = (i & 0x1) != 0;
      bool b = (i & 0x2) != 0;
      bool s = (i & 0x4) != 0;
      bool y = s ? b : a;
      if (y) {
        bits |= (1ULL << i);
      }
    }
    return naja::NL::SNLTruthTable(
      3, bits, naja::NL::SNLTruthTable::fullDependencies(3));
  }

  std::string getMux2InternalName(size_t width) {
    std::ostringstream name;
    name << Mux2Prefix << width;
    return name.str();
  }

  template<typename CreatePrimitive>
  naja::NL::SNLDesign* getOrCreateRootPrimitive(const char* name, CreatePrimitive createPrimitive) {
    auto* rootLibrary = naja::NL::NLDB0::getDB0RootLibrary();
    if (!rootLibrary) {
      return nullptr;
    }
    auto primitiveName = naja::NL::NLName(name);
    if (auto* existing = rootLibrary->getSNLDesign(primitiveName)) {
      return existing;
    }
    createPrimitive(rootLibrary);
    return rootLibrary->getSNLDesign(primitiveName);
  }

  bool isNamedRootPrimitive(const naja::NL::SNLDesign* design, const char* name) {
    return design && naja::NL::NLDB0::isDB0Primitive(design) && !design->isUnnamed() &&
           design->getName() == naja::NL::NLName(name);
  }

  naja::NL::SNLDesignModeling::BitTerms collectBitTerms(naja::NL::SNLBusTerm& busTerm) {
    naja::NL::SNLDesignModeling::BitTerms bitTerms;
    for (auto* bitTerm: busTerm.getBits()) {
      bitTerms.push_back(bitTerm);
    }
    return bitTerms;
  }

  std::string getMemoryResetModeSuffix(naja::NL::NLDB0::MemoryResetMode mode) {
    switch (mode) {
      case naja::NL::NLDB0::MemoryResetMode::None:
        return "none";
      case naja::NL::NLDB0::MemoryResetMode::AsyncLow:
        return "async_low";
      case naja::NL::NLDB0::MemoryResetMode::AsyncHigh:
        return "async_high";
      case naja::NL::NLDB0::MemoryResetMode::SyncLow:
        return "sync_low";
      case naja::NL::NLDB0::MemoryResetMode::SyncHigh:
        return "sync_high";
    }
    return "unknown"; // LCOV_EXCL_LINE
  }

  std::string getMemoryInternalName(const naja::NL::NLDB0::MemorySignature& signature) {
    std::ostringstream name;
    name << MemoryPrefix
         << "w" << signature.width
         << "_d" << signature.depth
         << "_a" << signature.abits
         << "_r" << signature.readPorts
         << "_w" << signature.writePorts
         << "_rst_" << getMemoryResetModeSuffix(signature.resetMode);
    return name.str();
  }

  void createMemoryPrimitive(
    naja::NL::NLLibrary* rootLibrary,
    const naja::NL::NLDB0::MemorySignature& signature) {
    using namespace naja::NL;
    auto memory = SNLDesign::create(
      rootLibrary,
      SNLDesign::Type::Primitive,
      NLName(getMemoryInternalName(signature)));

    SNLParameter::create(
      memory, NLName("WIDTH"), SNLParameter::Type::Decimal, std::to_string(signature.width));
    SNLParameter::create(
      memory, NLName("DEPTH"), SNLParameter::Type::Decimal, std::to_string(signature.depth));
    SNLParameter::create(
      memory, NLName("ABITS"), SNLParameter::Type::Decimal, std::to_string(signature.abits));
    SNLParameter::create(
      memory,
      NLName("RD_PORTS"),
      SNLParameter::Type::Decimal,
      std::to_string(signature.readPorts));
    SNLParameter::create(
      memory,
      NLName("WR_PORTS"),
      SNLParameter::Type::Decimal,
      std::to_string(signature.writePorts));
    SNLParameter::create(
      memory,
      NLName("RST_ENABLE"),
      SNLParameter::Type::Decimal,
      signature.resetMode == NLDB0::MemoryResetMode::None ? "0" : "1");
    SNLParameter::create(
      memory,
      NLName("RST_ASYNC"),
      SNLParameter::Type::Decimal,
      (signature.resetMode == NLDB0::MemoryResetMode::AsyncLow ||
       signature.resetMode == NLDB0::MemoryResetMode::AsyncHigh)
        ? "1"
        : "0");
    SNLParameter::create(
      memory,
      NLName("RST_ACTIVE_LOW"),
      SNLParameter::Type::Decimal,
      (signature.resetMode == NLDB0::MemoryResetMode::AsyncLow ||
       signature.resetMode == NLDB0::MemoryResetMode::SyncLow)
        ? "1"
        : "0");
    SNLParameter::create(memory, NLName("INIT"), SNLParameter::Type::Binary, "1'b0");

    SNLScalarTerm::create(memory, SNLTerm::Direction::Input, NLName("CLK"));
    SNLScalarTerm::create(memory, SNLTerm::Direction::Input, NLName("RST"));
    SNLBusTerm::create(memory, SNLTerm::Direction::Input, static_cast<NLID::Bit>(signature.readPorts * signature.abits - 1), 0, NLName("RADDR"));
    SNLBusTerm::create(memory, SNLTerm::Direction::Output, static_cast<NLID::Bit>(signature.readPorts * signature.width - 1), 0, NLName("RDATA"));
    SNLBusTerm::create(memory, SNLTerm::Direction::Input, static_cast<NLID::Bit>(signature.writePorts * signature.abits - 1), 0, NLName("WADDR"));
    SNLBusTerm::create(memory, SNLTerm::Direction::Input, static_cast<NLID::Bit>(signature.writePorts * signature.width - 1), 0, NLName("WDATA"));
    SNLBusTerm::create(memory, SNLTerm::Direction::Input, static_cast<NLID::Bit>(signature.writePorts - 1), 0, NLName("WE"));

    auto* clk = memory->getScalarTerm(NLName("CLK"));
    auto* rst = memory->getScalarTerm(NLName("RST"));
    auto* raddr = memory->getBusTerm(NLName("RADDR"));
    auto* rdata = memory->getBusTerm(NLName("RDATA"));
    auto* waddr = memory->getBusTerm(NLName("WADDR"));
    auto* wdata = memory->getBusTerm(NLName("WDATA"));
    auto* we = memory->getBusTerm(NLName("WE"));
    auto rdataBits = collectBitTerms(*rdata);
    SNLDesignModeling::addClockToOutputsArcs(clk, rdataBits);

    SNLDesignModeling::BitTerms writeInputs = collectBitTerms(*waddr);
    auto wdataBits = collectBitTerms(*wdata);
    writeInputs.insert(writeInputs.end(), wdataBits.begin(), wdataBits.end());
    auto weBits = collectBitTerms(*we);
    writeInputs.insert(writeInputs.end(), weBits.begin(), weBits.end());
    writeInputs.push_back(rst);
    SNLDesignModeling::addInputsToClockArcs(writeInputs, clk);

    for (size_t readPort = 0; readPort < signature.readPorts; ++readPort) {
      naja::NL::SNLDesignModeling::BitTerms readAddrBits;
      for (size_t bit = 0; bit < signature.abits; ++bit) {
        readAddrBits.push_back(static_cast<SNLBitTerm*>(
          raddr->getBit(static_cast<NLID::Bit>(readPort * signature.abits + bit))));
      }
      naja::NL::SNLDesignModeling::BitTerms readDataBits;
      for (size_t bit = 0; bit < signature.width; ++bit) {
        readDataBits.push_back(static_cast<SNLBitTerm*>(
          rdata->getBit(static_cast<NLID::Bit>(readPort * signature.width + bit))));
      }
      SNLDesignModeling::addCombinatorialArcs(readAddrBits, readDataBits);
    }
  }

  inline bool parity64(uint64_t x) {
#if defined(_MSC_VER)
    return (__popcnt64(x) & 1) != 0;
#else
    return __builtin_parityll(x);
#endif
  }

  std::vector<uint64_t> getInputFlatTermDependencies(
      const naja::NL::SNLDesign* design) {
    std::vector<size_t> deps;
    size_t flatPos = 0;
    for (auto term : design->getBitTerms()) {
      if (term->getDirection() != naja::NL::SNLTerm::Direction::Output) {
        deps.push_back(flatPos);
      }
      ++flatPos;
    }
    return naja::NL::NLBitDependencies::encodeBits(deps);
  }

  void createAssignPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto assign = SNLDesign::create(rootLibrary, SNLDesign::Type::Primitive);
    auto assignInput = SNLScalarTerm::create(assign, SNLTerm::Direction::Input);
    auto assignOutput = SNLScalarTerm::create(assign, SNLTerm::Direction::Output);

    auto assignFT = SNLScalarNet::create(assign);
    assignInput->setNet(assignFT);
    assignOutput->setNet(assignFT);
    SNLDesignModeling::addCombinatorialArcs({assignInput}, {assignOutput});
  }

  void createFAPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto fa = SNLDesign::create(rootLibrary, SNLDesign::Type::Primitive, NLName("naja_fa"));
    auto inA  = SNLScalarTerm::create(fa, SNLTerm::Direction::Input,  NLName("A"));
    auto inB  = SNLScalarTerm::create(fa, SNLTerm::Direction::Input,  NLName("B"));
    auto inCI = SNLScalarTerm::create(fa, SNLTerm::Direction::Input,  NLName("CI"));
    auto outS = SNLScalarTerm::create(fa, SNLTerm::Direction::Output, NLName("S"));
    auto outCO= SNLScalarTerm::create(fa, SNLTerm::Direction::Output, NLName("CO"));
    SNLDesignModeling::addCombinatorialArcs({inA, inB, inCI}, {outS, outCO});
  }

  void createDFFPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dff = SNLDesign::create(rootLibrary, SNLDesign::Type::Primitive, NLName("naja_dff"));
    auto dffClock = SNLScalarTerm::create(dff, SNLTerm::Direction::Input, NLName("C"));
    auto dffData = SNLScalarTerm::create(dff, SNLTerm::Direction::Input, NLName("D"));
    auto dffOutput = SNLScalarTerm::create(dff, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dffClock, {dffOutput});
    SNLDesignModeling::addInputsToClockArcs({dffData}, dffClock);
  }

  void createDFFRNPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dffrn = SNLDesign::create(rootLibrary, SNLDesign::Type::Primitive, NLName("naja_dffrn"));
    auto dffrnClock = SNLScalarTerm::create(dffrn, SNLTerm::Direction::Input, NLName("C"));
    auto dffrnData = SNLScalarTerm::create(dffrn, SNLTerm::Direction::Input, NLName("D"));
    auto dffrnResetN = SNLScalarTerm::create(dffrn, SNLTerm::Direction::Input, NLName("RN"));
    auto dffrnOutput = SNLScalarTerm::create(dffrn, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dffrnClock, {dffrnOutput});
    SNLDesignModeling::addInputsToClockArcs({dffrnData, dffrnResetN}, dffrnClock);
  }

  void createDFFEPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dffe = SNLDesign::create(rootLibrary, SNLDesign::Type::Primitive, NLName(DFFEName));
    auto dffeClock = SNLScalarTerm::create(dffe, SNLTerm::Direction::Input, NLName("C"));
    auto dffeData = SNLScalarTerm::create(dffe, SNLTerm::Direction::Input, NLName("D"));
    auto dffeEnable = SNLScalarTerm::create(dffe, SNLTerm::Direction::Input, NLName("E"));
    auto dffeOutput = SNLScalarTerm::create(dffe, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dffeClock, {dffeOutput});
    SNLDesignModeling::addInputsToClockArcs({dffeData, dffeEnable}, dffeClock);
  }

  void createDFFREPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dffre = SNLDesign::create(rootLibrary, SNLDesign::Type::Primitive, NLName(DFFREName));
    auto dffreClock = SNLScalarTerm::create(dffre, SNLTerm::Direction::Input, NLName("C"));
    auto dffreData = SNLScalarTerm::create(dffre, SNLTerm::Direction::Input, NLName("D"));
    auto dffreEnable = SNLScalarTerm::create(dffre, SNLTerm::Direction::Input, NLName("E"));
    auto dffreReset = SNLScalarTerm::create(dffre, SNLTerm::Direction::Input, NLName("R"));
    auto dffreOutput = SNLScalarTerm::create(dffre, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dffreClock, {dffreOutput});
    SNLDesignModeling::addInputsToClockArcs({dffreData, dffreEnable, dffreReset}, dffreClock);
  }

  void createDFFSEPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dffse = SNLDesign::create(rootLibrary, SNLDesign::Type::Primitive, NLName(DFFSEName));
    auto dffseClock = SNLScalarTerm::create(dffse, SNLTerm::Direction::Input, NLName("C"));
    auto dffseData = SNLScalarTerm::create(dffse, SNLTerm::Direction::Input, NLName("D"));
    auto dffseEnable = SNLScalarTerm::create(dffse, SNLTerm::Direction::Input, NLName("E"));
    auto dffseSet = SNLScalarTerm::create(dffse, SNLTerm::Direction::Input, NLName("S"));
    auto dffseOutput = SNLScalarTerm::create(dffse, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dffseClock, {dffseOutput});
    SNLDesignModeling::addInputsToClockArcs({dffseData, dffseEnable, dffseSet}, dffseClock);
  }

  void createMux2Primitive(naja::NL::NLLibrary* rootLibrary, size_t width) {
    using namespace naja::NL;
    auto mux2 = SNLDesign::create(
      rootLibrary,
      SNLDesign::Type::Primitive,
      NLName(getMux2InternalName(width)));
    SNLParameter::create(
      mux2, NLName("WIDTH"), SNLParameter::Type::Decimal, std::to_string(width));
    auto inA = SNLBusTerm::create(
      mux2, SNLTerm::Direction::Input, static_cast<NLID::Bit>(width - 1), 0, NLName("A"));
    auto inB = SNLBusTerm::create(
      mux2, SNLTerm::Direction::Input, static_cast<NLID::Bit>(width - 1), 0, NLName("B"));
    auto sel = SNLScalarTerm::create(mux2, SNLTerm::Direction::Input, NLName("S"));
    auto out = SNLBusTerm::create(
      mux2, SNLTerm::Direction::Output, static_cast<NLID::Bit>(width - 1), 0, NLName("Y"));

    std::vector<SNLTruthTable> truthTables(width, getMux2TruthTable());

    for (size_t bit = 0; bit < width; ++bit) {
      auto* inABit = inA->getBit(static_cast<NLID::Bit>(bit));
      auto* inBBit = inB->getBit(static_cast<NLID::Bit>(bit));
      auto* outBit = out->getBit(static_cast<NLID::Bit>(bit));
      SNLDesignModeling::BitTerms inputs;
      inputs.push_back(inABit);
      inputs.push_back(inBBit);
      inputs.push_back(sel);
      SNLDesignModeling::BitTerms outputs;
      outputs.push_back(outBit);
      SNLDesignModeling::addCombinatorialArcs(inputs, outputs);
    }
    SNLDesignModeling::setTruthTables(mux2, truthTables);
  }

  size_t getMemoryDecimalParameter(const naja::NL::SNLDesign* design, const char* name) {
    auto* parameter = design ? design->getParameter(naja::NL::NLName(name)) : nullptr;
    if (!parameter) {
      throw naja::NL::NLException(
          std::string("NLDB0 memory primitive is missing parameter ") + name);
    }
    return static_cast<size_t>(std::stoull(parameter->getValue()));
  }

  size_t getMemoryDecimalParameter(const naja::NL::SNLInstance* instance, const char* name) {
    if (!instance) {
      throw naja::NL::NLException("NLDB0::getMemorySignature: null instance");
    }
    if (auto* instParameter = instance->getInstParameter(naja::NL::NLName(name))) {
      return static_cast<size_t>(std::stoull(instParameter->getValue()));
    }
    return getMemoryDecimalParameter(instance->getModel(), name);
  }

  naja::NL::NLDB0::MemoryResetMode getMemoryResetMode(size_t resetEnable,
                                                      size_t resetAsync,
                                                      size_t resetActiveLow) {
    if (resetEnable == 0) {
      return naja::NL::NLDB0::MemoryResetMode::None;
    }
    if (resetAsync != 0) {
      return resetActiveLow != 0 ? naja::NL::NLDB0::MemoryResetMode::AsyncLow
                                 : naja::NL::NLDB0::MemoryResetMode::AsyncHigh;
    }
    return resetActiveLow != 0 ? naja::NL::NLDB0::MemoryResetMode::SyncLow
                               : naja::NL::NLDB0::MemoryResetMode::SyncHigh;
  }

  bool hasMemoryParameter(const naja::NL::SNLDesign* design, const char* name) {
    return design && design->getParameter(naja::NL::NLName(name)) != nullptr;
  }

  bool hasMemoryInterface(const naja::NL::SNLDesign* design) {
    if (!design) {
      return false;
    }

    return design->getScalarTerm(naja::NL::NLName("CLK")) != nullptr &&
           design->getScalarTerm(naja::NL::NLName("RST")) != nullptr &&
           design->getBusTerm(naja::NL::NLName("RADDR")) != nullptr &&
           design->getBusTerm(naja::NL::NLName("RDATA")) != nullptr &&
           design->getBusTerm(naja::NL::NLName("WADDR")) != nullptr &&
           design->getBusTerm(naja::NL::NLName("WDATA")) != nullptr &&
           design->getBusTerm(naja::NL::NLName("WE")) != nullptr &&
           hasMemoryParameter(design, "WIDTH") &&
           hasMemoryParameter(design, "DEPTH") &&
           hasMemoryParameter(design, "ABITS") &&
           hasMemoryParameter(design, "RD_PORTS") &&
           hasMemoryParameter(design, "WR_PORTS") &&
           hasMemoryParameter(design, "RST_ENABLE") &&
           hasMemoryParameter(design, "RST_ASYNC") &&
           hasMemoryParameter(design, "RST_ACTIVE_LOW") &&
           hasMemoryParameter(design, "INIT");
  }
}

namespace naja::NL {

NLDB0::GateType::GateType(const GateTypeEnum& typeEnum):
  gateTypeEnum_(typeEnum) 
{}

NLDB0::GateType::GateType(const std::string& name):
  gateTypeEnum_(GateType::Unknown) {
  if (name == "and") {
    gateTypeEnum_ = GateType::And;
  } else if (name == "nand") {
    gateTypeEnum_ = GateType::Nand;
  } else if (name == "or") {
    gateTypeEnum_ = GateType::Or;
  } else if (name == "nor") {
    gateTypeEnum_ = GateType::Nor;
  } else if (name == "xor") {
    gateTypeEnum_ = GateType::Xor;
  } else if (name == "xnor") {
    gateTypeEnum_ = GateType::Xnor;
  } else if (name == "buf") {
    gateTypeEnum_ = GateType::Buf;
  } else if (name == "not") {
    gateTypeEnum_ = GateType::Not;
  }
}

bool NLDB0::GateType::isNInput() const {
  return gateTypeEnum_ == GateType::And
    or gateTypeEnum_ == GateType::Nand
    or gateTypeEnum_ == GateType::Or
    or gateTypeEnum_ == GateType::Nor
    or gateTypeEnum_ == GateType::Xor
    or gateTypeEnum_ == GateType::Xnor;
}

bool NLDB0::GateType::isNOutput() const {
  return gateTypeEnum_ == GateType::Buf
    or gateTypeEnum_ == GateType::Not;
}

std::string NLDB0::GateType::getString() const {
  switch (gateTypeEnum_) {
    case GateType::And:
      return "and";
    case GateType::Nand:
      return "nand";
    case GateType::Or:
      return "or";
    case GateType::Nor:
      return "nor";
    case GateType::Xor:
      return "xor";
    case GateType::Xnor:
      return "xnor";
    case GateType::Buf:
      return "buf";
    case GateType::Not:
      return "not";
    case GateType::Unknown:
      return "UNKNOWN";
  }
  return "Bug"; //LCOV_EXCL_LINE
}

NLDB* NLDB0::create(NLUniverse* universe) {
  NLDB* db = NLDB::create(universe);
  assert(db->getID() == 0);

  auto rootLibrary =
    NLLibrary::create(db, NLLibrary::Type::Primitives, NLName(RootLibraryName));

  createAssignPrimitive(rootLibrary);
  createFAPrimitive(rootLibrary);
  createMux2Primitive(rootLibrary, 1);
  createDFFPrimitive(rootLibrary);
  createDFFRNPrimitive(rootLibrary);

  return db;
}

NLDB* NLDB0::getDB0() {
  return NLUniverse::getDB0();
}

bool NLDB0::isDB0(const NLDB* db) {
  return db and db == getDB0();
}

NLLibrary* NLDB0::getDB0RootLibrary() {
  auto db0 = NLDB0::getDB0();
  if (db0) {
    return db0->getLibrary(NLName(RootLibraryName));
  }
  return nullptr;
}

bool NLDB0::isDB0Library(const NLLibrary* library) {
  auto topLibrary = getDB0RootLibrary();
  if (library == topLibrary) {
    return true;
  }
  if (library->isRoot()) {
    return false;
  }
  return isDB0Library(library->getParentLibrary());
}

bool NLDB0::isDB0Primitive(const SNLDesign* design) {
  return design and isDB0Library(design->getLibrary());
}

bool NLDB0::isMemory(const SNLDesign* design) {
  if (!design || design->isUnnamed()) {
    return false;
  }
  const auto& name = design->getName().getString();
  if (name.rfind(MemoryPrefix, 0) != 0) {
    return false;
  }

  // Inferred memories are intended to be DB0 primitives, but once they have
  // the stable internal schema we can still recognize them generically even if
  // they were recreated or copied outside the DB0 root library.
  return hasMemoryInterface(design);
}

NLDB0::MemorySignature NLDB0::getMemorySignature(const SNLDesign* design) {
  if (!isMemory(design)) {
    throw NLException("NLDB0::getMemorySignature: design is not a memory primitive");
  }
  MemorySignature signature;
  signature.width = getMemoryDecimalParameter(design, "WIDTH");
  signature.depth = getMemoryDecimalParameter(design, "DEPTH");
  signature.abits = getMemoryDecimalParameter(design, "ABITS");
  signature.readPorts = getMemoryDecimalParameter(design, "RD_PORTS");
  signature.writePorts = getMemoryDecimalParameter(design, "WR_PORTS");
  signature.resetMode = getMemoryResetMode(
      getMemoryDecimalParameter(design, "RST_ENABLE"),
      getMemoryDecimalParameter(design, "RST_ASYNC"),
      getMemoryDecimalParameter(design, "RST_ACTIVE_LOW"));
  return signature;
}

NLDB0::MemorySignature NLDB0::getMemorySignature(const SNLInstance* instance) {
  if (!instance || !isMemory(instance->getModel())) {
    throw NLException("NLDB0::getMemorySignature: instance is not a memory primitive");
  }
  MemorySignature signature;
  signature.width = getMemoryDecimalParameter(instance, "WIDTH");
  signature.depth = getMemoryDecimalParameter(instance, "DEPTH");
  signature.abits = getMemoryDecimalParameter(instance, "ABITS");
  signature.readPorts = getMemoryDecimalParameter(instance, "RD_PORTS");
  signature.writePorts = getMemoryDecimalParameter(instance, "WR_PORTS");
  signature.resetMode = getMemoryResetMode(
      getMemoryDecimalParameter(instance, "RST_ENABLE"),
      getMemoryDecimalParameter(instance, "RST_ASYNC"),
      getMemoryDecimalParameter(instance, "RST_ACTIVE_LOW"));
  return signature;
}

SNLScalarTerm* NLDB0::getMemoryClock(const SNLDesign* design) {
  if (!isMemory(design)) {
    throw NLException("NLDB0::getMemoryClock: design is not a memory primitive");
  }
  return design->getScalarTerm(NLName("CLK"));
}

SNLScalarTerm* NLDB0::getMemoryReset(const SNLDesign* design) {
  if (!isMemory(design)) {
    throw NLException("NLDB0::getMemoryReset: design is not a memory primitive");
  }
  return design->getScalarTerm(NLName("RST"));
}

SNLBusTerm* NLDB0::getMemoryReadAddress(const SNLDesign* design) {
  if (!isMemory(design)) {
    throw NLException("NLDB0::getMemoryReadAddress: design is not a memory primitive");
  }
  return design->getBusTerm(NLName("RADDR"));
}

SNLBusTerm* NLDB0::getMemoryReadData(const SNLDesign* design) {
  if (!isMemory(design)) {
    throw NLException("NLDB0::getMemoryReadData: design is not a memory primitive");
  }
  return design->getBusTerm(NLName("RDATA"));
}

SNLBusTerm* NLDB0::getMemoryWriteAddress(const SNLDesign* design) {
  if (!isMemory(design)) {
    throw NLException("NLDB0::getMemoryWriteAddress: design is not a memory primitive");
  }
  return design->getBusTerm(NLName("WADDR"));
}

SNLBusTerm* NLDB0::getMemoryWriteData(const SNLDesign* design) {
  if (!isMemory(design)) {
    throw NLException("NLDB0::getMemoryWriteData: design is not a memory primitive");
  }
  return design->getBusTerm(NLName("WDATA"));
}

SNLBusTerm* NLDB0::getMemoryWriteEnable(const SNLDesign* design) {
  if (!isMemory(design)) {
    throw NLException("NLDB0::getMemoryWriteEnable: design is not a memory primitive");
  }
  return design->getBusTerm(NLName("WE"));
}

SNLTruthTable NLDB0::getPrimitiveTruthTable(const SNLDesign* design) {
  if (isMemory(design)) {
    throw NLException("NLDB0::getPrimitiveTruthTable: memory primitive has no truth table");
  }
  if (isAssign(design)) {
    return SNLTruthTable(1, 0b10, getInputFlatTermDependencies(design));
  }
  if (isFA(design)) {
    throw NLException("NLDB0::getPrimitiveTruthTable: FA has two outputs, use getFASumTruthTable/getFACoutTruthTable");
  }
  if (isMux2(design)) {
    return getMux2TruthTable();
  }

  if (isNOutputGate(design)) {
    size_t size = design->getBusTerm(NLID::DesignObjectID(0))->getWidth();
    if (size != 1) {
      throw NLException("NLDB0::getPrimitiveTruthTable: gate with more than 1 output is not supported");
    }
    auto type = GateType(design->getLibrary()->getName().getString());
    const auto deps = getInputFlatTermDependencies(design);
    switch (type) {
      case GateType::Buf:
        return SNLTruthTable(1, 0b10, deps);
      case GateType::Not:
        return SNLTruthTable(1, 0b01, deps);
      // LCOV_EXCL_START
      default:
        break;
      // LCOV_EXCL_STOP
    }
  }

  if (isNInputGate(design)) {
    size_t size = design->getBusTerm(NLID::DesignObjectID(1))->getWidth();
    const auto deps = getInputFlatTermDependencies(design);
    auto type = GateType(design->getLibrary()->getName().getString());
    switch (type) {
      case GateType::And: {
        SNLTruthTable tt(size, SNLTruthTable::GenericType::AND, deps);
        return tt;
      }
      case GateType::Nand: {
        SNLTruthTable tt(size, SNLTruthTable::GenericType::NAND, deps);
        return tt;
      }
      case GateType::Or: {        
        SNLTruthTable tt(size, SNLTruthTable::GenericType::OR, deps);
        return tt;
      }
      case GateType::Nor: {
        SNLTruthTable tt(size, SNLTruthTable::GenericType::NOR, deps);
        return tt;
      }
      case GateType::Xor: {
        SNLTruthTable tt(size, SNLTruthTable::GenericType::XOR, deps);
        return tt;
      }
      case GateType::Xnor: {
        SNLTruthTable tt(size, SNLTruthTable::GenericType::XNOR, deps);
        return tt;  
      }
      // LCOV_EXCL_START
      default:
        break;
      // LCOV_EXCL_STOP
    }
  }

std::string designName = design->getLibrary()->getName().getString() + "." + design->getName().getString();
  throw NLException("NLDB0::getPrimitiveTruthTable: unsupported primitive type: " + designName);
}

SNLDesign* NLDB0::getOrCreateMemory(const MemorySignature& signature) {
  auto* primitives = getDB0RootLibrary();
  if (!primitives) {
    return nullptr;
  }
  if (signature.width == 0 || signature.depth == 0 || signature.abits == 0 ||
      signature.readPorts == 0 || signature.writePorts == 0) {
    throw NLException("NLDB0::getOrCreateMemory: invalid memory signature");
  }
  const auto name = NLName(getMemoryInternalName(signature));
  if (auto* existing = primitives->getSNLDesign(name)) {
    return existing;
  }
  createMemoryPrimitive(primitives, signature);
  return primitives->getSNLDesign(name);
}

SNLDesign* NLDB0::getAssign() {
  auto primitives = getDB0RootLibrary();
  if (primitives) {
    // Static primitive, created first in NLDB0::create().
    return primitives->getSNLDesign(NLID::DesignID(0));
  }
  return nullptr;
}

bool NLDB0::isAssign(const SNLDesign* design) {
  return design and design == getAssign();
}

SNLScalarTerm* NLDB0::getAssignInput() {
  auto assign = getAssign();
  if (assign) {
    // Static term, created first in createAssignPrimitive().
    return assign->getScalarTerm(NLID::DesignObjectID(0));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getAssignOutput() {
  auto assign = getAssign();
  if (assign) {
    // Static term, created second in createAssignPrimitive().
    return assign->getScalarTerm(NLID::DesignObjectID(1));
  }
  return nullptr;
}

SNLDesign* NLDB0::getFA() {
  auto primitives = getDB0RootLibrary();
  if (primitives) {
    // Static primitive, created second in NLDB0::create().
    return primitives->getSNLDesign(NLID::DesignID(1));
  }
  return nullptr;
}

bool NLDB0::isFA(const SNLDesign* design) {
  return design and design == getFA();
}

SNLScalarTerm* NLDB0::getFAInputA() {
  auto fa = getFA();
  if (fa) { return fa->getScalarTerm(NLID::DesignObjectID(0)); }
  return nullptr;
}

SNLScalarTerm* NLDB0::getFAInputB() {
  auto fa = getFA();
  if (fa) { return fa->getScalarTerm(NLID::DesignObjectID(1)); }
  return nullptr;
}

SNLScalarTerm* NLDB0::getFAInputCI() {
  auto fa = getFA();
  if (fa) { return fa->getScalarTerm(NLID::DesignObjectID(2)); }
  return nullptr;
}

SNLScalarTerm* NLDB0::getFAOutputS() {
  auto fa = getFA();
  if (fa) { return fa->getScalarTerm(NLID::DesignObjectID(3)); }
  return nullptr;
}

SNLScalarTerm* NLDB0::getFAOutputCO() {
  auto fa = getFA();
  if (fa) { return fa->getScalarTerm(NLID::DesignObjectID(4)); }
  return nullptr;
}

// Sum = A XOR B XOR CI: odd-parity of 3 inputs
// Row encoding: bit i of 'bits' is the output for input combination i
// i=0b000->0, 001->1, 010->1, 011->0, 100->1, 101->0, 110->0, 111->1
// bits = 0b10010110 = 0x96
SNLTruthTable NLDB0::getFASumTruthTable() {
  auto fa = getFA();
  const auto deps = fa ? getInputFlatTermDependencies(fa)
                       : SNLTruthTable::fullDependencies(3);
  return SNLTruthTable(3, 0x96ULL, deps);
}

// Cout = majority(A,B,CI): output 1 when at least 2 inputs are 1
// i=0b000->0, 001->0, 010->0, 011->1, 100->0, 101->1, 110->1, 111->1
// bits = 0b11101000 = 0xE8
SNLTruthTable NLDB0::getFACoutTruthTable() {
  auto fa = getFA();
  const auto deps = fa ? getInputFlatTermDependencies(fa)
                       : SNLTruthTable::fullDependencies(3);
  return SNLTruthTable(3, 0xE8ULL, deps);
}

SNLDesign* NLDB0::getMux2() {
  auto primitives = getDB0RootLibrary();
  if (primitives) {
    // Static primitive, created third in NLDB0::create().
    return primitives->getSNLDesign(NLID::DesignID(2));
  }
  return nullptr;
}

SNLDesign* NLDB0::getOrCreateMux2(size_t width) {
  auto* primitives = getDB0RootLibrary();
  if (!primitives) {
    return nullptr;
  }
  if (width == 0) {
    throw NLException("NLDB0::getOrCreateMux2: invalid width");
  }
  const auto name = NLName(getMux2InternalName(width));
  if (auto* existing = primitives->getSNLDesign(name)) {
    return existing;
  }
  createMux2Primitive(primitives, width);
  return primitives->getSNLDesign(name);
}

bool NLDB0::isMux2(const SNLDesign* design) {
  if (!isDB0Primitive(design) || !design || design->isUnnamed()) {
    return false;
  }
  const auto& name = design->getName().getString();
  return name.rfind(Mux2Prefix, 0) == 0;
}

SNLBusTerm* NLDB0::getMux2InputA(const SNLDesign* mux2) {
  if (mux2) {
    return mux2->getBusTerm(NLName("A"));
  }
  return nullptr;
}

SNLBusTerm* NLDB0::getMux2InputA() {
  return getMux2InputA(getMux2());
}

SNLBusTerm* NLDB0::getMux2InputB(const SNLDesign* mux2) {
  if (mux2) {
    return mux2->getBusTerm(NLName("B"));
  }
  return nullptr;
}

SNLBusTerm* NLDB0::getMux2InputB() {
  return getMux2InputB(getMux2());
}

SNLScalarTerm* NLDB0::getMux2Select(const SNLDesign* mux2) {
  if (mux2) {
    return mux2->getScalarTerm(NLName("S"));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getMux2Select() {
  return getMux2Select(getMux2());
}

SNLBusTerm* NLDB0::getMux2Output(const SNLDesign* mux2) {
  if (mux2) {
    return mux2->getBusTerm(NLName("Y"));
  }
  return nullptr;
}

SNLBusTerm* NLDB0::getMux2Output() {
  return getMux2Output(getMux2());
}

SNLDesign* NLDB0::getDFF() {
  auto primitives = getDB0RootLibrary();
  if (primitives) {
    // Static primitive, created fourth in NLDB0::create().
    return primitives->getSNLDesign(NLID::DesignID(3));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFClock() {
  auto dff = getDFF();
  if (dff) {
    return dff->getScalarTerm(NLID::DesignObjectID(0));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFData() {
  auto dff = getDFF();
  if (dff) {
    return dff->getScalarTerm(NLID::DesignObjectID(1));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFOutput() {
  auto dff = getDFF();
  if (dff) {
    return dff->getScalarTerm(NLID::DesignObjectID(2));
  }
  return nullptr;
}

SNLDesign* NLDB0::getDFFRN() {
  auto primitives = getDB0RootLibrary();
  if (primitives) {
    // Static primitive, created fifth in NLDB0::create().
    return primitives->getSNLDesign(NLID::DesignID(4));
  }
  return nullptr;
}

bool NLDB0::isDFFRN(const SNLDesign* design) {
  return design and design == getDFFRN();
}

SNLScalarTerm* NLDB0::getDFFRNClock() {
  auto dffrn = getDFFRN();
  if (dffrn) {
    return dffrn->getScalarTerm(NLID::DesignObjectID(0));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFRNData() {
  auto dffrn = getDFFRN();
  if (dffrn) {
    return dffrn->getScalarTerm(NLID::DesignObjectID(1));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFRNResetN() {
  auto dffrn = getDFFRN();
  if (dffrn) {
    return dffrn->getScalarTerm(NLID::DesignObjectID(2));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFRNOutput() {
  auto dffrn = getDFFRN();
  if (dffrn) {
    return dffrn->getScalarTerm(NLID::DesignObjectID(3));
  }
  return nullptr;
}

SNLDesign* NLDB0::getDFFE() {
  return getOrCreateRootPrimitive(DFFEName, createDFFEPrimitive);
}

bool NLDB0::isDFFE(const SNLDesign* design) {
  return isNamedRootPrimitive(design, DFFEName);
}

SNLScalarTerm* NLDB0::getDFFEClock() {
  auto dffe = getDFFE();
  if (dffe) {
    return dffe->getScalarTerm(NLName("C"));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFEData() {
  auto dffe = getDFFE();
  if (dffe) {
    return dffe->getScalarTerm(NLName("D"));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFEEnable() {
  auto dffe = getDFFE();
  if (dffe) {
    return dffe->getScalarTerm(NLName("E"));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFEOutput() {
  auto dffe = getDFFE();
  if (dffe) {
    return dffe->getScalarTerm(NLName("Q"));
  }
  return nullptr;
}

SNLDesign* NLDB0::getDFFRE() {
  return getOrCreateRootPrimitive(DFFREName, createDFFREPrimitive);
}

bool NLDB0::isDFFRE(const SNLDesign* design) {
  return isNamedRootPrimitive(design, DFFREName);
}

SNLScalarTerm* NLDB0::getDFFREClock() {
  auto dffre = getDFFRE();
  if (dffre) {
    return dffre->getScalarTerm(NLName("C"));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFREData() {
  auto dffre = getDFFRE();
  if (dffre) {
    return dffre->getScalarTerm(NLName("D"));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFREEnable() {
  auto dffre = getDFFRE();
  if (dffre) {
    return dffre->getScalarTerm(NLName("E"));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFREReset() {
  auto dffre = getDFFRE();
  if (dffre) {
    return dffre->getScalarTerm(NLName("R"));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFREOutput() {
  auto dffre = getDFFRE();
  if (dffre) {
    return dffre->getScalarTerm(NLName("Q"));
  }
  return nullptr;
}

SNLDesign* NLDB0::getDFFSE() {
  return getOrCreateRootPrimitive(DFFSEName, createDFFSEPrimitive);
}

bool NLDB0::isDFFSE(const SNLDesign* design) {
  return isNamedRootPrimitive(design, DFFSEName);
}

SNLScalarTerm* NLDB0::getDFFSEClock() {
  auto dffse = getDFFSE();
  if (dffse) {
    return dffse->getScalarTerm(NLName("C"));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFSEData() {
  auto dffse = getDFFSE();
  if (dffse) {
    return dffse->getScalarTerm(NLName("D"));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFSEEnable() {
  auto dffse = getDFFSE();
  if (dffse) {
    return dffse->getScalarTerm(NLName("E"));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFSESet() {
  auto dffse = getDFFSE();
  if (dffse) {
    return dffse->getScalarTerm(NLName("S"));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFSEOutput() {
  auto dffse = getDFFSE();
  if (dffse) {
    return dffse->getScalarTerm(NLName("Q"));
  }
  return nullptr;
}

NLLibrary* NLDB0::getOrCreateGateLibrary(const GateType& type) {
  auto gateLib = getGateLibrary(type);
  if (gateLib) {
    return gateLib;
  }
  auto rootLib = getDB0RootLibrary();
  if (not rootLib) {
    return nullptr;
  }
  return NLLibrary::create(getDB0RootLibrary(), NLLibrary::Type::Primitives, NLName(type.getString()));
}

NLLibrary* NLDB0::getGateLibrary(const GateType& type) {
  auto rootLib = getDB0RootLibrary();
  if (not rootLib) {
    return nullptr;
  }
  auto gateLib = rootLib->getLibrary(NLName(type.getString()));
  if (gateLib) {
    return gateLib;
  }
  return nullptr;
}

bool NLDB0::isGateLibrary(const NLLibrary* library) {
  if (library->isRoot()) {
    return false;
  }
  auto rootLib = getDB0RootLibrary();
  if (library->getParentLibrary() != rootLib) {
    return false;
  }
  auto type = GateType(library->getName().getString());
  return type != GateType::Unknown;
}

SNLDesign* NLDB0::getOrCreateNOutputGate(const GateType& type, size_t nbOutputs) {
  if (not type.isNOutput()) {
    throw NLException(
      "NLDB0::getOrCreateNOutputGate: type " + type.getString() + " is not an NOutput gate" 
    );
  }
  if (nbOutputs == 0) {
    throw NLException("NLDB0::getOrCreateNOutputGate: nbOutputs is 0");
  }
  auto gateLibrary = getOrCreateGateLibrary(type);
  if (not gateLibrary) {
    throw NLException("NLDB0::getOrCreateNOutputGate: cannot create gate library");
  }
  std::string gateName(type.getString() + "_" + std::to_string(nbOutputs));
  auto gate = gateLibrary->getSNLDesign(NLName(gateName));
  if (not gate) {
    gate = SNLDesign::create(gateLibrary, SNLDesign::Type::Primitive, NLName(gateName));
    SNLBusTerm::create(gate, SNLTerm::Direction::Output, NLID::Bit(nbOutputs-1), 0);
    SNLScalarTerm::create(gate, SNLTerm::Direction::Input);
  }
  return gate;
}

SNLDesign* NLDB0::getOrCreateNInputGate(const GateType& type, size_t nbInputs) {
  if (not type.isNInput()) {
    throw NLException("NLDB0::getOrCreateNInputGate: type is not an NInput gate");
  }
  if (nbInputs == 0) {
    throw NLException("NLDB0::getOrCreateNInputGate: nbInputs is 0");
  }
  auto gateLibrary = getOrCreateGateLibrary(type);
  if (not gateLibrary) {
    throw NLException("NLDB0::getOrCreateNInputGate: cannot create gate library");
  }
  std::string gateName(type.getString() + "_" + std::to_string(nbInputs));
  auto gate = gateLibrary->getSNLDesign(NLName(gateName));
  if (not gate) {
    gate = SNLDesign::create(gateLibrary, SNLDesign::Type::Primitive, NLName(gateName));
    SNLScalarTerm::create(gate, SNLTerm::Direction::Output);
    SNLBusTerm::create(gate, SNLTerm::Direction::Input, NLID::Bit(nbInputs-1), 0);
  }
  return gate;
}

std::string NLDB0::getGateName(const SNLDesign* design) {
  if (not isGate(design)) {
    return std::string();
  }
  auto lib = design->getLibrary();
  auto type = GateType(lib->getName().getString());
  return type.getString();
}

bool NLDB0::isNInputGate(const SNLDesign* design) {
  if (not design) {
    return false;
  }
  auto lib = design->getLibrary();
  if (not isGateLibrary(lib)) {
    return false;
  }
  auto type = GateType(lib->getName().getString());
  return type.isNInput();
}

bool NLDB0::isNOutputGate(const SNLDesign* design) {
  if (not design) {
    return false;
  }
  auto lib = design->getLibrary();
  if (not isGateLibrary(lib)) {
    return false;
  }
  auto type = GateType(lib->getName().getString());
  return type.isNOutput();
}

bool NLDB0::isGate(const SNLDesign* design) {
  if (not design) {
    return false;
  }
  auto lib = design->getLibrary();
  if (not isGateLibrary(lib)) {
    return false;
  }
  auto type = GateType(lib->getName().getString());
  return type != GateType::Unknown;
}

SNLScalarTerm* NLDB0::getGateSingleTerm(const SNLDesign* gate) {
  if (isNInputGate(gate)) {
    return gate->getScalarTerm(NLID::DesignObjectID(0));
  } else if (isNOutputGate(gate)) {
    return gate->getScalarTerm(NLID::DesignObjectID(1));
  }
  return nullptr;
}

SNLBusTerm* NLDB0::getGateNTerms(const SNLDesign* gate) {
  if (isNInputGate(gate)) {
    return gate->getBusTerm(NLID::DesignObjectID(1));
  } else if (isNOutputGate(gate)) {
    return gate->getBusTerm(NLID::DesignObjectID(0));
  }
  return nullptr;
}

}  // namespace naja::NL
