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
#include <cctype>
#include <limits>
#include <sstream>
#include <vector>
#if defined(_MSC_VER)
  #include <intrin.h>
#endif
namespace {
  constexpr naja::NL::NLID::LibraryID RootLibraryID = 0;
  constexpr naja::NL::NLID::LibraryID AssignLibraryID = 1;
  constexpr naja::NL::NLID::LibraryID Mux2LibraryID = 2;
  //All DFF primitive families share one contiguous canonical ID range [3, 10].
  constexpr naja::NL::NLID::LibraryID DFFLibraryID = 3;
  constexpr naja::NL::NLID::LibraryID DFFNLibraryID = 4;
  constexpr naja::NL::NLID::LibraryID DFFRNLibraryID = 5;
  constexpr naja::NL::NLID::LibraryID DFFRLibraryID = 6;
  constexpr naja::NL::NLID::LibraryID DFFSLibraryID = 7;
  constexpr naja::NL::NLID::LibraryID DFFELibraryID = 8;
  constexpr naja::NL::NLID::LibraryID DFFRELibraryID = 9;
  constexpr naja::NL::NLID::LibraryID DFFSELibraryID = 10;
  constexpr naja::NL::NLID::LibraryID DLatchLibraryID = 11;
  constexpr naja::NL::NLID::LibraryID DivModLibraryID = 12;
  constexpr naja::NL::NLID::LibraryID DivModUnsignedLibraryID = 13;
  constexpr naja::NL::NLID::LibraryID DivModSignedLibraryID = 14;
  constexpr naja::NL::NLID::LibraryID TableSelectLibraryID = 15;
  constexpr naja::NL::NLID::LibraryID MemoryLibraryID = 16;
  constexpr naja::NL::NLID::LibraryID DFFSRLibraryID = 17;
  constexpr naja::NL::NLID::LibraryID DFFSRNLibraryID = 18;
  constexpr naja::NL::NLID::LibraryID DFFSSLibraryID = 19;
  constexpr naja::NL::NLID::LibraryID DFFSSNLibraryID = 20;
  constexpr naja::NL::NLID::LibraryID DFFSRELibraryID = 21;
  constexpr naja::NL::NLID::LibraryID DFFSRNELibraryID = 22;
  constexpr naja::NL::NLID::LibraryID DFFSSELibraryID = 23;
  constexpr naja::NL::NLID::LibraryID DFFSSNELibraryID = 24;
  //Gate libraries get one reserved canonical ID per GateType so that a model
  //reference into DB0 is self-describing: libraryID = GateLibraryIDBase + GateType,
  //designID = gate fan-in (N-input) or fan-out (N-output).
  constexpr naja::NL::NLID::LibraryID GateLibraryIDBase = 25;

  constexpr naja::NL::NLID::DesignID ScalarPrimitiveDesignID = 1;
  constexpr naja::NL::NLID::DesignID FADesignID = 1;

  constexpr naja::NL::NLID::DesignObjectID Term0ID = 0;
  constexpr naja::NL::NLID::DesignObjectID Term1ID = 1;
  constexpr naja::NL::NLID::DesignObjectID Term2ID = 2;
  constexpr naja::NL::NLID::DesignObjectID Term3ID = 3;
  constexpr naja::NL::NLID::DesignObjectID Term4ID = 4;

  constexpr const char* AssignName = "assign";
  constexpr const char* FAName = "naja_fa";
  constexpr const char* MemoryPrefix = "naja_mem__";
  constexpr const char* MemoryLibraryName = "MEMORY";
  constexpr const char* DivModLibraryName = "naja_divmod";
  constexpr const char* DivModUnsignedLibraryName = "unsigned";
  constexpr const char* DivModSignedLibraryName = "signed";
  constexpr const char* DivModPrefix = "naja_divmod__";
  constexpr const char* TableSelectName = "naja_table_select";
  constexpr const char* TableSelectPrefix = "naja_table_select__";
  constexpr const char* Mux2Name = "naja_mux2";
  constexpr const char* Mux2ScalarName = "naja_mux2__w1";
  constexpr const char* Mux2Prefix = "naja_mux2__w";
  constexpr const char* DFFName = "naja_dff";
  constexpr const char* DFFPrefix = "naja_dff__w";
  constexpr const char* DLatchName = "naja_dlatch";
  constexpr const char* DLatchPrefix = "naja_dlatch__w";
  constexpr const char* DFFNName = "naja_dffn";
  constexpr const char* DFFNPrefix = "naja_dffn__w";
  constexpr const char* DFFRNName = "naja_dffrn";
  constexpr const char* DFFRNPrefix = "naja_dffrn__w";
  constexpr const char* DFFRName = "naja_dffr";
  constexpr const char* DFFRPrefix = "naja_dffr__w";
  constexpr const char* DFFSName = "naja_dffs";
  constexpr const char* DFFSPrefix = "naja_dffs__w";
  constexpr const char* DFFEName = "naja_dffe";
  constexpr const char* DFFEPrefix = "naja_dffe__w";
  constexpr const char* DFFREName = "naja_dffre";
  constexpr const char* DFFREPrefix = "naja_dffre__w";
  constexpr const char* DFFSEName = "naja_dffse";
  constexpr const char* DFFSEPrefix = "naja_dffse__w";
  constexpr const char* DFFSRName = "naja_dffsr";
  constexpr const char* DFFSRPrefix = "naja_dffsr__w";
  constexpr const char* DFFSRNName = "naja_dffsrn";
  constexpr const char* DFFSRNPrefix = "naja_dffsrn__w";
  constexpr const char* DFFSSName = "naja_dffss";
  constexpr const char* DFFSSPrefix = "naja_dffss__w";
  constexpr const char* DFFSSNName = "naja_dffssn";
  constexpr const char* DFFSSNPrefix = "naja_dffssn__w";
  constexpr const char* DFFSREName = "naja_dffsre";
  constexpr const char* DFFSREPrefix = "naja_dffsre__w";
  constexpr const char* DFFSRNEName = "naja_dffsrne";
  constexpr const char* DFFSRNEPrefix = "naja_dffsrne__w";
  constexpr const char* DFFSSEName = "naja_dffsse";
  constexpr const char* DFFSSEPrefix = "naja_dffsse__w";
  constexpr const char* DFFSSNEName = "naja_dffssne";
  constexpr const char* DFFSSNEPrefix = "naja_dffssne__w";

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

  std::string getWidthInternalName(const char* prefix, size_t width) {
    std::ostringstream name;
    name << prefix << width;
    return name.str();
  }

  void addDFFInitParameter(naja::NL::SNLDesign* design, size_t width) {
    naja::NL::SNLParameter::create(
      design,
      naja::NL::NLName("INIT"),
      naja::NL::SNLParameter::Type::Binary,
      naja::NL::NLDB0::getUndefinedDFFInitValue(width));
  }

  std::string getDivModInternalName(const naja::NL::NLDB0::DivModSignature& signature) {
    std::ostringstream name;
    name << DivModPrefix
         << (signature.isSigned ? "s" : "u")
         << "_w" << signature.width;
    return name.str();
  }

  std::string getTableSelectInternalName(
      const naja::NL::NLDB0::TableSelectSignature& signature) {
    std::ostringstream name;
    name << TableSelectPrefix
         << "w" << signature.width
         << "_d" << signature.depth
         << "_a" << signature.abits;
    return name.str();
  }

  naja::NL::NLLibrary* getRootLibraryByID() {
    auto* db0 = naja::NL::NLDB0::getDB0();
    if (!db0) {
      return nullptr;
    }
    return db0->getLibrary(naja::NL::NLID::LibraryID(RootLibraryID));
  }

  naja::NL::NLLibrary* getPrimitiveLibrary(naja::NL::NLID::LibraryID id) {
    auto* rootLibrary = naja::NL::NLDB0::getDB0RootLibrary();
    if (!rootLibrary) {
      return nullptr;
    }
    return rootLibrary->getLibrary(id);
  }

  naja::NL::NLLibrary* getOrCreatePrimitiveLibrary(
      naja::NL::NLID::LibraryID id,
      const char* name) {
    if (auto* existing = getPrimitiveLibrary(id)) {
      return existing;
    }
    auto* rootLibrary = naja::NL::NLDB0::getDB0RootLibrary();
    if (!rootLibrary) {
      return nullptr;
    }
    return naja::NL::NLLibrary::create(
      rootLibrary,
      id,
      naja::NL::NLLibrary::Type::Primitives,
      naja::NL::NLName(name));
  }

  naja::NL::NLLibrary* getDivModSignednessLibrary(bool isSigned) {
    auto* divModLibrary = getPrimitiveLibrary(DivModLibraryID);
    if (!divModLibrary) {
      return nullptr;
    }
    return divModLibrary->getLibrary(
      isSigned ? DivModSignedLibraryID : DivModUnsignedLibraryID);
  }

  naja::NL::NLLibrary* getOrCreateDivModSignednessLibrary(bool isSigned) {
    if (auto* existing = getDivModSignednessLibrary(isSigned)) {
      return existing;
    }
    auto* divModLibrary = getOrCreatePrimitiveLibrary(DivModLibraryID, DivModLibraryName);
    if (!divModLibrary) {
      return nullptr;
    }
    return naja::NL::NLLibrary::create(
      divModLibrary,
      isSigned ? DivModSignedLibraryID : DivModUnsignedLibraryID,
      naja::NL::NLLibrary::Type::Primitives,
      naja::NL::NLName(isSigned ? DivModSignedLibraryName : DivModUnsignedLibraryName));
  }

  bool isPrimitiveInLibrary(
      const naja::NL::SNLDesign* design,
      naja::NL::NLID::LibraryID libraryID) {
    return design && design->isPrimitive() && naja::NL::NLDB0::isDB0Primitive(design) &&
           design->getLibrary() == getPrimitiveLibrary(libraryID);
  }

  bool isWidthPrimitive(
      const naja::NL::SNLDesign* design,
      naja::NL::NLID::LibraryID libraryID,
      const char* scalarName,
      const char* prefix) {
    if (!isPrimitiveInLibrary(design, libraryID) || design->isUnnamed()) {
      return false;
    }
    const auto& name = design->getName().getString();
    return (design->getID() == ScalarPrimitiveDesignID && name == scalarName) ||
           name.rfind(prefix, 0) == 0;
  }

  template<typename CreatePrimitive>
  naja::NL::SNLDesign* getOrCreateWidthPrimitive(
    naja::NL::NLID::LibraryID libraryID,
    const char* libraryName,
    const char* scalarName,
    const char* prefix,
    size_t width,
    CreatePrimitive createPrimitive) {
    auto* primitiveLibrary = getOrCreatePrimitiveLibrary(libraryID, libraryName);
    if (!primitiveLibrary) {
      return nullptr;
    }
    if (width == 0) {
      throw naja::NL::NLException("NLDB0 width primitive: invalid width");
    }
    if (width > std::numeric_limits<naja::NL::NLID::DesignID>::max()) {
      throw naja::NL::NLException("NLDB0 width primitive: width does not fit a design ID");
    }
    if (width == 1) {
      return primitiveLibrary->getSNLDesign(naja::NL::NLID::DesignID(ScalarPrimitiveDesignID));
    }
    const auto id = naja::NL::NLID::DesignID(width);
    if (auto* existing = primitiveLibrary->getSNLDesign(id)) {
      return existing;
    }
    createPrimitive(primitiveLibrary, width);
    return primitiveLibrary->getSNLDesign(id);
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
    naja::NL::NLLibrary* memoryLibrary,
    const naja::NL::NLDB0::MemorySignature& signature) {
    using namespace naja::NL;
    auto memory = SNLDesign::create(
      memoryLibrary,
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
    SNLParameter::create(
      memory, NLName("INIT_ENABLE"), SNLParameter::Type::Decimal, "0");
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

  void createDivModPrimitive(
    naja::NL::NLLibrary* divModSignednessLibrary,
    const naja::NL::NLDB0::DivModSignature& signature) {
    using namespace naja::NL;
    auto divmod = SNLDesign::create(
      divModSignednessLibrary,
      NLID::DesignID(signature.width),
      SNLDesign::Type::Primitive,
      NLName(getDivModInternalName(signature)));

    SNLParameter::create(
      divmod, NLName("WIDTH"), SNLParameter::Type::Decimal, std::to_string(signature.width));
    SNLParameter::create(
      divmod,
      NLName("SIGNED"),
      SNLParameter::Type::Decimal,
      signature.isSigned ? "1" : "0");

    auto dividend = SNLBusTerm::create(
      divmod,
      Term0ID,
      SNLTerm::Direction::Input,
      static_cast<NLID::Bit>(signature.width - 1),
      0,
      NLName("A"));
    auto divisor = SNLBusTerm::create(
      divmod,
      Term1ID,
      SNLTerm::Direction::Input,
      static_cast<NLID::Bit>(signature.width - 1),
      0,
      NLName("B"));
    auto quotient = SNLBusTerm::create(
      divmod,
      Term2ID,
      SNLTerm::Direction::Output,
      static_cast<NLID::Bit>(signature.width - 1),
      0,
      NLName("Q"));
    auto remainder = SNLBusTerm::create(
      divmod,
      Term3ID,
      SNLTerm::Direction::Output,
      static_cast<NLID::Bit>(signature.width - 1),
      0,
      NLName("R"));

    SNLDesignModeling::BitTerms inputs;
    auto dividendBits = collectBitTerms(*dividend);
    auto divisorBits = collectBitTerms(*divisor);
    inputs.insert(inputs.end(), dividendBits.begin(), dividendBits.end());
    inputs.insert(inputs.end(), divisorBits.begin(), divisorBits.end());

    SNLDesignModeling::BitTerms outputs;
    auto quotientBits = collectBitTerms(*quotient);
    auto remainderBits = collectBitTerms(*remainder);
    outputs.insert(outputs.end(), quotientBits.begin(), quotientBits.end());
    outputs.insert(outputs.end(), remainderBits.begin(), remainderBits.end());

    SNLDesignModeling::addCombinatorialArcs(inputs, outputs);
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
    auto assign = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(ScalarPrimitiveDesignID),
      SNLDesign::Type::Primitive,
      NLName(AssignName));
    auto assignInput = SNLScalarTerm::create(
      assign, Term0ID, SNLTerm::Direction::Input);
    auto assignOutput = SNLScalarTerm::create(
      assign, Term1ID, SNLTerm::Direction::Output);

    auto assignFT = SNLScalarNet::create(assign);
    assignInput->setNet(assignFT);
    assignOutput->setNet(assignFT);
    SNLDesignModeling::addCombinatorialArcs({assignInput}, {assignOutput});
  }

  void createFAPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto fa = SNLDesign::create(
      rootLibrary, NLID::DesignID(FADesignID), SNLDesign::Type::Primitive, NLName(FAName));
    auto inA  = SNLScalarTerm::create(fa, Term0ID, SNLTerm::Direction::Input,  NLName("A"));
    auto inB  = SNLScalarTerm::create(fa, Term1ID, SNLTerm::Direction::Input,  NLName("B"));
    auto inCI = SNLScalarTerm::create(fa, Term2ID, SNLTerm::Direction::Input,  NLName("CI"));
    auto outS = SNLScalarTerm::create(fa, Term3ID, SNLTerm::Direction::Output, NLName("S"));
    auto outCO= SNLScalarTerm::create(fa, Term4ID, SNLTerm::Direction::Output, NLName("CO"));
    SNLDesignModeling::addCombinatorialArcs({inA, inB, inCI}, {outS, outCO});
  }

  void createDFFPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dff = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(ScalarPrimitiveDesignID),
      SNLDesign::Type::Primitive,
      NLName(DFFName));
    SNLParameter::create(dff, NLName("WIDTH"), SNLParameter::Type::Decimal, "1");
    addDFFInitParameter(dff, 1);
    auto dffClock = SNLScalarTerm::create(dff, Term0ID, SNLTerm::Direction::Input, NLName("C"));
    auto dffData = SNLScalarTerm::create(dff, Term1ID, SNLTerm::Direction::Input, NLName("D"));
    auto dffOutput = SNLScalarTerm::create(dff, Term2ID, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dffClock, {dffOutput});
    SNLDesignModeling::addInputsToClockArcs({dffData}, dffClock);
    SNLDesignModeling::setTermRole(dffClock, SNLDesignModeling::SNLTermRole::Clock);
    SNLDesignModeling::setTermRole(dffData, SNLDesignModeling::SNLTermRole::DataInput);
    SNLDesignModeling::setTermRole(dffOutput, SNLDesignModeling::SNLTermRole::DataOutput);
  }

  void createDLatchPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dlatch = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(ScalarPrimitiveDesignID),
      SNLDesign::Type::Primitive,
      NLName(DLatchName));
    SNLParameter::create(dlatch, NLName("WIDTH"), SNLParameter::Type::Decimal, "1");
    auto dlatchEnable = SNLScalarTerm::create(dlatch, Term0ID, SNLTerm::Direction::Input, NLName("E"));
    auto dlatchData = SNLScalarTerm::create(dlatch, Term1ID, SNLTerm::Direction::Input, NLName("D"));
    auto dlatchOutput = SNLScalarTerm::create(dlatch, Term2ID, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dlatchEnable, {dlatchOutput});
    SNLDesignModeling::addInputsToClockArcs({dlatchData}, dlatchEnable);
    SNLDesignModeling::setTermRole(dlatchEnable, SNLDesignModeling::SNLTermRole::Enable);
    SNLDesignModeling::setTermRole(dlatchData, SNLDesignModeling::SNLTermRole::DataInput);
    SNLDesignModeling::setTermRole(dlatchOutput, SNLDesignModeling::SNLTermRole::DataOutput);
  }

  void createDFFNPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dffn = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(ScalarPrimitiveDesignID),
      SNLDesign::Type::Primitive,
      NLName(DFFNName));
    SNLParameter::create(dffn, NLName("WIDTH"), SNLParameter::Type::Decimal, "1");
    addDFFInitParameter(dffn, 1);
    auto dffnClock = SNLScalarTerm::create(dffn, Term0ID, SNLTerm::Direction::Input, NLName("C"));
    auto dffnData = SNLScalarTerm::create(dffn, Term1ID, SNLTerm::Direction::Input, NLName("D"));
    auto dffnOutput = SNLScalarTerm::create(dffn, Term2ID, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dffnClock, {dffnOutput});
    SNLDesignModeling::addInputsToClockArcs({dffnData}, dffnClock);
    SNLDesignModeling::setTermRole(dffnClock, SNLDesignModeling::SNLTermRole::Clock);
    SNLDesignModeling::setTermRole(dffnData, SNLDesignModeling::SNLTermRole::DataInput);
    SNLDesignModeling::setTermRole(dffnOutput, SNLDesignModeling::SNLTermRole::DataOutput);
  }

  void createDFFRNPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dffrn = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(ScalarPrimitiveDesignID),
      SNLDesign::Type::Primitive,
      NLName(DFFRNName));
    SNLParameter::create(dffrn, NLName("WIDTH"), SNLParameter::Type::Decimal, "1");
    addDFFInitParameter(dffrn, 1);
    auto dffrnClock = SNLScalarTerm::create(dffrn, Term0ID, SNLTerm::Direction::Input, NLName("C"));
    auto dffrnData = SNLScalarTerm::create(dffrn, Term1ID, SNLTerm::Direction::Input, NLName("D"));
    auto dffrnResetN = SNLScalarTerm::create(dffrn, Term2ID, SNLTerm::Direction::Input, NLName("RN"));
    auto dffrnOutput = SNLScalarTerm::create(dffrn, Term3ID, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dffrnClock, {dffrnOutput});
    SNLDesignModeling::addInputsToClockArcs({dffrnData, dffrnResetN}, dffrnClock);
    SNLDesignModeling::setTermRole(dffrnClock, SNLDesignModeling::SNLTermRole::Clock);
    SNLDesignModeling::setTermRole(dffrnData, SNLDesignModeling::SNLTermRole::DataInput);
    SNLDesignModeling::setTermRole(dffrnOutput, SNLDesignModeling::SNLTermRole::DataOutput);
    SNLDesignModeling::setTermRole(dffrnResetN, SNLDesignModeling::SNLTermRole::AsyncReset, SNLDesignModeling::SNLActiveLevel::Low);
  }

  void createDFFRPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dffr = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(ScalarPrimitiveDesignID),
      SNLDesign::Type::Primitive,
      NLName(DFFRName));
    SNLParameter::create(dffr, NLName("WIDTH"), SNLParameter::Type::Decimal, "1");
    addDFFInitParameter(dffr, 1);
    auto dffrClock = SNLScalarTerm::create(dffr, Term0ID, SNLTerm::Direction::Input, NLName("C"));
    auto dffrData = SNLScalarTerm::create(dffr, Term1ID, SNLTerm::Direction::Input, NLName("D"));
    auto dffrReset = SNLScalarTerm::create(dffr, Term2ID, SNLTerm::Direction::Input, NLName("R"));
    auto dffrOutput = SNLScalarTerm::create(dffr, Term3ID, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dffrClock, {dffrOutput});
    SNLDesignModeling::addInputsToClockArcs({dffrData, dffrReset}, dffrClock);
    SNLDesignModeling::setTermRole(dffrClock, SNLDesignModeling::SNLTermRole::Clock);
    SNLDesignModeling::setTermRole(dffrData, SNLDesignModeling::SNLTermRole::DataInput);
    SNLDesignModeling::setTermRole(dffrOutput, SNLDesignModeling::SNLTermRole::DataOutput);
    SNLDesignModeling::setTermRole(dffrReset, SNLDesignModeling::SNLTermRole::AsyncReset, SNLDesignModeling::SNLActiveLevel::High);
  }

  void createDFFSPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dffs = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(ScalarPrimitiveDesignID),
      SNLDesign::Type::Primitive,
      NLName(DFFSName));
    SNLParameter::create(dffs, NLName("WIDTH"), SNLParameter::Type::Decimal, "1");
    addDFFInitParameter(dffs, 1);
    auto dffsClock = SNLScalarTerm::create(dffs, Term0ID, SNLTerm::Direction::Input, NLName("C"));
    auto dffsData = SNLScalarTerm::create(dffs, Term1ID, SNLTerm::Direction::Input, NLName("D"));
    auto dffsSet = SNLScalarTerm::create(dffs, Term2ID, SNLTerm::Direction::Input, NLName("S"));
    auto dffsOutput = SNLScalarTerm::create(dffs, Term3ID, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dffsClock, {dffsOutput});
    SNLDesignModeling::addInputsToClockArcs({dffsData, dffsSet}, dffsClock);
    SNLDesignModeling::setTermRole(dffsClock, SNLDesignModeling::SNLTermRole::Clock);
    SNLDesignModeling::setTermRole(dffsData, SNLDesignModeling::SNLTermRole::DataInput);
    SNLDesignModeling::setTermRole(dffsOutput, SNLDesignModeling::SNLTermRole::DataOutput);
    SNLDesignModeling::setTermRole(dffsSet, SNLDesignModeling::SNLTermRole::AsyncSet, SNLDesignModeling::SNLActiveLevel::High);
  }

  void createDFFEPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dffe = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(ScalarPrimitiveDesignID),
      SNLDesign::Type::Primitive,
      NLName(DFFEName));
    SNLParameter::create(dffe, NLName("WIDTH"), SNLParameter::Type::Decimal, "1");
    addDFFInitParameter(dffe, 1);
    auto dffeClock = SNLScalarTerm::create(dffe, Term0ID, SNLTerm::Direction::Input, NLName("C"));
    auto dffeData = SNLScalarTerm::create(dffe, Term1ID, SNLTerm::Direction::Input, NLName("D"));
    auto dffeEnable = SNLScalarTerm::create(dffe, Term2ID, SNLTerm::Direction::Input, NLName("E"));
    auto dffeOutput = SNLScalarTerm::create(dffe, Term3ID, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dffeClock, {dffeOutput});
    SNLDesignModeling::addInputsToClockArcs({dffeData, dffeEnable}, dffeClock);
    SNLDesignModeling::setTermRole(dffeClock, SNLDesignModeling::SNLTermRole::Clock);
    SNLDesignModeling::setTermRole(dffeData, SNLDesignModeling::SNLTermRole::DataInput);
    SNLDesignModeling::setTermRole(dffeOutput, SNLDesignModeling::SNLTermRole::DataOutput);
    SNLDesignModeling::setTermRole(dffeEnable, SNLDesignModeling::SNLTermRole::Enable);
  }

  void createDFFREPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dffre = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(ScalarPrimitiveDesignID),
      SNLDesign::Type::Primitive,
      NLName(DFFREName));
    SNLParameter::create(dffre, NLName("WIDTH"), SNLParameter::Type::Decimal, "1");
    addDFFInitParameter(dffre, 1);
    auto dffreClock = SNLScalarTerm::create(dffre, Term0ID, SNLTerm::Direction::Input, NLName("C"));
    auto dffreData = SNLScalarTerm::create(dffre, Term1ID, SNLTerm::Direction::Input, NLName("D"));
    auto dffreEnable = SNLScalarTerm::create(dffre, Term2ID, SNLTerm::Direction::Input, NLName("E"));
    auto dffreReset = SNLScalarTerm::create(dffre, Term3ID, SNLTerm::Direction::Input, NLName("R"));
    auto dffreOutput = SNLScalarTerm::create(dffre, Term4ID, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dffreClock, {dffreOutput});
    SNLDesignModeling::addInputsToClockArcs({dffreData, dffreEnable, dffreReset}, dffreClock);
    SNLDesignModeling::setTermRole(dffreClock, SNLDesignModeling::SNLTermRole::Clock);
    SNLDesignModeling::setTermRole(dffreData, SNLDesignModeling::SNLTermRole::DataInput);
    SNLDesignModeling::setTermRole(dffreOutput, SNLDesignModeling::SNLTermRole::DataOutput);
    SNLDesignModeling::setTermRole(dffreEnable, SNLDesignModeling::SNLTermRole::Enable);
    SNLDesignModeling::setTermRole(dffreReset, SNLDesignModeling::SNLTermRole::AsyncReset, SNLDesignModeling::SNLActiveLevel::High);
  }

  void createDFFSEPrimitive(naja::NL::NLLibrary* rootLibrary) {
    using namespace naja::NL;
    auto dffse = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(ScalarPrimitiveDesignID),
      SNLDesign::Type::Primitive,
      NLName(DFFSEName));
    SNLParameter::create(dffse, NLName("WIDTH"), SNLParameter::Type::Decimal, "1");
    addDFFInitParameter(dffse, 1);
    auto dffseClock = SNLScalarTerm::create(dffse, Term0ID, SNLTerm::Direction::Input, NLName("C"));
    auto dffseData = SNLScalarTerm::create(dffse, Term1ID, SNLTerm::Direction::Input, NLName("D"));
    auto dffseEnable = SNLScalarTerm::create(dffse, Term2ID, SNLTerm::Direction::Input, NLName("E"));
    auto dffseSet = SNLScalarTerm::create(dffse, Term3ID, SNLTerm::Direction::Input, NLName("S"));
    auto dffseOutput = SNLScalarTerm::create(dffse, Term4ID, SNLTerm::Direction::Output, NLName("Q"));
    SNLDesignModeling::addClockToOutputsArcs(dffseClock, {dffseOutput});
    SNLDesignModeling::addInputsToClockArcs({dffseData, dffseEnable, dffseSet}, dffseClock);
    SNLDesignModeling::setTermRole(dffseClock, SNLDesignModeling::SNLTermRole::Clock);
    SNLDesignModeling::setTermRole(dffseData, SNLDesignModeling::SNLTermRole::DataInput);
    SNLDesignModeling::setTermRole(dffseOutput, SNLDesignModeling::SNLTermRole::DataOutput);
    SNLDesignModeling::setTermRole(dffseEnable, SNLDesignModeling::SNLTermRole::Enable);
    SNLDesignModeling::setTermRole(dffseSet, SNLDesignModeling::SNLTermRole::AsyncSet, SNLDesignModeling::SNLActiveLevel::High);
  }

  void createSequentialWidthPrimitive(
    naja::NL::NLLibrary* rootLibrary,
    const char* prefix,
    size_t width,
    const char* asyncControlName,
    bool enableControl,
    naja::NL::SNLDesignModeling::SNLTermRole asyncControlRole =
      naja::NL::SNLDesignModeling::SNLTermRole::Other,
    naja::NL::SNLDesignModeling::SNLActiveLevel asyncControlLevel =
      naja::NL::SNLDesignModeling::SNLActiveLevel::NA) {
    using namespace naja::NL;
    auto seq = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(width),
      SNLDesign::Type::Primitive,
      NLName(getWidthInternalName(prefix, width)));
    SNLParameter::create(
      seq, NLName("WIDTH"), SNLParameter::Type::Decimal, std::to_string(width));
    addDFFInitParameter(seq, width);
    auto clock = SNLScalarTerm::create(seq, Term0ID, SNLTerm::Direction::Input, NLName("C"));
    auto data = SNLBusTerm::create(
      seq, Term1ID, SNLTerm::Direction::Input, static_cast<NLID::Bit>(width - 1), 0, NLName("D"));
    SNLScalarTerm* enable = nullptr;
    if (enableControl) {
      enable = SNLScalarTerm::create(seq, Term2ID, SNLTerm::Direction::Input, NLName("E"));
    }
    SNLScalarTerm* asyncControl = nullptr;
    if (asyncControlName) {
      asyncControl = SNLScalarTerm::create(
        seq,
        enableControl ? Term3ID : Term2ID,
        SNLTerm::Direction::Input,
        NLName(asyncControlName));
    }
    auto output = SNLBusTerm::create(
      seq,
      enableControl && asyncControlName ? Term4ID :
        (enableControl || asyncControlName ? Term3ID : Term2ID),
      SNLTerm::Direction::Output,
      static_cast<NLID::Bit>(width - 1),
      0,
      NLName("Q"));

    auto dataBits = collectBitTerms(*data);
    auto outputBits = collectBitTerms(*output);
    SNLDesignModeling::BitTerms clockInputs = dataBits;
    if (enable) {
      clockInputs.push_back(enable);
    }
    if (asyncControl) {
      clockInputs.push_back(asyncControl);
    }
    SNLDesignModeling::addClockToOutputsArcs(clock, outputBits);
    SNLDesignModeling::addInputsToClockArcs(clockInputs, clock);
    SNLDesignModeling::setTermRole(clock, SNLDesignModeling::SNLTermRole::Clock);
    for (auto* term : dataBits) {
      SNLDesignModeling::setTermRole(term, SNLDesignModeling::SNLTermRole::DataInput);
    }
    for (auto* term : outputBits) {
      SNLDesignModeling::setTermRole(term, SNLDesignModeling::SNLTermRole::DataOutput);
    }
    if (enable) {
      SNLDesignModeling::setTermRole(enable, SNLDesignModeling::SNLTermRole::Enable);
    }
    if (asyncControl) {
      SNLDesignModeling::setTermRole(asyncControl, asyncControlRole, asyncControlLevel);
    }
  }

  void createSyncDFFPrimitive(
    naja::NL::NLLibrary* rootLibrary,
    const char* name,
    const char* controlName,
    bool enableControl,
    naja::NL::SNLDesignModeling::SNLTermRole controlRole,
    naja::NL::SNLDesignModeling::SNLActiveLevel controlLevel) {
    using namespace naja::NL;
    auto seq = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(ScalarPrimitiveDesignID),
      SNLDesign::Type::Primitive,
      NLName(name));
    SNLParameter::create(seq, NLName("WIDTH"), SNLParameter::Type::Decimal, "1");
    addDFFInitParameter(seq, 1);
    auto clock = SNLScalarTerm::create(seq, Term0ID, SNLTerm::Direction::Input, NLName("C"));
    auto data = SNLScalarTerm::create(seq, Term1ID, SNLTerm::Direction::Input, NLName("D"));
    SNLScalarTerm* enable = nullptr;
    if (enableControl) {
      enable = SNLScalarTerm::create(seq, Term2ID, SNLTerm::Direction::Input, NLName("E"));
    }
    auto control = SNLScalarTerm::create(
      seq,
      enableControl ? Term3ID : Term2ID,
      SNLTerm::Direction::Input,
      NLName(controlName));
    auto output = SNLScalarTerm::create(
      seq,
      enableControl ? Term4ID : Term3ID,
      SNLTerm::Direction::Output,
      NLName("Q"));
    SNLDesignModeling::BitTerms clockInputs {data, control};
    if (enable) {
      clockInputs.push_back(enable);
    }
    SNLDesignModeling::addClockToOutputsArcs(clock, {output});
    SNLDesignModeling::addInputsToClockArcs(clockInputs, clock);
    SNLDesignModeling::setTermRole(clock, SNLDesignModeling::SNLTermRole::Clock);
    SNLDesignModeling::setTermRole(data, SNLDesignModeling::SNLTermRole::DataInput);
    SNLDesignModeling::setTermRole(output, SNLDesignModeling::SNLTermRole::DataOutput);
    if (enable) {
      SNLDesignModeling::setTermRole(enable, SNLDesignModeling::SNLTermRole::Enable);
    }
    SNLDesignModeling::setTermRole(control, controlRole, controlLevel);
  }

  void createSequentialWidthPrimitive(
    naja::NL::NLLibrary* rootLibrary,
    const char* prefix,
    size_t width,
    const char* latchEnableName) {
    using namespace naja::NL;
    auto seq = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(width),
      SNLDesign::Type::Primitive,
      NLName(getWidthInternalName(prefix, width)));
    SNLParameter::create(
      seq, NLName("WIDTH"), SNLParameter::Type::Decimal, std::to_string(width));
    auto enable = SNLScalarTerm::create(seq, Term0ID, SNLTerm::Direction::Input, NLName(latchEnableName));
    auto data = SNLBusTerm::create(
      seq, Term1ID, SNLTerm::Direction::Input, static_cast<NLID::Bit>(width - 1), 0, NLName("D"));
    auto output = SNLBusTerm::create(
      seq, Term2ID, SNLTerm::Direction::Output, static_cast<NLID::Bit>(width - 1), 0, NLName("Q"));
    auto dataBits = collectBitTerms(*data);
    auto outputBits = collectBitTerms(*output);
    SNLDesignModeling::addClockToOutputsArcs(enable, outputBits);
    SNLDesignModeling::addInputsToClockArcs(dataBits, enable);
    SNLDesignModeling::setTermRole(enable, SNLDesignModeling::SNLTermRole::Enable);
    for (auto* term : dataBits) {
      SNLDesignModeling::setTermRole(term, SNLDesignModeling::SNLTermRole::DataInput);
    }
    for (auto* term : outputBits) {
      SNLDesignModeling::setTermRole(term, SNLDesignModeling::SNLTermRole::DataOutput);
    }
  }

  void createMux2Primitive(naja::NL::NLLibrary* rootLibrary, size_t width) {
    using namespace naja::NL;
    auto mux2 = SNLDesign::create(
      rootLibrary,
      NLID::DesignID(width),
      SNLDesign::Type::Primitive,
      NLName(getMux2InternalName(width)));
    SNLParameter::create(
      mux2, NLName("WIDTH"), SNLParameter::Type::Decimal, std::to_string(width));
    auto inA = SNLBusTerm::create(
      mux2, Term0ID, SNLTerm::Direction::Input, static_cast<NLID::Bit>(width - 1), 0, NLName("A"));
    auto inB = SNLBusTerm::create(
      mux2, Term1ID, SNLTerm::Direction::Input, static_cast<NLID::Bit>(width - 1), 0, NLName("B"));
    auto sel = SNLScalarTerm::create(mux2, Term2ID, SNLTerm::Direction::Input, NLName("S"));
    auto out = SNLBusTerm::create(
      mux2, Term3ID, SNLTerm::Direction::Output, static_cast<NLID::Bit>(width - 1), 0, NLName("Y"));

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

  void createTableSelectPrimitive(
    naja::NL::NLLibrary* tableSelectLibrary,
    const naja::NL::NLDB0::TableSelectSignature& signature) {
    using namespace naja::NL;
    auto tableSelect = SNLDesign::create(
      tableSelectLibrary,
      SNLDesign::Type::Primitive,
      NLName(getTableSelectInternalName(signature)));
    SNLParameter::create(
      tableSelect, NLName("WIDTH"), SNLParameter::Type::Decimal, std::to_string(signature.width));
    SNLParameter::create(
      tableSelect, NLName("DEPTH"), SNLParameter::Type::Decimal, std::to_string(signature.depth));
    SNLParameter::create(
      tableSelect, NLName("ABITS"), SNLParameter::Type::Decimal, std::to_string(signature.abits));

    auto* data = SNLBusTerm::create(
      tableSelect,
      Term0ID,
      SNLTerm::Direction::Input,
      static_cast<NLID::Bit>(signature.width * signature.depth - 1),
      0,
      NLName("DATA"));
    auto* addr = SNLBusTerm::create(
      tableSelect,
      Term1ID,
      SNLTerm::Direction::Input,
      static_cast<NLID::Bit>(signature.abits - 1),
      0,
      NLName("ADDR"));
    auto* out = SNLBusTerm::create(
      tableSelect,
      Term2ID,
      SNLTerm::Direction::Output,
      static_cast<NLID::Bit>(signature.width - 1),
      0,
      NLName("Y"));

    auto addrBits = collectBitTerms(*addr);
    for (size_t bit = 0; bit < signature.width; ++bit) {
      SNLDesignModeling::BitTerms inputs = addrBits;
      for (size_t row = 0; row < signature.depth; ++row) {
        inputs.push_back(data->getBit(static_cast<NLID::Bit>(row * signature.width + bit)));
      }
      SNLDesignModeling::addCombinatorialArcs(
        inputs,
        {out->getBit(static_cast<NLID::Bit>(bit))});
    }
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
      throw naja::NL::NLException("NLDB0::getMemorySignature: null instance"); // LCOV_EXCL_LINE
    }
    if (auto* instParameter = instance->getInstParameter(naja::NL::NLName(name))) {
      return static_cast<size_t>(std::stoull(instParameter->getValue()));
    }
    return getMemoryDecimalParameter(instance->getModel(), name);
  }

  size_t getDivModDecimalParameter(const naja::NL::SNLDesign* design, const char* name) {
    auto* parameter = design ? design->getParameter(naja::NL::NLName(name)) : nullptr;
    if (!parameter) {
      throw naja::NL::NLException(
          std::string("NLDB0 divmod primitive is missing parameter ") + name);
    }
    return static_cast<size_t>(std::stoull(parameter->getValue()));
  }

  size_t getDivModDecimalParameter(const naja::NL::SNLInstance* instance, const char* name) {
    if (!instance) {
      throw naja::NL::NLException("NLDB0::getDivModSignature: null instance"); // LCOV_EXCL_LINE
    }
    if (auto* instParameter = instance->getInstParameter(naja::NL::NLName(name))) {
      return static_cast<size_t>(std::stoull(instParameter->getValue()));
    }
    return getDivModDecimalParameter(instance->getModel(), name);
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

  naja::NL::NLLibrary* getMemoryLibrary() {
    auto* rootLibrary = naja::NL::NLDB0::getDB0RootLibrary();
    if (!rootLibrary) {
      return nullptr;
    }
    return rootLibrary->getLibrary(naja::NL::NLName(MemoryLibraryName));
  }

  naja::NL::NLLibrary* getOrCreateMemoryLibrary() {
    if (auto* existing = getMemoryLibrary()) {
      return existing;
    }
    auto* rootLibrary = naja::NL::NLDB0::getDB0RootLibrary();
    if (!rootLibrary) {
      return nullptr;
    }
    return naja::NL::NLLibrary::create(
        rootLibrary,
        MemoryLibraryID,
        naja::NL::NLLibrary::Type::Primitives,
        naja::NL::NLName(MemoryLibraryName));
  }

  naja::NL::NLLibrary* getTableSelectLibrary() {
    return getPrimitiveLibrary(TableSelectLibraryID);
  }

  naja::NL::NLLibrary* getOrCreateTableSelectLibrary() {
    if (auto* existing = getTableSelectLibrary()) {
      return existing;
    }
    return getOrCreatePrimitiveLibrary(TableSelectLibraryID, TableSelectName);
  }

  size_t getDecimalParameter(
      const naja::NL::NLDB0::PrimitiveParameters& parameters, const char* name) {
    auto it = parameters.find(name);
    if (it == parameters.end()) {
      throw naja::NL::NLException(
          std::string("NLDB0::getOrCreatePrimitive: missing parameter ") + name);
    }
    return static_cast<size_t>(std::stoull(it->second));
  }

  naja::NL::NLDB0::MemorySignature memorySignatureFromParameters(
      const naja::NL::NLDB0::PrimitiveParameters& parameters) {
    naja::NL::NLDB0::MemorySignature signature;
    signature.width = getDecimalParameter(parameters, "WIDTH");
    signature.depth = getDecimalParameter(parameters, "DEPTH");
    signature.abits = getDecimalParameter(parameters, "ABITS");
    signature.readPorts = getDecimalParameter(parameters, "RD_PORTS");
    signature.writePorts = getDecimalParameter(parameters, "WR_PORTS");
    signature.resetMode = getMemoryResetMode(
        getDecimalParameter(parameters, "RST_ENABLE"),
        getDecimalParameter(parameters, "RST_ASYNC"),
        getDecimalParameter(parameters, "RST_ACTIVE_LOW"));
    return signature;
  }

  naja::NL::NLDB0::TableSelectSignature tableSelectSignatureFromParameters(
      const naja::NL::NLDB0::PrimitiveParameters& parameters) {
    naja::NL::NLDB0::TableSelectSignature signature;
    signature.width = getDecimalParameter(parameters, "WIDTH");
    signature.depth = getDecimalParameter(parameters, "DEPTH");
    signature.abits = getDecimalParameter(parameters, "ABITS");
    return signature;
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

std::string NLDB0::formatDFFInitValue(
  size_t width,
  std::string_view msbToLsbDigits) {
  if (width == 0 || msbToLsbDigits.size() != width) {
    throw NLException("NLDB0::formatDFFInitValue: invalid DFF INIT width");
  }
  std::ostringstream literal;
  literal << width << "'b";
  for (char digit : msbToLsbDigits) {
    const char normalized = static_cast<char>(
      std::tolower(static_cast<unsigned char>(digit)));
    if (normalized != '0' && normalized != '1' &&
        normalized != 'x' && normalized != 'z') {
      throw NLException("NLDB0::formatDFFInitValue: invalid DFF INIT digit");
    }
    literal << normalized;
  }
  return literal.str();
}

std::string NLDB0::getUndefinedDFFInitValue(size_t width) {
  return formatDFFInitValue(width, std::string(width, 'x'));
}

NLDB* NLDB0::create(NLUniverse* universe) {
  NLDB* db = NLDB::create(universe);
  assert(db->getID() == 0);

  auto rootLibrary =
    NLLibrary::create(db, RootLibraryID, NLLibrary::Type::Primitives, NLName(RootLibraryName));

  auto assignLibrary =
    NLLibrary::create(rootLibrary, AssignLibraryID, NLLibrary::Type::Primitives, NLName(AssignName));
  auto mux2Library =
    NLLibrary::create(rootLibrary, Mux2LibraryID, NLLibrary::Type::Primitives, NLName(Mux2Name));
  auto dffLibrary =
    NLLibrary::create(rootLibrary, DFFLibraryID, NLLibrary::Type::Primitives, NLName(DFFName));
  auto dlatchLibrary =
    NLLibrary::create(rootLibrary, DLatchLibraryID, NLLibrary::Type::Primitives, NLName(DLatchName));
  auto dffnLibrary =
    NLLibrary::create(rootLibrary, DFFNLibraryID, NLLibrary::Type::Primitives, NLName(DFFNName));
  auto dffrnLibrary =
    NLLibrary::create(rootLibrary, DFFRNLibraryID, NLLibrary::Type::Primitives, NLName(DFFRNName));
  auto dffrLibrary =
    NLLibrary::create(rootLibrary, DFFRLibraryID, NLLibrary::Type::Primitives, NLName(DFFRName));
  auto dffsLibrary =
    NLLibrary::create(rootLibrary, DFFSLibraryID, NLLibrary::Type::Primitives, NLName(DFFSName));
  auto dffeLibrary =
    NLLibrary::create(rootLibrary, DFFELibraryID, NLLibrary::Type::Primitives, NLName(DFFEName));
  auto dffreLibrary =
    NLLibrary::create(rootLibrary, DFFRELibraryID, NLLibrary::Type::Primitives, NLName(DFFREName));
  auto dffseLibrary =
    NLLibrary::create(rootLibrary, DFFSELibraryID, NLLibrary::Type::Primitives, NLName(DFFSEName));
  auto dffsrLibrary =
    NLLibrary::create(rootLibrary, DFFSRLibraryID, NLLibrary::Type::Primitives, NLName(DFFSRName));
  auto dffsrnLibrary =
    NLLibrary::create(rootLibrary, DFFSRNLibraryID, NLLibrary::Type::Primitives, NLName(DFFSRNName));
  auto dffssLibrary =
    NLLibrary::create(rootLibrary, DFFSSLibraryID, NLLibrary::Type::Primitives, NLName(DFFSSName));
  auto dffssnLibrary =
    NLLibrary::create(rootLibrary, DFFSSNLibraryID, NLLibrary::Type::Primitives, NLName(DFFSSNName));
  auto dffsreLibrary =
    NLLibrary::create(rootLibrary, DFFSRELibraryID, NLLibrary::Type::Primitives, NLName(DFFSREName));
  auto dffsrneLibrary =
    NLLibrary::create(rootLibrary, DFFSRNELibraryID, NLLibrary::Type::Primitives, NLName(DFFSRNEName));
  auto dffsseLibrary =
    NLLibrary::create(rootLibrary, DFFSSELibraryID, NLLibrary::Type::Primitives, NLName(DFFSSEName));
  auto dffssneLibrary =
    NLLibrary::create(rootLibrary, DFFSSNELibraryID, NLLibrary::Type::Primitives, NLName(DFFSSNEName));
  auto divModLibrary =
    NLLibrary::create(rootLibrary, DivModLibraryID, NLLibrary::Type::Primitives, NLName(DivModLibraryName));
  NLLibrary::create(
    rootLibrary, TableSelectLibraryID, NLLibrary::Type::Primitives, NLName(TableSelectName));
  NLLibrary::create(
    divModLibrary, DivModUnsignedLibraryID, NLLibrary::Type::Primitives, NLName(DivModUnsignedLibraryName));
  NLLibrary::create(
    divModLibrary, DivModSignedLibraryID, NLLibrary::Type::Primitives, NLName(DivModSignedLibraryName));

  createAssignPrimitive(assignLibrary);
  createFAPrimitive(rootLibrary);
  createMux2Primitive(mux2Library, 1);
  createDFFPrimitive(dffLibrary);
  createDLatchPrimitive(dlatchLibrary);
  createDFFNPrimitive(dffnLibrary);
  createDFFRNPrimitive(dffrnLibrary);
  createDFFRPrimitive(dffrLibrary);
  createDFFSPrimitive(dffsLibrary);
  createDFFEPrimitive(dffeLibrary);
  createDFFREPrimitive(dffreLibrary);
  createDFFSEPrimitive(dffseLibrary);
  createSyncDFFPrimitive(
    dffsrLibrary, DFFSRName, "R", false,
    SNLDesignModeling::SNLTermRole::SyncReset,
    SNLDesignModeling::SNLActiveLevel::High);
  createSyncDFFPrimitive(
    dffsrnLibrary, DFFSRNName, "RN", false,
    SNLDesignModeling::SNLTermRole::SyncReset,
    SNLDesignModeling::SNLActiveLevel::Low);
  createSyncDFFPrimitive(
    dffssLibrary, DFFSSName, "S", false,
    SNLDesignModeling::SNLTermRole::SyncSet,
    SNLDesignModeling::SNLActiveLevel::High);
  createSyncDFFPrimitive(
    dffssnLibrary, DFFSSNName, "SN", false,
    SNLDesignModeling::SNLTermRole::SyncSet,
    SNLDesignModeling::SNLActiveLevel::Low);
  createSyncDFFPrimitive(
    dffsreLibrary, DFFSREName, "R", true,
    SNLDesignModeling::SNLTermRole::SyncReset,
    SNLDesignModeling::SNLActiveLevel::High);
  createSyncDFFPrimitive(
    dffsrneLibrary, DFFSRNEName, "RN", true,
    SNLDesignModeling::SNLTermRole::SyncReset,
    SNLDesignModeling::SNLActiveLevel::Low);
  createSyncDFFPrimitive(
    dffsseLibrary, DFFSSEName, "S", true,
    SNLDesignModeling::SNLTermRole::SyncSet,
    SNLDesignModeling::SNLActiveLevel::High);
  createSyncDFFPrimitive(
    dffssneLibrary, DFFSSNEName, "SN", true,
    SNLDesignModeling::SNLTermRole::SyncSet,
    SNLDesignModeling::SNLActiveLevel::Low);

  return db;
}

NLDB* NLDB0::getDB0() {
  return NLUniverse::getDB0();
}

bool NLDB0::isDB0(const NLDB* db) {
  return db and db == getDB0();
}

NLLibrary* NLDB0::getDB0RootLibrary() {
  return getRootLibraryByID();
}

bool NLDB0::isDB0Library(const NLLibrary* library) {
  auto topLibrary = getDB0RootLibrary();
  if (!library || !topLibrary) {
    return false;
  }
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
  return design && design->isPrimitive() && design->getLibrary() == getMemoryLibrary();
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

bool NLDB0::isTableSelect(const SNLDesign* design) {
  return design && design->isPrimitive() && design->getLibrary() == getTableSelectLibrary() &&
         !design->isUnnamed() &&
         design->getName().getString().rfind(TableSelectPrefix, 0) == 0;
}

NLDB0::TableSelectSignature NLDB0::getTableSelectSignature(const SNLDesign* design) {
  if (!isTableSelect(design)) {
    throw NLException("NLDB0::getTableSelectSignature: design is not a table select primitive");
  }
  TableSelectSignature signature;
  signature.width = getMemoryDecimalParameter(design, "WIDTH");
  signature.depth = getMemoryDecimalParameter(design, "DEPTH");
  signature.abits = getMemoryDecimalParameter(design, "ABITS");
  return signature;
}

NLDB0::TableSelectSignature NLDB0::getTableSelectSignature(const SNLInstance* instance) {
  if (!instance || !isTableSelect(instance->getModel())) {
    throw NLException("NLDB0::getTableSelectSignature: instance is not a table select primitive");
  }
  auto signature = getTableSelectSignature(instance->getModel());
  if (instance->getInstParameter(NLName("WIDTH"))) {
    signature.width = getMemoryDecimalParameter(instance, "WIDTH");
  }
  if (instance->getInstParameter(NLName("DEPTH"))) {
    signature.depth = getMemoryDecimalParameter(instance, "DEPTH");
  }
  if (instance->getInstParameter(NLName("ABITS"))) {
    signature.abits = getMemoryDecimalParameter(instance, "ABITS");
  }
  return signature;
}

SNLBusTerm* NLDB0::getTableSelectData(const SNLDesign* design) {
  if (!isTableSelect(design)) {
    throw NLException("NLDB0::getTableSelectData: design is not a table select primitive");
  }
  return design->getBusTerm(NLName("DATA"));
}

SNLBusTerm* NLDB0::getTableSelectAddress(const SNLDesign* design) {
  if (!isTableSelect(design)) {
    throw NLException("NLDB0::getTableSelectAddress: design is not a table select primitive");
  }
  return design->getBusTerm(NLName("ADDR"));
}

SNLBusTerm* NLDB0::getTableSelectOutput(const SNLDesign* design) {
  if (!isTableSelect(design)) {
    throw NLException("NLDB0::getTableSelectOutput: design is not a table select primitive");
  }
  return design->getBusTerm(NLName("Y"));
}

SNLTruthTable NLDB0::getTableSelectTruthTable(const SNLDesign* design, size_t flatTermID) {
  if (!isTableSelect(design)) {
    throw NLException("NLDB0::getTableSelectTruthTable: design is not a table select primitive");
  }
  auto signature = getTableSelectSignature(design);
  if (signature.abits > std::numeric_limits<uint32_t>::max() ||
      signature.depth > std::numeric_limits<uint32_t>::max()) {
    throw NLException("NLDB0::getTableSelectTruthTable: signature does not fit truth table metadata");
  }

  const SNLBitTerm* outputTerm = nullptr;
  for (const auto* term: design->getBitTerms()) {
    if (term->getOrderID() == flatTermID) {
      outputTerm = term;
      break;
    }
  }
  if (!outputTerm ||
      outputTerm->getDirection() != SNLTerm::Direction::Output ||
      outputTerm->getID() != Term2ID) {
    std::ostringstream reason;
    reason << "Term ID " << flatTermID
           << " is not an output in table select design <"
           << design->getName().getString() << ">";
    throw NLException(reason.str());
  }

  const size_t outputBit = static_cast<size_t>(outputTerm->getBit());
  if (outputBit >= signature.width) {
    throw NLException("NLDB0::getTableSelectTruthTable: output bit is out of range");
  }

  auto* data = getTableSelectData(design);
  auto* addr = getTableSelectAddress(design);
  std::vector<size_t> deps;
  deps.reserve(signature.abits + signature.depth);
  for (auto* addrBit: addr->getBits()) {
    deps.push_back(addrBit->getOrderID());
  }
  for (size_t row = 0; row < signature.depth; ++row) {
    const auto dataBit = static_cast<NLID::Bit>(row * signature.width + outputBit);
    deps.push_back(data->getBit(dataBit)->getOrderID());
  }

  return SNLTruthTable::TableSelect(
      static_cast<uint32_t>(signature.abits),
      static_cast<uint32_t>(signature.depth),
      NLBitDependencies::encodeBits(deps));
}

bool NLDB0::isDivMod(const SNLDesign* design) {
  if (!design || !design->isPrimitive() || !isDB0Primitive(design) || design->isUnnamed()) {
    return false;
  }
  auto* library = design->getLibrary();
  if (!library || library->isRoot()) {
    return false;
  }
  const auto libraryID = library->getID();
  if (libraryID != DivModUnsignedLibraryID && libraryID != DivModSignedLibraryID) {
    return false;
  }
  return library->getParentLibrary() == getPrimitiveLibrary(DivModLibraryID) &&
         design->getName().getString().rfind(DivModPrefix, 0) == 0;
}

NLDB0::DivModSignature NLDB0::getDivModSignature(const SNLDesign* design) {
  if (!isDivMod(design)) {
    throw NLException("NLDB0::getDivModSignature: design is not a divmod primitive");
  }
  DivModSignature signature;
  signature.width = getDivModDecimalParameter(design, "WIDTH");
  signature.isSigned = getDivModDecimalParameter(design, "SIGNED") != 0;
  return signature;
}

NLDB0::DivModSignature NLDB0::getDivModSignature(const SNLInstance* instance) {
  if (!instance || !isDivMod(instance->getModel())) {
    throw NLException("NLDB0::getDivModSignature: instance is not a divmod primitive");
  }
  DivModSignature signature;
  signature.width = getDivModDecimalParameter(instance, "WIDTH");
  signature.isSigned = getDivModDecimalParameter(instance, "SIGNED") != 0;
  return signature;
}

SNLBusTerm* NLDB0::getDivModDividend(const SNLDesign* design) {
  if (!isDivMod(design)) {
    throw NLException("NLDB0::getDivModDividend: design is not a divmod primitive");
  }
  return design->getBusTerm(Term0ID);
}

SNLBusTerm* NLDB0::getDivModDivisor(const SNLDesign* design) {
  if (!isDivMod(design)) {
    throw NLException("NLDB0::getDivModDivisor: design is not a divmod primitive");
  }
  return design->getBusTerm(Term1ID);
}

SNLBusTerm* NLDB0::getDivModQuotient(const SNLDesign* design) {
  if (!isDivMod(design)) {
    throw NLException("NLDB0::getDivModQuotient: design is not a divmod primitive");
  }
  return design->getBusTerm(Term2ID);
}

SNLBusTerm* NLDB0::getDivModRemainder(const SNLDesign* design) {
  if (!isDivMod(design)) {
    throw NLException("NLDB0::getDivModRemainder: design is not a divmod primitive");
  }
  return design->getBusTerm(Term3ID);
}

SNLTruthTable NLDB0::getPrimitiveTruthTable(const SNLDesign* design) {
  if (isMemory(design)) {
    throw NLException("NLDB0::getPrimitiveTruthTable: memory primitive has no truth table");
  }
  if (isDivMod(design)) {
    throw NLException("NLDB0::getPrimitiveTruthTable: divmod primitive has no truth table");
  }
  if (isTableSelect(design)) {
    throw NLException("NLDB0::getPrimitiveTruthTable: table select primitive has no truth table");
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

SNLDesign* NLDB0::getOrCreateDivMod(const DivModSignature& signature) {
  auto* divModLibrary = getOrCreateDivModSignednessLibrary(signature.isSigned);
  if (!divModLibrary) {
    return nullptr;
  }
  if (signature.width == 0) {
    throw NLException("NLDB0::getOrCreateDivMod: invalid divmod signature");
  }
  if (signature.width > std::numeric_limits<NLID::DesignID>::max()) {
    throw NLException("NLDB0::getOrCreateDivMod: width does not fit a design ID");
  }
  const auto id = NLID::DesignID(signature.width);
  if (auto* existing = divModLibrary->getSNLDesign(id)) {
    return existing;
  }
  createDivModPrimitive(divModLibrary, signature);
  return divModLibrary->getSNLDesign(id);
}

SNLDesign* NLDB0::getOrCreateMemory(const MemorySignature& signature) {
  auto* memoryLibrary = getOrCreateMemoryLibrary();
  if (!memoryLibrary) {
    return nullptr;
  }
  if (signature.width == 0 || signature.depth == 0 || signature.abits == 0 ||
      signature.readPorts == 0 || signature.writePorts == 0) {
    throw NLException("NLDB0::getOrCreateMemory: invalid memory signature");
  }
  const auto name = NLName(getMemoryInternalName(signature));
  if (auto* existing = memoryLibrary->getSNLDesign(name)) {
    return existing;
  }
  createMemoryPrimitive(memoryLibrary, signature);
  return memoryLibrary->getSNLDesign(name);
}

SNLDesign* NLDB0::getOrCreateTableSelect(const TableSelectSignature& signature) {
  auto* tableSelectLibrary = getOrCreateTableSelectLibrary();
  if (!tableSelectLibrary) {
    return nullptr;
  }
  if (signature.width == 0 || signature.depth == 0 || signature.abits == 0) {
    throw NLException("NLDB0::getOrCreateTableSelect: invalid table select signature");
  }
  if (signature.depth > std::numeric_limits<size_t>::max() / signature.width) {
    throw NLException("NLDB0::getOrCreateTableSelect: data width overflow");
  }
  const auto dataWidth = signature.width * signature.depth;
  const auto maxBit = static_cast<size_t>(std::numeric_limits<NLID::Bit>::max());
  if (dataWidth - 1 > maxBit ||
      signature.width - 1 > maxBit ||
      signature.abits - 1 > maxBit) {
    throw NLException("NLDB0::getOrCreateTableSelect: bus bound does not fit a bit ID");
  }
  const auto name = NLName(getTableSelectInternalName(signature));
  if (auto* existing = tableSelectLibrary->getSNLDesign(name)) {
    return existing;
  }
  createTableSelectPrimitive(tableSelectLibrary, signature);
  return tableSelectLibrary->getSNLDesign(name);
}

SNLDesign* NLDB0::getAssign() {
  auto* assignLibrary = getPrimitiveLibrary(AssignLibraryID);
  if (assignLibrary) {
    return assignLibrary->getSNLDesign(NLID::DesignID(ScalarPrimitiveDesignID));
  }
  return nullptr;
}

bool NLDB0::isAssign(const SNLDesign* design) {
  return design and design == getAssign();
}

SNLScalarTerm* NLDB0::getAssignInput() {
  auto assign = getAssign();
  if (assign) {
    return assign->getScalarTerm(Term0ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getAssignOutput() {
  auto assign = getAssign();
  if (assign) {
    return assign->getScalarTerm(Term1ID);
  }
  return nullptr;
}

SNLDesign* NLDB0::getFA() {
  auto primitives = getDB0RootLibrary();
  if (primitives) {
    return primitives->getSNLDesign(NLID::DesignID(FADesignID));
  }
  return nullptr;
}

bool NLDB0::isFA(const SNLDesign* design) {
  return design and design == getFA();
}

SNLScalarTerm* NLDB0::getFAInputA() {
  auto fa = getFA();
  if (fa) { return fa->getScalarTerm(Term0ID); }
  return nullptr;
}

SNLScalarTerm* NLDB0::getFAInputB() {
  auto fa = getFA();
  if (fa) { return fa->getScalarTerm(Term1ID); }
  return nullptr;
}

SNLScalarTerm* NLDB0::getFAInputCI() {
  auto fa = getFA();
  if (fa) { return fa->getScalarTerm(Term2ID); }
  return nullptr;
}

SNLScalarTerm* NLDB0::getFAOutputS() {
  auto fa = getFA();
  if (fa) { return fa->getScalarTerm(Term3ID); }
  return nullptr;
}

SNLScalarTerm* NLDB0::getFAOutputCO() {
  auto fa = getFA();
  if (fa) { return fa->getScalarTerm(Term4ID); }
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
  auto* mux2Library = getPrimitiveLibrary(Mux2LibraryID);
  if (mux2Library) {
    return mux2Library->getSNLDesign(NLID::DesignID(ScalarPrimitiveDesignID));
  }
  return nullptr;
}

SNLDesign* NLDB0::getOrCreateMux2(size_t width) {
  if (width == 0) {
    throw NLException("NLDB0::getOrCreateMux2: invalid width");
  }
  return getOrCreateWidthPrimitive(
    Mux2LibraryID,
    Mux2Name,
    Mux2ScalarName,
    Mux2Prefix,
    width,
    [](NLLibrary* primitives, size_t width) {
      createMux2Primitive(primitives, width);
    });
}

bool NLDB0::isMux2(const SNLDesign* design) {
  return isWidthPrimitive(design, Mux2LibraryID, Mux2ScalarName, Mux2Prefix);
}

SNLBusTerm* NLDB0::getMux2InputA(const SNLDesign* mux2) {
  if (mux2) {
    return mux2->getBusTerm(Term0ID);
  }
  return nullptr;
}

SNLBusTerm* NLDB0::getMux2InputA() {
  return getMux2InputA(getMux2());
}

SNLBusTerm* NLDB0::getMux2InputB(const SNLDesign* mux2) {
  if (mux2) {
    return mux2->getBusTerm(Term1ID);
  }
  return nullptr;
}

SNLBusTerm* NLDB0::getMux2InputB() {
  return getMux2InputB(getMux2());
}

SNLScalarTerm* NLDB0::getMux2Select(const SNLDesign* mux2) {
  if (mux2) {
    return mux2->getScalarTerm(Term2ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getMux2Select() {
  return getMux2Select(getMux2());
}

SNLBusTerm* NLDB0::getMux2Output(const SNLDesign* mux2) {
  if (mux2) {
    return mux2->getBusTerm(Term3ID);
  }
  return nullptr;
}

SNLBusTerm* NLDB0::getMux2Output() {
  return getMux2Output(getMux2());
}

SNLDesign* NLDB0::getDFF() {
  auto* dffLibrary = getPrimitiveLibrary(DFFLibraryID);
  if (dffLibrary) {
    return dffLibrary->getSNLDesign(NLID::DesignID(ScalarPrimitiveDesignID));
  }
  return nullptr;
}

SNLDesign* NLDB0::getOrCreateDFF(size_t width) {
  return getOrCreateWidthPrimitive(
    DFFLibraryID,
    DFFName,
    DFFName,
    DFFPrefix,
    width,
    [](NLLibrary* primitives, size_t width) {
      createSequentialWidthPrimitive(primitives, DFFPrefix, width, nullptr, false);
    });
}

bool NLDB0::isDFF(const SNLDesign* design) {
  return isWidthPrimitive(design, DFFLibraryID, DFFName, DFFPrefix);
}

SNLScalarTerm* NLDB0::getDFFClock() {
  auto dff = getDFF();
  if (dff) {
    return dff->getScalarTerm(Term0ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFData() {
  auto dff = getDFF();
  if (dff) {
    return dff->getScalarTerm(Term1ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFOutput() {
  auto dff = getDFF();
  if (dff) {
    return dff->getScalarTerm(Term2ID);
  }
  return nullptr;
}

SNLDesign* NLDB0::getDLatch() {
  auto* dlatchLibrary = getPrimitiveLibrary(DLatchLibraryID);
  if (dlatchLibrary) {
    return dlatchLibrary->getSNLDesign(NLID::DesignID(ScalarPrimitiveDesignID));
  }
  return nullptr;
}

SNLDesign* NLDB0::getOrCreateDLatch(size_t width) {
  return getOrCreateWidthPrimitive(
    DLatchLibraryID,
    DLatchName,
    DLatchName,
    DLatchPrefix,
    width,
    [](NLLibrary* primitives, size_t width) {
      createSequentialWidthPrimitive(primitives, DLatchPrefix, width, "E");
    });
}

bool NLDB0::isDLatch(const SNLDesign* design) {
  return isWidthPrimitive(design, DLatchLibraryID, DLatchName, DLatchPrefix);
}

SNLScalarTerm* NLDB0::getDLatchEnable() {
  auto dlatch = getDLatch();
  if (dlatch) {
    return dlatch->getScalarTerm(Term0ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDLatchData() {
  auto dlatch = getDLatch();
  if (dlatch) {
    return dlatch->getScalarTerm(Term1ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDLatchOutput() {
  auto dlatch = getDLatch();
  if (dlatch) {
    return dlatch->getScalarTerm(Term2ID);
  }
  return nullptr;
}

SNLDesign* NLDB0::getDFFN() {
  auto* dffnLibrary = getPrimitiveLibrary(DFFNLibraryID);
  if (dffnLibrary) {
    return dffnLibrary->getSNLDesign(NLID::DesignID(ScalarPrimitiveDesignID));
  }
  return nullptr;
}

SNLDesign* NLDB0::getOrCreateDFFN(size_t width) {
  return getOrCreateWidthPrimitive(
    DFFNLibraryID,
    DFFNName,
    DFFNName,
    DFFNPrefix,
    width,
    [](NLLibrary* primitives, size_t width) {
      createSequentialWidthPrimitive(primitives, DFFNPrefix, width, nullptr, false);
    });
}

bool NLDB0::isDFFN(const SNLDesign* design) {
  return isWidthPrimitive(design, DFFNLibraryID, DFFNName, DFFNPrefix);
}

SNLScalarTerm* NLDB0::getDFFNClock() {
  auto dffn = getDFFN();
  if (dffn) {
    return dffn->getScalarTerm(Term0ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFNData() {
  auto dffn = getDFFN();
  if (dffn) {
    return dffn->getScalarTerm(Term1ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFNOutput() {
  auto dffn = getDFFN();
  if (dffn) {
    return dffn->getScalarTerm(Term2ID);
  }
  return nullptr;
}

SNLDesign* NLDB0::getDFFRN() {
  auto* dffrnLibrary = getPrimitiveLibrary(DFFRNLibraryID);
  if (dffrnLibrary) {
    return dffrnLibrary->getSNLDesign(NLID::DesignID(ScalarPrimitiveDesignID));
  }
  return nullptr;
}

SNLDesign* NLDB0::getOrCreateDFFRN(size_t width) {
  return getOrCreateWidthPrimitive(
    DFFRNLibraryID,
    DFFRNName,
    DFFRNName,
    DFFRNPrefix,
    width,
    [](NLLibrary* primitives, size_t width) {
      createSequentialWidthPrimitive(
        primitives, DFFRNPrefix, width, "RN", false,
        SNLDesignModeling::SNLTermRole::AsyncReset,
        SNLDesignModeling::SNLActiveLevel::Low);
    });
}

bool NLDB0::isDFFRN(const SNLDesign* design) {
  return isWidthPrimitive(design, DFFRNLibraryID, DFFRNName, DFFRNPrefix);
}

SNLScalarTerm* NLDB0::getDFFRNClock() {
  auto dffrn = getDFFRN();
  if (dffrn) {
    return dffrn->getScalarTerm(Term0ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFRNData() {
  auto dffrn = getDFFRN();
  if (dffrn) {
    return dffrn->getScalarTerm(Term1ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFRNResetN() {
  auto dffrn = getDFFRN();
  if (dffrn) {
    return dffrn->getScalarTerm(Term2ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFRNOutput() {
  auto dffrn = getDFFRN();
  if (dffrn) {
    return dffrn->getScalarTerm(Term3ID);
  }
  return nullptr;
}

SNLDesign* NLDB0::getDFFR() {
  auto* dffrLibrary = getPrimitiveLibrary(DFFRLibraryID);
  if (dffrLibrary) {
    return dffrLibrary->getSNLDesign(NLID::DesignID(ScalarPrimitiveDesignID));
  }
  return nullptr;
}

SNLDesign* NLDB0::getOrCreateDFFR(size_t width) {
  return getOrCreateWidthPrimitive(
    DFFRLibraryID,
    DFFRName,
    DFFRName,
    DFFRPrefix,
    width,
    [](NLLibrary* primitives, size_t width) {
      createSequentialWidthPrimitive(
        primitives, DFFRPrefix, width, "R", false,
        SNLDesignModeling::SNLTermRole::AsyncReset,
        SNLDesignModeling::SNLActiveLevel::High);
    });
}

bool NLDB0::isDFFR(const SNLDesign* design) {
  return isWidthPrimitive(design, DFFRLibraryID, DFFRName, DFFRPrefix);
}

SNLScalarTerm* NLDB0::getDFFRClock() {
  auto dffr = getDFFR();
  if (dffr) {
    return dffr->getScalarTerm(Term0ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFRData() {
  auto dffr = getDFFR();
  if (dffr) {
    return dffr->getScalarTerm(Term1ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFRReset() {
  auto dffr = getDFFR();
  if (dffr) {
    return dffr->getScalarTerm(Term2ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFROutput() {
  auto dffr = getDFFR();
  if (dffr) {
    return dffr->getScalarTerm(Term3ID);
  }
  return nullptr;
}

SNLDesign* NLDB0::getDFFS() {
  auto* dffsLibrary = getPrimitiveLibrary(DFFSLibraryID);
  if (dffsLibrary) {
    return dffsLibrary->getSNLDesign(NLID::DesignID(ScalarPrimitiveDesignID));
  }
  return nullptr;
}

SNLDesign* NLDB0::getOrCreateDFFS(size_t width) {
  return getOrCreateWidthPrimitive(
    DFFSLibraryID,
    DFFSName,
    DFFSName,
    DFFSPrefix,
    width,
    [](NLLibrary* primitives, size_t width) {
      createSequentialWidthPrimitive(
        primitives, DFFSPrefix, width, "S", false,
        SNLDesignModeling::SNLTermRole::AsyncSet,
        SNLDesignModeling::SNLActiveLevel::High);
    });
}

bool NLDB0::isDFFS(const SNLDesign* design) {
  return isWidthPrimitive(design, DFFSLibraryID, DFFSName, DFFSPrefix);
}

SNLScalarTerm* NLDB0::getDFFSClock() {
  auto dffs = getDFFS();
  if (dffs) {
    return dffs->getScalarTerm(Term0ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFSData() {
  auto dffs = getDFFS();
  if (dffs) {
    return dffs->getScalarTerm(Term1ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFSSet() {
  auto dffs = getDFFS();
  if (dffs) {
    return dffs->getScalarTerm(Term2ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFSOutput() {
  auto dffs = getDFFS();
  if (dffs) {
    return dffs->getScalarTerm(Term3ID);
  }
  return nullptr;
}

SNLDesign* NLDB0::getDFFE() {
  auto* dffeLibrary = getPrimitiveLibrary(DFFELibraryID);
  if (dffeLibrary) {
    return dffeLibrary->getSNLDesign(NLID::DesignID(ScalarPrimitiveDesignID));
  }
  return nullptr;
}

SNLDesign* NLDB0::getOrCreateDFFE(size_t width) {
  return getOrCreateWidthPrimitive(
    DFFELibraryID,
    DFFEName,
    DFFEName,
    DFFEPrefix,
    width,
    [](NLLibrary* primitives, size_t width) {
      createSequentialWidthPrimitive(primitives, DFFEPrefix, width, nullptr, true);
    });
}

bool NLDB0::isDFFE(const SNLDesign* design) {
  return isWidthPrimitive(design, DFFELibraryID, DFFEName, DFFEPrefix);
}

SNLScalarTerm* NLDB0::getDFFEClock() {
  auto dffe = getDFFE();
  if (dffe) {
    return dffe->getScalarTerm(Term0ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFEData() {
  auto dffe = getDFFE();
  if (dffe) {
    return dffe->getScalarTerm(Term1ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFEEnable() {
  auto dffe = getDFFE();
  if (dffe) {
    return dffe->getScalarTerm(Term2ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFEOutput() {
  auto dffe = getDFFE();
  if (dffe) {
    return dffe->getScalarTerm(Term3ID);
  }
  return nullptr;
}

SNLDesign* NLDB0::getDFFRE() {
  auto* dffreLibrary = getPrimitiveLibrary(DFFRELibraryID);
  if (dffreLibrary) {
    return dffreLibrary->getSNLDesign(NLID::DesignID(ScalarPrimitiveDesignID));
  }
  return nullptr;
}

SNLDesign* NLDB0::getOrCreateDFFRE(size_t width) {
  return getOrCreateWidthPrimitive(
    DFFRELibraryID,
    DFFREName,
    DFFREName,
    DFFREPrefix,
    width,
    [](NLLibrary* primitives, size_t width) {
      createSequentialWidthPrimitive(
        primitives, DFFREPrefix, width, "R", true,
        SNLDesignModeling::SNLTermRole::AsyncReset,
        SNLDesignModeling::SNLActiveLevel::High);
    });
}

bool NLDB0::isDFFRE(const SNLDesign* design) {
  return isWidthPrimitive(design, DFFRELibraryID, DFFREName, DFFREPrefix);
}

SNLScalarTerm* NLDB0::getDFFREClock() {
  auto dffre = getDFFRE();
  if (dffre) {
    return dffre->getScalarTerm(Term0ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFREData() {
  auto dffre = getDFFRE();
  if (dffre) {
    return dffre->getScalarTerm(Term1ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFREEnable() {
  auto dffre = getDFFRE();
  if (dffre) {
    return dffre->getScalarTerm(Term2ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFREReset() {
  auto dffre = getDFFRE();
  if (dffre) {
    return dffre->getScalarTerm(Term3ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFREOutput() {
  auto dffre = getDFFRE();
  if (dffre) {
    return dffre->getScalarTerm(Term4ID);
  }
  return nullptr;
}

SNLDesign* NLDB0::getDFFSE() {
  auto* dffseLibrary = getPrimitiveLibrary(DFFSELibraryID);
  if (dffseLibrary) {
    return dffseLibrary->getSNLDesign(NLID::DesignID(ScalarPrimitiveDesignID));
  }
  return nullptr;
}

SNLDesign* NLDB0::getOrCreateDFFSE(size_t width) {
  return getOrCreateWidthPrimitive(
    DFFSELibraryID,
    DFFSEName,
    DFFSEName,
    DFFSEPrefix,
    width,
    [](NLLibrary* primitives, size_t width) {
      createSequentialWidthPrimitive(
        primitives, DFFSEPrefix, width, "S", true,
        SNLDesignModeling::SNLTermRole::AsyncSet,
        SNLDesignModeling::SNLActiveLevel::High);
    });
}

bool NLDB0::isDFFSE(const SNLDesign* design) {
  return isWidthPrimitive(design, DFFSELibraryID, DFFSEName, DFFSEPrefix);
}

SNLScalarTerm* NLDB0::getDFFSEClock() {
  auto dffse = getDFFSE();
  if (dffse) {
    return dffse->getScalarTerm(Term0ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFSEData() {
  auto dffse = getDFFSE();
  if (dffse) {
    return dffse->getScalarTerm(Term1ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFSEEnable() {
  auto dffse = getDFFSE();
  if (dffse) {
    return dffse->getScalarTerm(Term2ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFSESet() {
  auto dffse = getDFFSE();
  if (dffse) {
    return dffse->getScalarTerm(Term3ID);
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getDFFSEOutput() {
  auto dffse = getDFFSE();
  if (dffse) {
    return dffse->getScalarTerm(Term4ID);
  }
  return nullptr;
}

#define DEFINE_SYNC_DFF_FAMILY(API, LIBID, NAME, PREFIX, CONTROL_NAME, CONTROL_ROLE, CONTROL_LEVEL) \
SNLDesign* NLDB0::get##API() {                                                                      \
  auto* lib = getPrimitiveLibrary(LIBID);                                                           \
  if (lib) {                                                                                        \
    return lib->getSNLDesign(NLID::DesignID(ScalarPrimitiveDesignID));                              \
  }                                                                                                 \
  return nullptr;                                                                                   \
}                                                                                                   \
SNLDesign* NLDB0::getOrCreate##API(size_t width) {                                                  \
  return getOrCreateWidthPrimitive(                                                                 \
    LIBID,                                                                                          \
    NAME,                                                                                           \
    NAME,                                                                                           \
    PREFIX,                                                                                         \
    width,                                                                                          \
    [](NLLibrary* primitives, size_t width) {                                                        \
      createSequentialWidthPrimitive(                                                               \
        primitives, PREFIX, width, CONTROL_NAME, false,                                             \
        SNLDesignModeling::SNLTermRole::CONTROL_ROLE,                                               \
        SNLDesignModeling::SNLActiveLevel::CONTROL_LEVEL);                                          \
    });                                                                                             \
}                                                                                                   \
bool NLDB0::is##API(const SNLDesign* design) {                                                      \
  return isWidthPrimitive(design, LIBID, NAME, PREFIX);                                             \
}

DEFINE_SYNC_DFF_FAMILY(DFFSR, DFFSRLibraryID, DFFSRName, DFFSRPrefix, "R", SyncReset, High)
DEFINE_SYNC_DFF_FAMILY(DFFSRN, DFFSRNLibraryID, DFFSRNName, DFFSRNPrefix, "RN", SyncReset, Low)
DEFINE_SYNC_DFF_FAMILY(DFFSS, DFFSSLibraryID, DFFSSName, DFFSSPrefix, "S", SyncSet, High)
DEFINE_SYNC_DFF_FAMILY(DFFSSN, DFFSSNLibraryID, DFFSSNName, DFFSSNPrefix, "SN", SyncSet, Low)
#undef DEFINE_SYNC_DFF_FAMILY

#define DEFINE_SYNC_DFFE_FAMILY(API, LIBID, NAME, PREFIX, CONTROL_NAME, CONTROL_ROLE, CONTROL_LEVEL) \
SNLDesign* NLDB0::get##API() {                                                                       \
  auto* lib = getPrimitiveLibrary(LIBID);                                                            \
  if (lib) {                                                                                         \
    return lib->getSNLDesign(NLID::DesignID(ScalarPrimitiveDesignID));                               \
  }                                                                                                  \
  return nullptr;                                                                                    \
}                                                                                                    \
SNLDesign* NLDB0::getOrCreate##API(size_t width) {                                                   \
  return getOrCreateWidthPrimitive(                                                                  \
    LIBID,                                                                                           \
    NAME,                                                                                            \
    NAME,                                                                                            \
    PREFIX,                                                                                          \
    width,                                                                                           \
    [](NLLibrary* primitives, size_t width) {                                                         \
      createSequentialWidthPrimitive(                                                                \
        primitives, PREFIX, width, CONTROL_NAME, true,                                               \
        SNLDesignModeling::SNLTermRole::CONTROL_ROLE,                                                \
        SNLDesignModeling::SNLActiveLevel::CONTROL_LEVEL);                                           \
    });                                                                                              \
}                                                                                                    \
bool NLDB0::is##API(const SNLDesign* design) {                                                       \
  return isWidthPrimitive(design, LIBID, NAME, PREFIX);                                              \
}

DEFINE_SYNC_DFFE_FAMILY(DFFSRE, DFFSRELibraryID, DFFSREName, DFFSREPrefix, "R", SyncReset, High)
DEFINE_SYNC_DFFE_FAMILY(DFFSRNE, DFFSRNELibraryID, DFFSRNEName, DFFSRNEPrefix, "RN", SyncReset, Low)
DEFINE_SYNC_DFFE_FAMILY(DFFSSE, DFFSSELibraryID, DFFSSEName, DFFSSEPrefix, "S", SyncSet, High)
DEFINE_SYNC_DFFE_FAMILY(DFFSSNE, DFFSSNELibraryID, DFFSSNEName, DFFSSNEPrefix, "SN", SyncSet, Low)
#undef DEFINE_SYNC_DFFE_FAMILY

SNLScalarTerm* NLDB0::getDFFSRClock() { return getDFFSR() ? getDFFSR()->getScalarTerm(Term0ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSRData() { return getDFFSR() ? getDFFSR()->getScalarTerm(Term1ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSRReset() { return getDFFSR() ? getDFFSR()->getScalarTerm(Term2ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSROutput() { return getDFFSR() ? getDFFSR()->getScalarTerm(Term3ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSRNClock() { return getDFFSRN() ? getDFFSRN()->getScalarTerm(Term0ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSRNData() { return getDFFSRN() ? getDFFSRN()->getScalarTerm(Term1ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSRNResetN() { return getDFFSRN() ? getDFFSRN()->getScalarTerm(Term2ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSRNOutput() { return getDFFSRN() ? getDFFSRN()->getScalarTerm(Term3ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSSClock() { return getDFFSS() ? getDFFSS()->getScalarTerm(Term0ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSSData() { return getDFFSS() ? getDFFSS()->getScalarTerm(Term1ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSSSet() { return getDFFSS() ? getDFFSS()->getScalarTerm(Term2ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSSOutput() { return getDFFSS() ? getDFFSS()->getScalarTerm(Term3ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSSNClock() { return getDFFSSN() ? getDFFSSN()->getScalarTerm(Term0ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSSNData() { return getDFFSSN() ? getDFFSSN()->getScalarTerm(Term1ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSSNSetN() { return getDFFSSN() ? getDFFSSN()->getScalarTerm(Term2ID) : nullptr; }
SNLScalarTerm* NLDB0::getDFFSSNOutput() { return getDFFSSN() ? getDFFSSN()->getScalarTerm(Term3ID) : nullptr; }

NLLibrary* NLDB0::getOrCreateGateLibrary(const GateType& type) {
  auto gateLib = getGateLibrary(type);
  if (gateLib) {
    return gateLib;
  }
  auto rootLib = getDB0RootLibrary();
  if (not rootLib) {
    return nullptr;
  }
  auto libraryID = static_cast<NLID::LibraryID>(
    GateLibraryIDBase + static_cast<int>(static_cast<GateType::GateTypeEnum>(type)));
  return NLLibrary::create(rootLib, libraryID, NLLibrary::Type::Primitives, NLName(type.getString()));
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
    //Canonical design ID = fan-out, so the model reference is self-describing.
    gate = SNLDesign::create(
      gateLibrary, NLID::DesignID(nbOutputs), SNLDesign::Type::Primitive, NLName(gateName));
    auto outputs =
      SNLBusTerm::create(gate, SNLTerm::Direction::Output, NLID::Bit(nbOutputs-1), 0);
    auto input = SNLScalarTerm::create(gate, SNLTerm::Direction::Input);
    SNLDesignModeling::addCombinatorialArcs({input}, collectBitTerms(*outputs));
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
    //Canonical design ID = fan-in, so the model reference is self-describing.
    gate = SNLDesign::create(
      gateLibrary, NLID::DesignID(nbInputs), SNLDesign::Type::Primitive, NLName(gateName));
    auto output = SNLScalarTerm::create(gate, SNLTerm::Direction::Output);
    auto inputs =
      SNLBusTerm::create(gate, SNLTerm::Direction::Input, NLID::Bit(nbInputs-1), 0);
    SNLDesignModeling::addCombinatorialArcs(collectBitTerms(*inputs), {output});
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

SNLDesign* NLDB0::getOrCreatePrimitive(
    NLID::LibraryID libraryID,
    NLID::DesignID designID,
    const PrimitiveParameters& parameters) {
  //Single-parameter families: designID carries the width / fan-in / fan-out.
  switch (libraryID) {
    case RootLibraryID:
      return (designID == FADesignID) ? getFA() : nullptr;
    case AssignLibraryID:
      return getAssign();
    case Mux2LibraryID:
      return getOrCreateMux2(designID);
    case DFFLibraryID:
      return getOrCreateDFF(designID);
    case DLatchLibraryID:
      return getOrCreateDLatch(designID);
    case DFFNLibraryID:
      return getOrCreateDFFN(designID);
    case DFFRNLibraryID:
      return getOrCreateDFFRN(designID);
    case DFFRLibraryID:
      return getOrCreateDFFR(designID);
    case DFFSLibraryID:
      return getOrCreateDFFS(designID);
    case DFFELibraryID:
      return getOrCreateDFFE(designID);
    case DFFRELibraryID:
      return getOrCreateDFFRE(designID);
    case DFFSELibraryID:
      return getOrCreateDFFSE(designID);
    case DFFSRLibraryID:
      return getOrCreateDFFSR(designID);
    case DFFSRNLibraryID:
      return getOrCreateDFFSRN(designID);
    case DFFSSLibraryID:
      return getOrCreateDFFSS(designID);
    case DFFSSNLibraryID:
      return getOrCreateDFFSSN(designID);
    case DFFSRELibraryID:
      return getOrCreateDFFSRE(designID);
    case DFFSRNELibraryID:
      return getOrCreateDFFSRNE(designID);
    case DFFSSELibraryID:
      return getOrCreateDFFSSE(designID);
    case DFFSSNELibraryID:
      return getOrCreateDFFSSNE(designID);
    case DivModUnsignedLibraryID:
      return getOrCreateDivMod(DivModSignature{size_t(designID), false});
    case DivModSignedLibraryID:
      return getOrCreateDivMod(DivModSignature{size_t(designID), true});
    //Multi-parameter families: the full signature comes from the instance params.
    case MemoryLibraryID:
      return getOrCreateMemory(memorySignatureFromParameters(parameters));
    case TableSelectLibraryID:
      return getOrCreateTableSelect(tableSelectSignatureFromParameters(parameters));
    default:
      break;
  }
  //Gate libraries: one reserved ID per GateType, designID = fan-in/out.
  if (libraryID >= GateLibraryIDBase &&
      libraryID < GateLibraryIDBase + GateType::Unknown) {
    GateType type(GateType::GateTypeEnum(libraryID - GateLibraryIDBase));
    if (type.isNInput()) {
      return getOrCreateNInputGate(type, designID);
    }
    if (type.isNOutput()) {
      return getOrCreateNOutputGate(type, designID);
    }
  }
  return nullptr;
}

}  // namespace naja::NL
