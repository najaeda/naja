// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesignModeling.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "NLBitDependencies.h"
#include "NLException.h"
#include "NLName.h"
#include "NajaDumpableProperty.h"
#include "NajaPrivateProperty.h"

#include "NLDB0.h"
#include "SNLDesign.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBitNet.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLInstParameter.h"
#include "SNLParameter.h"
#include "SNLScalarTerm.h"

// Common macros to unify repeated calculations without changing behavior
#define TT_NCHUNKS_FROM_BITS(nBits) \
  ((nBits) / 64 + (((nBits) % 64) > 0 ? 1 : 0))

#define TT_FILL_BITS(bitsVec, nBits, property, bitsIdx)          \
  do {                                                           \
    size_t _tt_nchunks = TT_NCHUNKS_FROM_BITS(nBits);            \
    for (size_t c = 0; c < _tt_nchunks; ++c) {                   \
      uint64_t mask = (property)->getUInt64Value((bitsIdx) + c); \
      for (size_t b = 0; b < 64; ++b) {                          \
        size_t pos = c * 64 + b;                                 \
        if (pos >= (nBits))                                      \
          break;                                                 \
        if ((mask >> b) & 1)                                     \
          (bitsVec)[pos] = true;                                 \
      }                                                          \
    }                                                            \
  } while (0)

namespace {

bool isDB0SequentialPrimitive(const naja::NL::SNLDesign* design) {
  return design &&
         (naja::NL::NLDB0::isDLatch(design) ||
          naja::NL::NLDB0::isDFF(design) ||
          naja::NL::NLDB0::isDFFN(design) ||
          naja::NL::NLDB0::isDFFRN(design) ||
          naja::NL::NLDB0::isDFFR(design) ||
          naja::NL::NLDB0::isDFFS(design) ||
          naja::NL::NLDB0::isDFFE(design) ||
          naja::NL::NLDB0::isDFFRE(design) ||
          naja::NL::NLDB0::isDFFSE(design) ||
          naja::NL::NLDB0::isDFFSR(design) ||
          naja::NL::NLDB0::isDFFSRN(design) ||
          naja::NL::NLDB0::isDFFSS(design) ||
          naja::NL::NLDB0::isDFFSSN(design) ||
          naja::NL::NLDB0::isDFFSRE(design) ||
          naja::NL::NLDB0::isDFFSRNE(design) ||
          naja::NL::NLDB0::isDFFSSE(design) ||
          naja::NL::NLDB0::isDFFSSNE(design));
}

naja::NL::SNLScalarTerm* getDB0SequentialClockTerm(const naja::NL::SNLDesign* design) {
  if (naja::NL::NLDB0::isDLatch(design)) {
    return design->getScalarTerm(naja::NL::NLName("E"));
  }
  if (naja::NL::NLDB0::isDFF(design) ||
      naja::NL::NLDB0::isDFFN(design) ||
      naja::NL::NLDB0::isDFFRN(design) ||
      naja::NL::NLDB0::isDFFR(design) ||
      naja::NL::NLDB0::isDFFS(design) ||
      naja::NL::NLDB0::isDFFE(design) ||
      naja::NL::NLDB0::isDFFRE(design) ||
      naja::NL::NLDB0::isDFFSE(design) ||
      naja::NL::NLDB0::isDFFSR(design) ||
      naja::NL::NLDB0::isDFFSRN(design) ||
      naja::NL::NLDB0::isDFFSS(design) ||
      naja::NL::NLDB0::isDFFSSN(design) ||
      naja::NL::NLDB0::isDFFSRE(design) ||
      naja::NL::NLDB0::isDFFSRNE(design) ||
      naja::NL::NLDB0::isDFFSSE(design) ||
      naja::NL::NLDB0::isDFFSSNE(design)) {
    return design->getScalarTerm(naja::NL::NLName("C"));
  }
  return nullptr;  // LCOV_EXCL_LINE
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getDB0ClockRelatedInputs(
    naja::NL::SNLBitTerm* clock) {
  auto design = clock->getDesign();
  auto db0Clock = getDB0SequentialClockTerm(design);
  if (clock != db0Clock) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return design->getBitTerms().getSubCollection(
      [db0Clock](const naja::NL::SNLBitTerm* term) {
        return term != db0Clock &&
               term->getDirection() != naja::NL::SNLTerm::Direction::Output;
      });
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getDB0ClockRelatedOutputs(
    naja::NL::SNLBitTerm* clock) {
  auto design = clock->getDesign();
  auto db0Clock = getDB0SequentialClockTerm(design);
  if (clock != db0Clock) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return design->getBitTerms().getSubCollection(
      [](const naja::NL::SNLBitTerm* term) {
        return term->getDirection() == naja::NL::SNLTerm::Direction::Output;
      });
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getDB0InputRelatedClocks(
    naja::NL::SNLBitTerm* input) {
  auto design = input->getDesign();
  auto db0Clock = getDB0SequentialClockTerm(design);
  if (!db0Clock || input == db0Clock ||
      input->getDirection() == naja::NL::SNLTerm::Direction::Output) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return naja::NajaCollection(new naja::NajaSingletonCollection(db0Clock))
      .getParentTypeCollection<naja::NL::SNLBitTerm*>();
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getDB0OutputRelatedClocks(
    naja::NL::SNLBitTerm* output) {
  auto db0Clock = getDB0SequentialClockTerm(output->getDesign());
  if (!db0Clock ||
      output->getDirection() != naja::NL::SNLTerm::Direction::Output) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return naja::NajaCollection(new naja::NajaSingletonCollection(db0Clock))
      .getParentTypeCollection<naja::NL::SNLBitTerm*>();
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getDB0ClockRelatedInputs(
    naja::NL::SNLInstTerm* clock) {
  auto instance = clock->getInstance();
  auto db0Clock = getDB0SequentialClockTerm(instance->getModel());
  if (!db0Clock || clock->getBitTerm() != db0Clock) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return instance->getInstTerms().getSubCollection(
      [db0Clock](const naja::NL::SNLInstTerm* term) {
        return term->getBitTerm() != db0Clock &&
               term->getBitTerm()->getDirection() != naja::NL::SNLTerm::Direction::Output;
      });
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getDB0ClockRelatedOutputs(
    naja::NL::SNLInstTerm* clock) {
  auto instance = clock->getInstance();
  auto db0Clock = getDB0SequentialClockTerm(instance->getModel());
  if (!db0Clock || clock->getBitTerm() != db0Clock) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return instance->getInstTerms().getSubCollection(
      [](const naja::NL::SNLInstTerm* term) {
        return term->getBitTerm()->getDirection() == naja::NL::SNLTerm::Direction::Output;
      });
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getDB0InputRelatedClocks(
    naja::NL::SNLInstTerm* input) {
  auto instance = input->getInstance();
  auto db0Clock = getDB0SequentialClockTerm(instance->getModel());
  if (!db0Clock || input->getBitTerm() == db0Clock ||
      input->getBitTerm()->getDirection() == naja::NL::SNLTerm::Direction::Output) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return naja::NajaCollection<naja::NL::SNLInstTerm*>(
      new naja::NajaSingletonCollection(instance->getInstTerm(db0Clock)));
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getDB0OutputRelatedClocks(
    naja::NL::SNLInstTerm* output) {
  auto instance = output->getInstance();
  auto db0Clock = getDB0SequentialClockTerm(instance->getModel());
  if (!db0Clock ||
      output->getBitTerm()->getDirection() != naja::NL::SNLTerm::Direction::Output) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return naja::NajaCollection<naja::NL::SNLInstTerm*>(
      new naja::NajaSingletonCollection(instance->getInstTerm(db0Clock)));
}

naja::NL::SNLDesignModeling::MemoryResetMode convertMemoryResetMode(
    naja::NL::NLDB0::MemoryResetMode mode) {
  switch (mode) {
    case naja::NL::NLDB0::MemoryResetMode::None:
      return naja::NL::SNLDesignModeling::MemoryResetMode::None;
    case naja::NL::NLDB0::MemoryResetMode::AsyncLow:
      return naja::NL::SNLDesignModeling::MemoryResetMode::AsyncLow;
    case naja::NL::NLDB0::MemoryResetMode::AsyncHigh:
      return naja::NL::SNLDesignModeling::MemoryResetMode::AsyncHigh;
    case naja::NL::NLDB0::MemoryResetMode::SyncLow:
      return naja::NL::SNLDesignModeling::MemoryResetMode::SyncLow;
    case naja::NL::NLDB0::MemoryResetMode::SyncHigh:
      return naja::NL::SNLDesignModeling::MemoryResetMode::SyncHigh;
  }
  return naja::NL::SNLDesignModeling::MemoryResetMode::None;  // LCOV_EXCL_LINE
}

naja::NL::SNLDesignModeling::MemoryInterface buildDB0MemoryInterface(
    const naja::NL::SNLDesign* design) {
  // DB0 memories already carry a compact signature. Expand that signature into
  // the same generic MemoryInterface shape used for Liberty/SV-inferred
  // memories so downstream clients do not need separate DB0-specific code.
  auto signature = naja::NL::NLDB0::getMemorySignature(design);
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  memInterface.width = signature.width;
  memInterface.depth = signature.depth;
  memInterface.abits = signature.abits;
  memInterface.resetMode = convertMemoryResetMode(signature.resetMode);
  memInterface.clock = naja::NL::NLDB0::getMemoryClock(design);
  memInterface.reset = naja::NL::NLDB0::getMemoryReset(design);

  auto* raddr = naja::NL::NLDB0::getMemoryReadAddress(design);
  auto* rdata = naja::NL::NLDB0::getMemoryReadData(design);
  auto* waddr = naja::NL::NLDB0::getMemoryWriteAddress(design);
  auto* wdata = naja::NL::NLDB0::getMemoryWriteData(design);
  auto* we = naja::NL::NLDB0::getMemoryWriteEnable(design);

  memInterface.readPorts.reserve(signature.readPorts);
  for (size_t port = 0; port < signature.readPorts; ++port) {
    naja::NL::SNLDesignModeling::MemoryReadPort readPort;
    for (size_t bit = 0; bit < signature.abits; ++bit) {
      readPort.address.push_back(
          static_cast<naja::NL::SNLBitTerm*>(
              raddr->getBit(static_cast<naja::NL::NLID::Bit>(
                  port * signature.abits + bit))));
    }
    for (size_t bit = 0; bit < signature.width; ++bit) {
      readPort.data.push_back(
          static_cast<naja::NL::SNLBitTerm*>(
              rdata->getBit(static_cast<naja::NL::NLID::Bit>(
                  port * signature.width + bit))));
    }
    memInterface.readPorts.push_back(std::move(readPort));
  }

  memInterface.writePorts.reserve(signature.writePorts);
  for (size_t port = 0; port < signature.writePorts; ++port) {
    naja::NL::SNLDesignModeling::MemoryWritePort writePort;
    for (size_t bit = 0; bit < signature.abits; ++bit) {
      writePort.address.push_back(
          static_cast<naja::NL::SNLBitTerm*>(
              waddr->getBit(static_cast<naja::NL::NLID::Bit>(
                  port * signature.abits + bit))));
    }
    for (size_t bit = 0; bit < signature.width; ++bit) {
      writePort.data.push_back(
          static_cast<naja::NL::SNLBitTerm*>(
              wdata->getBit(static_cast<naja::NL::NLID::Bit>(
                  port * signature.width + bit))));
    }
    // LCOV_EXCL_START
    writePort.enables.push_back(
        static_cast<naja::NL::SNLBitTerm*>(
            we->getBit(static_cast<naja::NL::NLID::Bit>(port))));
    // LCOV_EXCL_STOP
    memInterface.writePorts.push_back(std::move(writePort));
  }

  return memInterface;
}

bool containsBitTerm(
    const naja::NL::SNLDesignModeling::BitTerms& terms,
    const naja::NL::SNLBitTerm* candidate) {
  return std::find(terms.begin(), terms.end(), candidate) != terms.end();
}

bool isMemoryClockRelatedInputTerm(
    const naja::NL::SNLDesignModeling::MemoryInterface& memInterface,
    const naja::NL::SNLBitTerm* term) {
  if (term == nullptr || term == memInterface.clock) {
    return false;
  }
  if (memInterface.reset != nullptr && term == memInterface.reset) {
    return true;
  }
  // Read ports describe the combinational read surface of the memory. They do
  // not update stored state on the clock edge, so keep them out of the
  // clock-input relation. Write controls/data and reset are the sequential
  // next-state dependencies.
  for (const auto& writePort : memInterface.writePorts) {
    if (containsBitTerm(writePort.address, term) ||
        containsBitTerm(writePort.data, term) ||
        containsBitTerm(writePort.mask, term) ||
        containsBitTerm(writePort.enables, term)) {
      return true;
    }
    for (const auto& extraWriteInputs : writePort.extraWriteInputs) {
      if (containsBitTerm(extraWriteInputs, term)) {
        return true;
      }
    }
  }
  return false;
}

bool isMemoryClockRelatedOutputTerm(
    const naja::NL::SNLDesignModeling::MemoryInterface& memInterface,
    const naja::NL::SNLBitTerm* term) {
  if (term == nullptr) {
    return false; // LCOV_EXCL_LINE
  }
  for (const auto& readPort : memInterface.readPorts) {
    if (containsBitTerm(readPort.data, term)) {
      return true; 
    }
  }
  return false;
}

bool tryGetMemoryInterface(
    const naja::NL::SNLDesign* design,
    naja::NL::SNLDesignModeling::MemoryInterface& memInterface) {
  if (!naja::NL::SNLDesignModeling::hasMemoryInterface(design)) {
    return false; // LCOV_EXCL_LINE
  }
  memInterface = naja::NL::SNLDesignModeling::getMemoryInterface(design);
  return memInterface.clock != nullptr;
}

bool tryGetMemoryInterface(
    const naja::NL::SNLInstance* instance,
    naja::NL::SNLDesignModeling::MemoryInterface& memInterface) {
  if (instance == nullptr ||
      !naja::NL::SNLDesignModeling::hasMemoryInterface(instance->getModel())) {
    return false; // LCOV_EXCL_LINE
  }
  memInterface = naja::NL::SNLDesignModeling::getMemoryInterface(instance);
  return memInterface.clock != nullptr;
}

template <typename Predicate>
naja::NajaCollection<naja::NL::SNLBitTerm*> getMemoryBitTerms(
    naja::NL::SNLDesign* design,
    Predicate predicate) {
  return design->getBitTerms().getSubCollection(
      [predicate](const naja::NL::SNLBitTerm* term) {
        return predicate(term);
      }); // LCOV_EXCL_LINE
} // LCOV_EXCL_LINE

template <typename Predicate>
naja::NajaCollection<naja::NL::SNLInstTerm*> getMemoryInstTerms(
    naja::NL::SNLInstance* instance,
    Predicate predicate) {
  return instance->getInstTerms().getSubCollection(
      [predicate](const naja::NL::SNLInstTerm* term) {
        return term != nullptr && predicate(term->getBitTerm());
      }); // LCOV_EXCL_LINE
} // LCOV_EXCL_LINE

naja::NajaCollection<naja::NL::SNLBitTerm*> getMemoryClockRelatedInputs(
    naja::NL::SNLBitTerm* clock) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!clock || !tryGetMemoryInterface(clock->getDesign(), memInterface) ||
      clock != memInterface.clock) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return getMemoryBitTerms(
      clock->getDesign(), [memInterface](const naja::NL::SNLBitTerm* term) {
        return isMemoryClockRelatedInputTerm(memInterface, term);
      });
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getMemoryClockRelatedOutputs(
    naja::NL::SNLBitTerm* clock) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!clock || !tryGetMemoryInterface(clock->getDesign(), memInterface) ||
      clock != memInterface.clock) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return getMemoryBitTerms(
      clock->getDesign(), [memInterface](const naja::NL::SNLBitTerm* term) {
        return isMemoryClockRelatedOutputTerm(memInterface, term);
      });
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getMemoryInputRelatedClocks(
    naja::NL::SNLBitTerm* input) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!input || !tryGetMemoryInterface(input->getDesign(), memInterface) ||
      !isMemoryClockRelatedInputTerm(memInterface, input)) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return naja::NajaCollection(new naja::NajaSingletonCollection(memInterface.clock))
      .getParentTypeCollection<naja::NL::SNLBitTerm*>();
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getMemoryOutputRelatedClocks(
    naja::NL::SNLBitTerm* output) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!output || !tryGetMemoryInterface(output->getDesign(), memInterface) ||
      !isMemoryClockRelatedOutputTerm(memInterface, output)) {
    return naja::NajaCollection<naja::NL::SNLBitTerm*>();
  }
  return naja::NajaCollection(new naja::NajaSingletonCollection(memInterface.clock))
      .getParentTypeCollection<naja::NL::SNLBitTerm*>();
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getMemoryClockRelatedInputs(
    naja::NL::SNLInstTerm* clock) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!clock || !tryGetMemoryInterface(clock->getInstance(), memInterface) ||
      clock->getBitTerm() != memInterface.clock) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return getMemoryInstTerms(
      clock->getInstance(), [memInterface](const naja::NL::SNLBitTerm* term) {
        return isMemoryClockRelatedInputTerm(memInterface, term);
      });
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getMemoryClockRelatedOutputs(
    naja::NL::SNLInstTerm* clock) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!clock || !tryGetMemoryInterface(clock->getInstance(), memInterface) ||
      clock->getBitTerm() != memInterface.clock) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return getMemoryInstTerms(
      clock->getInstance(), [memInterface](const naja::NL::SNLBitTerm* term) {
        return isMemoryClockRelatedOutputTerm(memInterface, term);
      });
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getMemoryInputRelatedClocks(
    naja::NL::SNLInstTerm* input) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!input || !tryGetMemoryInterface(input->getInstance(), memInterface) ||
      !isMemoryClockRelatedInputTerm(memInterface, input->getBitTerm())) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return naja::NajaCollection<naja::NL::SNLInstTerm*>(
      new naja::NajaSingletonCollection(
          input->getInstance()->getInstTerm(memInterface.clock)));
}

naja::NajaCollection<naja::NL::SNLInstTerm*> getMemoryOutputRelatedClocks(
    naja::NL::SNLInstTerm* output) {
  naja::NL::SNLDesignModeling::MemoryInterface memInterface;
  if (!output || !tryGetMemoryInterface(output->getInstance(), memInterface) ||
      !isMemoryClockRelatedOutputTerm(memInterface, output->getBitTerm())) {
    return naja::NajaCollection<naja::NL::SNLInstTerm*>();
  }
  return naja::NajaCollection<naja::NL::SNLInstTerm*>(
      new naja::NajaSingletonCollection(
          output->getInstance()->getInstTerm(memInterface.clock)));
}

void validateMemoryInterfaceForDesign(
    naja::NL::SNLDesign* design,
    const naja::NL::SNLDesignModeling::MemoryInterface& memInterface) {
  if (!memInterface.isValid()) {
    throw naja::NL::NLException(
        "SNLDesignModeling::setMemoryInterface: invalid memory memInterface");
  }
  auto validateBitTerms = [design](const naja::NL::SNLDesignModeling::BitTerms& terms,
                                   const std::string& name) {
    for (auto* term : terms) {
      if (!term || term->getDesign() != design) {
        throw naja::NL::NLException(
            "SNLDesignModeling::setMemoryInterface: invalid " + name +
            " term ownership");
      }
    }
  };

  if (!memInterface.clock || memInterface.clock->getDesign() != design) {
    throw naja::NL::NLException(
        "SNLDesignModeling::setMemoryInterface: invalid clock term");
  }
  if (memInterface.reset && memInterface.reset->getDesign() != design) {
    throw naja::NL::NLException(
        "SNLDesignModeling::setMemoryInterface: invalid reset term");
  }

  for (const auto& port : memInterface.readPorts) {
    validateBitTerms(port.address, "read-address");
    validateBitTerms(port.data, "read-data");
    validateBitTerms(port.enables, "read-enable");
  }
  for (const auto& port : memInterface.writePorts) {
    validateBitTerms(port.address, "write-address");
    validateBitTerms(port.data, "write-data");
    validateBitTerms(port.mask, "write-mask");
    validateBitTerms(port.enables, "write-enable");
    for (const auto& extra : port.extraWriteInputs) {
      validateBitTerms(extra, "extra-write");
    }
  }
}

bool isConnectedInstanceBitTerm(const naja::NL::SNLInstance* instance,
                                const naja::NL::SNLBitTerm* term) {
  if (!instance || !term) {
    return false; // LCOV_EXCL_LINE
  }
  auto* instTerm = instance->getInstTerm(term);
  return instTerm != nullptr && instTerm->getNet() != nullptr;
}

bool areConnectedInstanceBitTerms(
    const naja::NL::SNLInstance* instance,
    const naja::NL::SNLDesignModeling::BitTerms& terms) {
  for (auto* term : terms) {
    if (!isConnectedInstanceBitTerm(instance, term)) {
      return false;
    }
  }
  return true;
}

bool isConnectedReadPort(
    const naja::NL::SNLInstance* instance,
    const naja::NL::SNLDesignModeling::MemoryReadPort& port) {
  return areConnectedInstanceBitTerms(instance, port.address) &&
         areConnectedInstanceBitTerms(instance, port.data) &&
         areConnectedInstanceBitTerms(instance, port.enables);
}

bool isConnectedWritePort(
    const naja::NL::SNLInstance* instance,
    const naja::NL::SNLDesignModeling::MemoryWritePort& port) {
  if (!areConnectedInstanceBitTerms(instance, port.address) ||
      !areConnectedInstanceBitTerms(instance, port.data) ||
      !areConnectedInstanceBitTerms(instance, port.mask) ||
      !areConnectedInstanceBitTerms(instance, port.enables)) {
    return false;
  }
  for (const auto& extraInputs : port.extraWriteInputs) {
    if (!areConnectedInstanceBitTerms(instance, extraInputs)) {
      return false;
    }
  }
  return true;
}

class SNLInstanceStateProperty: public naja::NajaPrivateProperty {
  public:
    using Inherit = naja::NajaPrivateProperty;
    static const inline std::string Name = "SNLInstanceStateProperty";

    static SNLInstanceStateProperty* create(naja::NL::SNLInstance* instance) {
      preCreate(instance, Name);
      auto* property = new SNLInstanceStateProperty();
      property->postCreate(instance);
      return property;
    }

    static SNLInstanceStateProperty* get(const naja::NL::SNLInstance* instance) {
      return instance
        ? static_cast<SNLInstanceStateProperty*>(instance->getProperty(Name))
        : nullptr;
    }

    static SNLInstanceStateProperty* getOrCreate(naja::NL::SNLInstance* instance) {
      auto* property = get(instance);
      return property ? property : create(instance);
    }

    std::string getName() const override { return Name; }
    //LCOV_EXCL_START
    std::string getString() const override { return Name; }
    //LCOV_EXCL_STOP

    std::optional<naja::NL::NLLogicVector> initValue_  {};
    std::optional<naja::NL::NLLogicVector> resetValue_ {};
};

class SNLConstantDriverProperty: public naja::NajaPrivateProperty {
  public:
    using Inherit = naja::NajaPrivateProperty;
    static const inline std::string Name = "SNLConstantDriverProperty";

    static SNLConstantDriverProperty* create(
        naja::NL::SNLInstance* instance,
        const naja::NL::NLConstantDriver& driver) {
      preCreate(instance, Name);
      auto* property = new SNLConstantDriverProperty();
      property->driver_ = driver;
      property->postCreate(instance);
      return property;
    }

    static SNLConstantDriverProperty* get(const naja::NL::SNLInstance* instance) {
      return instance
        ? static_cast<SNLConstantDriverProperty*>(instance->getProperty(Name))
        : nullptr;
    }

    std::string getName() const override { return Name; }
    //LCOV_EXCL_START
    std::string getString() const override { return Name; }
    //LCOV_EXCL_STOP

    naja::NL::NLConstantDriver driver_ {};
};

std::optional<std::string> getEffectiveParameterValue(
    const naja::NL::SNLInstance* instance,
    const naja::NL::SNLDesign* model,
    const naja::NL::NLName& name) {
  if (auto* instanceParameter = instance->getInstParameter(name)) {
    return instanceParameter->getValue();
  }
  if (auto* parameter = model->getParameter(name)) {
    return parameter->getValue();
  }
  return std::nullopt;
}

size_t getDecimalParameterValue(
    const naja::NL::SNLInstance* instance,
    const naja::NL::SNLDesign* model,
    const naja::NL::NLName& name) {
  const auto value = getEffectiveParameterValue(instance, model, name);
  if (!value) {
    throw naja::NL::NLException(
      "SNLDesignModeling: missing " + name.getString() + " value");
  }
  try {
    size_t position = 0;
    const auto result = std::stoull(*value, &position);
    if (position != value->size()) {
      throw std::invalid_argument("trailing characters");
    }
    return result;
  } catch (const std::exception&) {
    throw naja::NL::NLException(
      "SNLDesignModeling: invalid " + name.getString() + " value");
  }
}

size_t getInitializationWidth(
    const naja::NL::SNLInstance* instance,
    const naja::NL::SNLDesign* model) {
  if (naja::NL::NLDB0::isMemory(model)) {
    return getDecimalParameterValue(instance, model, naja::NL::NLName("WIDTH")) *
           getDecimalParameterValue(instance, model, naja::NL::NLName("DEPTH"));
  }
  if (isDB0SequentialPrimitive(model)) {
    return getDecimalParameterValue(instance, model, naja::NL::NLName("WIDTH"));
  }
  throw naja::NL::NLException(
    "SNLDesignModeling: instance is not a DB0 state element");
}

bool hasMemoryReset(
    const naja::NL::SNLInstance* instance,
    const naja::NL::SNLDesign* model) {
  return naja::NL::NLDB0::isMemory(model) &&
         getDecimalParameterValue(
           instance, model, naja::NL::NLName("RST_ENABLE")) != 0;
}

class SNLDesignModelingProperty : public naja::NajaPrivateProperty {
 public:
  using Inherit = naja::NajaPrivateProperty;
  static const inline std::string Name = "SNLDesignModelingProperty";
  static SNLDesignModelingProperty* create(
      naja::NL::SNLDesign* design,
      naja::NL::SNLDesignModeling::Type type) {
    preCreate(design, Name);
    SNLDesignModelingProperty* property = new SNLDesignModelingProperty();
    property->modeling_ = new naja::NL::SNLDesignModeling(type);
    property->postCreate(design);
    return property;
  }
  static void preCreate(naja::NL::SNLDesign* design, const std::string& name) {
    Inherit::preCreate(design, name);
    if (not(design->isLeaf())) {
      std::ostringstream reason;
      reason << "Impossible to add Timing Modeling on a non leaf design <"
             << design->getName().getString() << ">";
      throw naja::NL::NLException(reason.str());
    }
  }
  void preDestroy() override {
    if (modeling_) {
      delete modeling_;
    }
    Inherit::preDestroy();
  }
  naja::NL::SNLDesignModeling::Type getModelingType() const {
    return getModeling()->getType();
  }
  std::string getName() const override { return Name; }
  // LCOV_EXCL_START
  std::string getString() const override { return Name; }
  // LCOV_EXCL_STOP
  naja::NL::SNLDesignModeling* getModeling() const { return modeling_; }

 private:
  naja::NL::SNLDesignModeling* modeling_{nullptr};
};

SNLDesignModelingProperty* getProperty(const naja::NL::SNLDesign* design) {
  auto property = static_cast<SNLDesignModelingProperty*>(
      design->getProperty(SNLDesignModelingProperty::Name));
  if (property) {
    return property;
  }
  return nullptr;
}

// type will used only if the property is created
SNLDesignModelingProperty* getOrCreateProperty(
    naja::NL::SNLDesign* design,
    naja::NL::SNLDesignModeling::Type type) {
  auto property = getProperty(design);
  if (property) {
    return property;
  }
  return SNLDesignModelingProperty::create(design, type);
}

void insertInArcs(naja::NL::SNLDesignModeling::Arcs& arcs,
                  naja::NL::SNLBitTerm* term0,
                  naja::NL::SNLBitTerm* term1) {
  auto iit = arcs.find(term0);
  if (iit == arcs.end()) {
    auto result = arcs.insert({term0, naja::NL::SNLDesignModeling::TermArcs()});
    if (not result.second) {
      throw naja::NL::NLException("Error while inserting in timing arcs");
    }
    iit = result.first;
  }
  naja::NL::SNLDesignModeling::TermArcs& termArcs = iit->second;
  auto oit = termArcs.find(term1);
  if (oit != termArcs.end()) {
    throw naja::NL::NLException("Error while inserting in timing arcs");
  }
  termArcs.insert(term1);
}

naja::NL::SNLDesign* verifyInputs(
    const naja::NL::SNLDesignModeling::BitTerms& terms0,
    const std::string& terms0Naming,
    const naja::NL::SNLDesignModeling::BitTerms& terms1,
    const std::string& terms1Naming,
    const std::string& method) {
  if (terms0.empty()) {
    throw naja::NL::NLException("Error in " + method + ": empty " +
                                terms0Naming);
  }
  if (terms1.empty()) {
    throw naja::NL::NLException("Error in " + method + ": empty " +
                                terms1Naming);
  }
  naja::NL::SNLDesign* design = nullptr;
  for (auto term : terms0) {
    if (not design) {
      design = term->getDesign();
    } else if (design not_eq term->getDesign()) {
      throw naja::NL::NLException("Error in " + method +
                                  ": incompatible designs");
    }
  }
  for (auto term : terms1) {
    if (design not_eq term->getDesign()) {
      throw naja::NL::NLException("Error in " + method +
                                  ": incompatible designs");
    }
  }
  return design;
}

#define GET_RELATED_TERMS_IN_ARCS(ARCS)        \
  const auto* timingArcs = getTimingArcs();    \
  const auto it = timingArcs->ARCS.find(term); \
  if (it == timingArcs->ARCS.end()) {          \
    return NajaCollection<SNLBitTerm*>();      \
  }                                            \
  return NajaCollection(new NajaSTLCollection(&(it->second)));

#define GET_RELATED_INSTTERMS_IN_ARCS(ARCS)                   \
  auto instance = iterm->getInstance();                       \
  const TimingArcs* timingArcs = getTimingArcs(instance);     \
  auto it = timingArcs->ARCS.find(iterm->getBitTerm());       \
  if (it == timingArcs->ARCS.end()) {                         \
    return NajaCollection<SNLInstTerm*>();                    \
  }                                                           \
  auto transformer = [=](const SNLBitTerm* term) {            \
    return instance->getInstTerm(term);                       \
  };                                                          \
  return NajaCollection(new NajaSTLCollection(&(it->second))) \
      .getTransformerCollection<SNLInstTerm*>(transformer);

#define GET_RELATED_OBJECTS(TYPE, INPUT, DESIGN_GETTER, GETTER) \
  auto property = getProperty(INPUT->DESIGN_GETTER);            \
  if (property) {                                               \
    auto modeling = property->getModeling();                    \
    return modeling->GETTER(INPUT);                             \
  }                                                             \
  return NajaCollection<TYPE*>();

static const std::string SNLDesignTruthTablePropertyName =
    "SNLDesignTruthTableProperty";

naja::NajaDumpableProperty* getTruthTableProperty(
    const naja::NL::SNLDesign* design) {
  auto property = static_cast<naja::NajaDumpableProperty*>(
      design->getProperty(SNLDesignTruthTablePropertyName));
  return property;
}

size_t getDependencyChunkCount(const naja::NL::SNLDesign* design) {
  return TT_NCHUNKS_FROM_BITS(design->getBitTerms().size()); }

std::vector<size_t> getInputFlatPositions(const naja::NL::SNLDesign* design) {
  std::vector<size_t> inputFlatPositions;
  size_t flatPos = 0;
  for (auto term : design->getBitTerms()) {
    if (term->getDirection() != naja::NL::SNLTerm::Direction::Output) {
      inputFlatPositions.push_back(flatPos);
    }
    ++flatPos;
  }
  return inputFlatPositions;
}

std::vector<uint64_t> validateAndPadDependenciesForDesign(
    const naja::NL::SNLDesign* design,
    const naja::NL::SNLTruthTable& truthTable) {
  const auto depChunkCount = getDependencyChunkCount(design);
  if (truthTable.size() == 0) {
    return std::vector<uint64_t>(depChunkCount, 0);
  }
  const auto& deps = truthTable.getDependencies();
  if (deps.empty()) {
    throw naja::NL::NLException("Truth table dependencies are required");
  }

  const auto decodedDeps = naja::NL::NLBitDependencies::decodeBits(deps);
  if (decodedDeps.empty()) {
    throw naja::NL::NLException("Truth table dependencies are required");
  }
  // SNLTruthTable constructors already reject oversized dependency sets.
  // LCOV_EXCL_START
  if (decodedDeps.size() > truthTable.size()) {
    throw naja::NL::NLException(
        "Truth table dependencies count cannot exceed truth table size");
  }
  // LCOV_EXCL_STOP

  const auto inputFlatPositions = getInputFlatPositions(design);
  if (inputFlatPositions.empty()) {
    std::vector<uint64_t> paddedDeps(depChunkCount, 0);
    std::copy(deps.begin(), deps.end(), paddedDeps.begin());
    return paddedDeps;
  }
  for (size_t dep : decodedDeps) {
    if (std::find(inputFlatPositions.begin(), inputFlatPositions.end(), dep) ==
        inputFlatPositions.end()) {
      std::ostringstream reason;
      reason << "Truth table dependency " << dep
             << " is not an input flat-term position in design <"
             << design->getName().getString() << ">";
      throw naja::NL::NLException(reason.str());
    }
  }

  if (deps.size() > depChunkCount) {
    std::ostringstream reason;
    reason << "Truth table dependency storage for design <"
           << design->getName().getString() << "> exceeds flat-term span";
    throw naja::NL::NLException(reason.str());
  }

  std::vector<uint64_t> paddedDeps(depChunkCount, 0);
  std::copy(deps.begin(), deps.end(), paddedDeps.begin());
  return paddedDeps;
}

void advanceTruthTableValueIndex(const naja::NL::SNLDesign* design,
                                 size_t& valIdx,
                                 uint32_t nInputs) {
  size_t ttNBits = static_cast<size_t>(1u) << nInputs;
  valIdx += 1;
  valIdx += TT_NCHUNKS_FROM_BITS(ttNBits);
  valIdx += getDependencyChunkCount(design);
}

void createTruthTableProperty(naja::NL::SNLDesign* design,
                              const naja::NL::SNLTruthTable& truthTable) {
  // LCOV_EXCL_START
  if (getTruthTableProperty(design)) {
    throw naja::NL::NLException("Design already has a Truth Table");
  }
  // LCOV_EXCL_STOP
  const auto deps = validateAndPadDependenciesForDesign(design, truthTable);
  auto property = naja::NajaDumpableProperty::create(
      design, SNLDesignTruthTablePropertyName);
  property->addUInt64Value(truthTable.size());
  for (auto mask : truthTable.bits().getChunks()) {
    property->addUInt64Value(mask);
  }
  for (auto dep : deps) {
    property->addUInt64Value(dep);
  }
}

void createTruthTableProperty(
    naja::NL::SNLDesign* design,
    const std::vector<naja::NL::SNLTruthTable>& truthTables) {
  if (truthTables.empty()) {
    throw naja::NL::NLException("Cannot set empty truth table");
  }
  // LCOV_EXCL_START
  if (getTruthTableProperty(design)) {
    throw naja::NL::NLException("Design already has a Truth Table");
  }
  // LCOV_EXCL_STOP
  std::vector<std::vector<uint64_t>> depsPerTable;
  depsPerTable.reserve(truthTables.size());
  for (const auto& truthTable : truthTables) {
    depsPerTable.push_back(
        validateAndPadDependenciesForDesign(design, truthTable));
  }
  auto property = naja::NajaDumpableProperty::create(
      design, SNLDesignTruthTablePropertyName);
  for (size_t index = 0; index < truthTables.size(); ++index) {
    const auto& truthTable = truthTables[index];
    property->addUInt64Value(truthTable.size());
    for (auto mask : truthTable.bits().getChunks()) {
      property->addUInt64Value(mask);
    }
    for (auto dep : depsPerTable[index]) {
      property->addUInt64Value(dep);
    }
  }
}

naja::NajaCollection<naja::NL::SNLBitTerm*>
getCombinatorialOutputsDepsFromTruthTable(naja::NL::SNLBitTerm* term) {
  size_t flatID = term->getOrderID();

  // Find the flatID of the term
  auto flatTerms = term->getDesign()->getBitTerms();

  // build map: output‐index → flat‐index
  std::map<size_t, size_t> outputFlatID2FlatID;
  size_t outputID = 0, flatIdx = 0;
  for (auto const& ft : flatTerms) {
    if (ft->getDirection() != naja::NL::SNLTerm::Direction::Input) {
      outputFlatID2FlatID[outputID++] = flatIdx;
    }
    ++flatIdx;
  }

  // turn to vector for std::find_if
  std::vector<naja::NL::SNLBitTerm*> flatTermsVec(flatTerms.begin(),
                                                  flatTerms.end());

  // LCOV_EXCL_START
  // ensure there's at least one truth table
  if (naja::NL::SNLDesignModeling::getTruthTableCount(term->getDesign()) == 0) {
    std::ostringstream reason;
    reason << "Design <" << term->getDesign()->getName().getString()
           << "> has no truth table";
    throw naja::NL::NLException(reason.str());
  }

  // LCOV_EXCL_STOP
  auto design = term->getDesign();
  auto property = getTruthTableProperty(design);
  size_t tableIdx = 0;
  std::vector<size_t> flatDepIDs;

  if (property) {
    size_t valIdx = 0;
    size_t total = property->getValues().size();

    while (valIdx < total) {
      uint32_t nInputs =
          static_cast<uint32_t>(property->getUInt64Value(valIdx));
      size_t nBits = static_cast<size_t>(1u) << nInputs;
      size_t nchunks = TT_NCHUNKS_FROM_BITS(nBits);
      size_t ndeps = getDependencyChunkCount(design);

      // collect dependency masks
      std::vector<uint64_t> deps;
      for (size_t i = valIdx + 1 + nchunks; i < valIdx + 1 + nchunks + ndeps;
           ++i) {
        deps.push_back(property->getUInt64Value(i));
      }

      auto decodedDeps = naja::NL::NLBitDependencies::decodeBits(deps);
      if (decodedDeps.empty() ||
          std::find(decodedDeps.begin(), decodedDeps.end(), flatID) !=
              decodedDeps.end()) {
        auto mapIt = outputFlatID2FlatID.find(tableIdx);
        // LCOV_EXCL_START
        if (mapIt == outputFlatID2FlatID.end()) {
          std::ostringstream reason;
          reason << "Output flat ID " << tableIdx << " not found in map";
          throw naja::NL::NLException(reason.str());
        }
        // LCOV_EXCL_STOP
        flatDepIDs.push_back(mapIt->second);
      }

      // advance valIdx past this table's values, masks, and count entry
      advanceTruthTableValueIndex(design, valIdx, nInputs);

      // LCOV_EXCL_START: malformed‐table guard
      if (valIdx >= total + 1) {
        std::ostringstream reason;
        std::string all;
        for (size_t i = 0; i < total; ++i) {
          all += std::to_string(property->getUInt64Value(i)) + " ";
        }
        reason << "Malformed truth table for design <"
               << design->getName().getString() << "> " << all
               << "\nWith valIdx " << valIdx << " and total " << total;
        throw naja::NL::NLException(reason.str());
      }
      // LCOV_EXCL_STOP

      ++tableIdx;
    }
  }

  // build result collection from unique (ID,bit) pairs
  std::set<
      std::pair<naja::NL::NLID::DesignObjectID, naja::NL::NLID::DesignObjectID>>
      depTermsIds;
  for (auto id : flatDepIDs) {
    auto bt = flatTermsVec[id];
    // LCOV_EXCL_START
    if (!bt) {
      std::ostringstream reason;
      reason << "Bit term with ID " << id << " not found in design "
             << term->getDesign()->getName().getString();
      throw naja::NL::NLException(reason.str());
    }
    // LCOV_EXCL_STOP
    depTermsIds.insert({bt->getID(), bt->getBit()});
  }

  auto filter = [=](auto const* bterm) {
    return depTermsIds.count({bterm->getID(), bterm->getBit()}) > 0;
  };
  return flatTerms.getSubCollection(filter);
}

naja::NajaCollection<naja::NL::SNLBitTerm*>
getCombinatorialInputDepsFromTruthTable(naja::NL::SNLBitTerm* term) {
  size_t flatID = term->getOrderID();
  // Find the flatID of the term
  auto flatTerms = term->getDesign()->getBitTerms();
  // create a map for output index and flatID
  std::map<size_t, size_t> outputFlatID2FlatID;

  // turn to vector to use the STL algorithm
  std::vector<naja::NL::SNLBitTerm*> flatTermsVec(flatTerms.begin(),
                                                  flatTerms.end());
  size_t tableCount =
      naja::NL::SNLDesignModeling::getTruthTableCount(term->getDesign());
  if (tableCount == 0) {
    return {};
  }
  naja::NL::SNLTruthTable truthTable;
  if (tableCount == 1) {
    truthTable = naja::NL::SNLDesignModeling::getTruthTable(term->getDesign());
  } else {
    truthTable =
        naja::NL::SNLDesignModeling::getTruthTable(term->getDesign(), flatID);
  }
  const auto& deps = truthTable.getDependencies();
  if (naja::NL::NLBitDependencies::countBitsForVector(deps) == 0) {
    // In the absence of any deps, we assume—by default—that everything depends
    // on everything
    return flatTerms.getSubCollection([](const naja::NL::SNLBitTerm* bterm) {
      return bterm->getDirection() != naja::NL::SNLBitTerm::Direction::Output;
    });
  }
  // auto flatTermsIDs = naja::NL::NLBitDependencies::decodeBits(deps);
  // auto flatTerms = term->getDesign()->getBitTerms();
  //  turn to vector to use the STL algorithm
  // std::vector<naja::NL::SNLBitTerm*> flatTermsVec(flatTerms.begin(),
  //                                                 flatTerms.end());

  std::set<
      std::pair<naja::NL::NLID::DesignObjectID, naja::NL::NLID::DesignObjectID>>
      depTermsIds;
  // extract bit values for depsß
  size_t depsIdx = 0;
  for (auto chunk : deps) {
    for (size_t b = 0; b < 64; ++b) {
      size_t pos = depsIdx * 64 + b;
      if (pos >= flatTerms.size())
        break;
      if ((chunk >> b) & 1) {
        auto bt = flatTermsVec[pos];
        // LCOV_EXCL_START
        if (!bt) {
          std::ostringstream reason;
          reason << "Bit term with ID " << pos << " not found in design "
                 << term->getDesign()->getName().getString();
          throw naja::NL::NLException(reason.str());
        }
        // LCOV_EXCL_STOP
        depTermsIds.insert({bt->getID(), bt->getBit()});
      }
    }
    depsIdx++;
  }
  auto filter = [=](const naja::NL::SNLBitTerm* bterm) {
    return depTermsIds.find({bterm->getID(), bterm->getBit()}) !=
           depTermsIds.end();
  };
  return flatTerms.getSubCollection(filter);
}

naja::NajaCollection<naja::NL::SNLBitTerm*> getCombinatorialDepsFromTruthTable(
    naja::NL::SNLBitTerm* term) {
  if (term->getDirection() != naja::NL::SNLTerm::Direction::Input) {
    return getCombinatorialInputDepsFromTruthTable(term);
  }
  return getCombinatorialOutputsDepsFromTruthTable(term);
}

}  // namespace

namespace naja::NL {

SNLDesignModeling::SNLDesignModeling(Type type) : type_(type) {
  if (type_ == NO_PARAMETER) {
    model_ = TimingArcs();
  } else {
    model_ = ParameterizedArcs();
  }
}

void SNLDesignModeling::addCombinatorialArc_(SNLBitTerm* input,
                                             SNLBitTerm* output) {
  TimingArcs* arcs = getOrCreateTimingArcs();
  insertInArcs(arcs->inputCombinatorialArcs_, input, output);
  insertInArcs(arcs->outputCombinatorialArcs_, output, input);
}

void SNLDesignModeling::addCombinatorialArc_(
    SNLBitTerm* input,
    SNLBitTerm* output,
    const std::string& parameterValue) {
  TimingArcs* arcs = getOrCreateTimingArcs(parameterValue);
  insertInArcs(arcs->inputCombinatorialArcs_, input, output);
  insertInArcs(arcs->outputCombinatorialArcs_, output, input);
}

void SNLDesignModeling::addInputToClockArc_(SNLBitTerm* input,
                                            SNLBitTerm* clock) {
  if (type_ not_eq Type::NO_PARAMETER) {
    throw NLException("Wrong SNLDesignModeling type for addInputToClockArc");
  }
  TimingArcs& arcs = std::get<Type::NO_PARAMETER>(model_);
  insertInArcs(arcs.inputToClockArcs_, input, clock);
  insertInArcs(arcs.clockToInputArcs_, clock, input);
}

void SNLDesignModeling::addClockToOutputArc_(SNLBitTerm* clock,
                                             SNLBitTerm* output) {
  if (type_ not_eq Type::NO_PARAMETER) {
    throw NLException("Wrong SNLDesignModeling type for addClockToOutputArc");
  }
  TimingArcs& arcs = std::get<Type::NO_PARAMETER>(model_);
  insertInArcs(arcs.outputToClockArcs_, output, clock);
  insertInArcs(arcs.clockToOutputArcs_, clock, output);
}

SNLDesignModeling::TimingArcs* SNLDesignModeling::getOrCreateTimingArcs(
    const std::string& parameterValue) {
  if (type_ == Type::NO_PARAMETER) {
    if (not parameterValue.empty()) {
      throw NLException("Contradictory type in SNLDesignModeling");
    }
    return &std::get<Type::NO_PARAMETER>(model_);
  } else {
    std::string paramValue = parameter_.second;
    if (not parameterValue.empty()) {
      paramValue = parameterValue;
    }
    ParameterizedArcs& parameterizedArcs =
        std::get<Type::PARAMETERIZED>(model_);
    auto ait = parameterizedArcs.find(paramValue);
    if (ait == parameterizedArcs.end()) {
      // create it
      auto result = parameterizedArcs.insert({paramValue, TimingArcs()});
      if (not result.second) {
        throw naja::NL::NLException("Error in Timing arcs insertion");
      }
      ait = result.first;
    }
    return &(ait->second);
  }
}

const SNLDesignModeling::TimingArcs* SNLDesignModeling::getTimingArcs(
    const SNLInstance* instance) const {
  if (type_ == Type::NO_PARAMETER) {
    return &std::get<Type::NO_PARAMETER>(model_);
  } else {
    const ParameterizedArcs& parameterizedArcs =
        std::get<Type::PARAMETERIZED>(model_);
    // find the parameter value
    if (instance != nullptr) {
      // //get Arcs from parameter
      auto parameter = parameter_.first;
      // find parameter in instance
      auto instParameter = instance->getInstParameter(NLName(parameter));
      if (instParameter) {
        auto value = instParameter->getValue();
        auto pit = parameterizedArcs.find(value);
        if (pit == parameterizedArcs.end()) {
          std::ostringstream reason;
          reason << "Parameter value <" << value << "> for Parameter <"
                 << parameter << "> cannot be found in design <"
                 << instance->getModel()->getName().getString()
                 << "> modeling. Existing values are: ";
          bool first = true;
          for (auto parcs : parameterizedArcs) {
            if (!first) {
              reason << ", ";
            }
            reason << parcs.first;
            first = false;
          }
          throw NLException(reason.str());
        }
        return &(pit->second);
      }
      // if not found then switch to default parameter
    }
    auto defaultParameterValue = parameter_.second;
    if (defaultParameterValue.empty()) {
      // LCOV_EXCL_START
      throw NLException("No Default parameter value while getting Timing Arcs");
      // LCOV_EXCL_STOP
    }
    auto ait = parameterizedArcs.find(defaultParameterValue);
    if (ait != parameterizedArcs.end()) {
      return &(ait->second);
    } else {
      // LCOV_EXCL_START
      std::ostringstream reason;
      reason << "cannot find " << defaultParameterValue
             << " in parameterized arcs.";
      throw NLException(reason.str());
      // LCOV_EXCL_STOP
    }
  }
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialOutputs_(
    SNLBitTerm* term) const {GET_RELATED_TERMS_IN_ARCS(inputCombinatorialArcs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialOutputs_(
    SNLInstTerm* iterm) const {
    GET_RELATED_INSTTERMS_IN_ARCS(inputCombinatorialArcs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialInputs_(
    SNLBitTerm* term) const {
    GET_RELATED_TERMS_IN_ARCS(outputCombinatorialArcs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialInputs_(
    SNLInstTerm* iterm) const {
    GET_RELATED_INSTTERMS_IN_ARCS(outputCombinatorialArcs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockRelatedInputs_(
    SNLBitTerm* term) const {GET_RELATED_TERMS_IN_ARCS(clockToInputArcs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockRelatedOutputs_(
    SNLBitTerm* term) const {GET_RELATED_TERMS_IN_ARCS(clockToOutputArcs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getInputRelatedClocks_(
    SNLBitTerm* term) const {GET_RELATED_TERMS_IN_ARCS(inputToClockArcs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getOutputRelatedClocks_(
    SNLBitTerm* term) const {GET_RELATED_TERMS_IN_ARCS(outputToClockArcs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getClockRelatedInputs_(
    SNLInstTerm* iterm) const {GET_RELATED_INSTTERMS_IN_ARCS(clockToInputArcs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getClockRelatedOutputs_(
    SNLInstTerm* iterm) const {
    GET_RELATED_INSTTERMS_IN_ARCS(clockToOutputArcs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getInputRelatedClocks_(
    SNLInstTerm* iterm) const {GET_RELATED_INSTTERMS_IN_ARCS(inputToClockArcs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getOutputRelatedClocks_(
    SNLInstTerm* iterm) const {
  GET_RELATED_INSTTERMS_IN_ARCS(outputToClockArcs_)
}

void SNLDesignModeling::addCombinatorialArcs(const BitTerms& inputs,
                                             const BitTerms& outputs) {
  auto design = verifyInputs(inputs, "inputs", outputs, "outputs",
                             "addCombinatorialArcs");
  auto property = getOrCreateProperty(design, Type::NO_PARAMETER);
  auto modeling = property->getModeling();
  for (auto input : inputs) {
    for (auto output : outputs) {
      modeling->addCombinatorialArc_(input, output);
    }
  }
}

void SNLDesignModeling::addCombinatorialArcs(const std::string& parameterValue,
                                             const BitTerms& inputs,
                                             const BitTerms& outputs) {
  auto design = verifyInputs(inputs, "inputs", outputs, "outputs",
                             "addCombinatorialArcs");
  auto property = getOrCreateProperty(design, Type::PARAMETERIZED);
  auto modeling = property->getModeling();
  for (auto input : inputs) {
    for (auto output : outputs) {
      modeling->addCombinatorialArc_(input, output, parameterValue);
    }
  }
}

void SNLDesignModeling::addInputsToClockArcs(const BitTerms& inputs,
                                             SNLBitTerm* clock) {
  auto design =
      verifyInputs(inputs, "inputs", {clock}, "clock", "addInputsToClockArcs");
  auto property = getOrCreateProperty(design, Type::NO_PARAMETER);
  auto modeling = property->getModeling();
  for (auto input : inputs) {
    modeling->addInputToClockArc_(input, clock);
  }
}

void SNLDesignModeling::addClockToOutputsArcs(SNLBitTerm* clock,
                                              const BitTerms& outputs) {
  auto design = verifyInputs({clock}, "clock", outputs, "outputs",
                             "addClockToOutputsArcs");
  auto property = getOrCreateProperty(design, Type::NO_PARAMETER);
  auto modeling = property->getModeling();
  for (auto output : outputs) {
    modeling->addClockToOutputArc_(clock, output);
  }
}

void SNLDesignModeling::setParameter(SNLDesign* design,
                                     const std::string& name,
                                     const std::string& defaultValue) {
  auto parameter = design->getParameter(NLName(name));
  if (not parameter) {
    std::ostringstream reason;
    reason << "Parameter " << name << " is unknown in "
           << design->getName().getString();
    throw NLException(reason.str());
  }
  auto property = getOrCreateProperty(design, Type::PARAMETERIZED);
  auto modeling = property->getModeling();
  modeling->parameter_ = std::make_pair(name, defaultValue);
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialOutputs(
    SNLBitTerm* term) {
  auto property = getProperty(term->getDesign());
  if (property) {
    GET_RELATED_OBJECTS(SNLBitTerm, term, getDesign(), getCombinatorialOutputs_)
  } else {
    if (naja::NL::SNLDesignModeling::getTruthTableCount(term->getDesign()) > 0) {
      if (naja::NL::SNLDesignModeling::areDependenciesDefined(term)) {
        return getCombinatorialDepsFromTruthTable(term);
      }
    }
  }
  // return all outputs of the design
  return term->getDesign()->getBitTerms().getSubCollection(
      [](const SNLBitTerm* t) {
        return t->getDirection() != SNLTerm::Direction::Input;
      });
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialOutputs(
    SNLInstTerm* iterm) {
  auto property = getProperty(iterm->getInstance()->getModel());
  if (property) {
    GET_RELATED_OBJECTS(SNLInstTerm, iterm, getInstance()->getModel(),
                        getCombinatorialOutputs_)
  } else {
    if (naja::NL::SNLDesignModeling::getTruthTableCount(
            iterm->getBitTerm()->getDesign()) > 0) {
      if (naja::NL::SNLDesignModeling::areDependenciesDefined(
              iterm->getBitTerm())) {
        return getCombinatorialDepsFromTruthTable(iterm->getBitTerm())
              .getTransformerCollection<SNLInstTerm*>(
                  [=](const SNLBitTerm* term) {
                    return iterm->getInstance()->getInstTerm(term);
                  });
      }
    }
  }
  // return all outputs of the instance
  return iterm->getInstance()
      ->getModel()
      ->getBitTerms()
      .getSubCollection([](const SNLBitTerm* t) {
        return t->getDirection() != SNLTerm::Direction::Input;
      })
      .getTransformerCollection<SNLInstTerm*>([=](const SNLBitTerm* term) {
        return iterm->getInstance()->getInstTerm(term);
      });
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getCombinatorialInputs(
    SNLBitTerm* term) {
  auto property = getProperty(term->getDesign());
  if (property) {
    GET_RELATED_OBJECTS(SNLBitTerm, term, getDesign(), getCombinatorialInputs_)
  } else {
    if (naja::NL::SNLDesignModeling::getTruthTableCount(term->getDesign()) >
        0) {
      auto snltt = naja::NL::SNLDesignModeling::getTruthTable(
          term->getDesign(), term->getOrderID());
      if (NLBitDependencies::countBitsForVector(snltt.getDependencies()) > 0) {
        return getCombinatorialDepsFromTruthTable(term);
      }
    }
  }
  // return all inputs of the design
  return term->getDesign()->getBitTerms().getSubCollection(
      [](const SNLBitTerm* t) {
        return t->getDirection() != SNLTerm::Direction::Output;
      });
}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getCombinatorialInputs(
    SNLInstTerm* iterm) {
  auto property = getProperty(iterm->getInstance()->getModel());
  if (property) {
    GET_RELATED_OBJECTS(SNLInstTerm, iterm, getInstance()->getModel(),
                        getCombinatorialInputs_)
  } else {
    if (naja::NL::SNLDesignModeling::getTruthTableCount(
            iterm->getInstance()->getModel()) > 0) {
      auto snltt = naja::NL::SNLDesignModeling::getTruthTable(
          iterm->getInstance()->getModel(), iterm->getBitTerm()->getOrderID());
      if (NLBitDependencies::countBitsForVector(snltt.getDependencies()) > 0) {
        return getCombinatorialDepsFromTruthTable(iterm->getBitTerm())
            .getTransformerCollection<SNLInstTerm*>(
                [=](const SNLBitTerm* term) {
                  return iterm->getInstance()->getInstTerm(term);
                });
      }
    }
    // return all inputs of the instance
  }
  return iterm->getInstance()
      ->getModel()
      ->getBitTerms()
      .getSubCollection([](const SNLBitTerm* t) {
        return t->getDirection() != SNLTerm::Direction::Output;
      })
      .getTransformerCollection<SNLInstTerm*>([=](const SNLBitTerm* term) {
        return iterm->getInstance()->getInstTerm(term);
      });
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockRelatedInputs(
    SNLBitTerm* clock){
    if (isDB0SequentialPrimitive(clock->getDesign())) {
      return getDB0ClockRelatedInputs(clock);
    }
    if (hasMemoryInterface(clock->getDesign())) {
      return getMemoryClockRelatedInputs(clock);
    }
    GET_RELATED_OBJECTS(SNLBitTerm, clock, getDesign(), getClockRelatedInputs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockRelatedOutputs(
    SNLBitTerm* clock){
                                           if (isDB0SequentialPrimitive(clock->getDesign())) {
                                             return getDB0ClockRelatedOutputs(clock);
                                           }
                                           if (hasMemoryInterface(clock->getDesign())) {
                                             return getMemoryClockRelatedOutputs(clock);
                                           }
                                           GET_RELATED_OBJECTS(SNLBitTerm,
                                           clock,
                                           getDesign(),
                                           getClockRelatedOutputs_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getInputRelatedClocks(
    SNLBitTerm* input){
    if (isDB0SequentialPrimitive(input->getDesign())) {
      return getDB0InputRelatedClocks(input);
    }
    if (hasMemoryInterface(input->getDesign())) {
      return getMemoryInputRelatedClocks(input);
    }
    GET_RELATED_OBJECTS(SNLBitTerm, input, getDesign(), getInputRelatedClocks_)}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getOutputRelatedClocks(
    SNLBitTerm* output){
                                            if (isDB0SequentialPrimitive(output->getDesign())) {
                                              return getDB0OutputRelatedClocks(output);
                                            }
                                            if (hasMemoryInterface(output->getDesign())) {
                                              return getMemoryOutputRelatedClocks(output);
                                            }
                                            GET_RELATED_OBJECTS(SNLBitTerm,
                                            output,
                                            getDesign(),
                                            getOutputRelatedClocks_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getClockRelatedInputs(
    SNLInstTerm* clock){
                                            if (isDB0SequentialPrimitive(clock->getInstance()->getModel())) {
                                              return getDB0ClockRelatedInputs(clock);
                                            }
                                            if (hasMemoryInterface(clock->getInstance()->getModel())) {
                                              return getMemoryClockRelatedInputs(clock);
                                            }
                                            GET_RELATED_OBJECTS(SNLInstTerm,
                                            clock,
                                            getInstance() -> getModel(),
                                            getClockRelatedInputs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getClockRelatedOutputs(
    SNLInstTerm* clock){
                                            if (isDB0SequentialPrimitive(clock->getInstance()->getModel())) {
                                              return getDB0ClockRelatedOutputs(clock);
                                            }
                                            if (hasMemoryInterface(clock->getInstance()->getModel())) {
                                              return getMemoryClockRelatedOutputs(clock);
                                            }
                                            GET_RELATED_OBJECTS(SNLInstTerm,
                                            clock,
                                            getInstance()->getModel(),
                                            getClockRelatedOutputs_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getInputRelatedClocks(
    SNLInstTerm* input){
                                            if (isDB0SequentialPrimitive(input->getInstance()->getModel())) {
                                              return getDB0InputRelatedClocks(input);
                                            }
                                            if (hasMemoryInterface(input->getInstance()->getModel())) {
                                              return getMemoryInputRelatedClocks(input);
                                            }
                                            GET_RELATED_OBJECTS(SNLInstTerm,
                                            input,
                                            getInstance()->getModel(),
                                            getInputRelatedClocks_)}

NajaCollection<SNLInstTerm*> SNLDesignModeling::getOutputRelatedClocks(
    SNLInstTerm* output) {
  if (isDB0SequentialPrimitive(output->getInstance()->getModel())) {
    return getDB0OutputRelatedClocks(output);
  }
  if (hasMemoryInterface(output->getInstance()->getModel())) {
    return getMemoryOutputRelatedClocks(output);
  }
  GET_RELATED_OBJECTS(SNLInstTerm, output, getInstance()->getModel(),
                      getOutputRelatedClocks_)
}

void SNLDesignModeling::setMemoryInterface(
    SNLDesign* design,
    const MemoryInterface& memInterface) {
  if (!design->isPrimitive()) {
    throw NLException("Cannot add memory memInterface on non-primitive design");
  }
  validateMemoryInterfaceForDesign(design, memInterface);
  auto property = getOrCreateProperty(design, NO_PARAMETER);
  property->getModeling()->setMemoryInterface_(memInterface);
}

void SNLDesignModeling::setTermRole(
    SNLBitTerm* term, SNLTermRole role, SNLActiveLevel activeLevel) {
  if (!term || !term->getDesign()->isLeaf()) return;
  auto property = getOrCreateProperty(term->getDesign(), NO_PARAMETER);
  property->getModeling()->setTermRole_(term, role, activeLevel);
}

namespace {
SNLDesignModeling::SNLTermRole getMemoryTermRole(
    const SNLBitTerm* term,
    const SNLDesignModeling::MemoryInterface& memory) {
  using Role = SNLDesignModeling::SNLTermRole;
  using ResetMode = SNLDesignModeling::MemoryResetMode;
  if (term == memory.clock) return Role::Clock;
  if (term == memory.reset &&
      (memory.resetMode == ResetMode::AsyncLow ||
       memory.resetMode == ResetMode::AsyncHigh)) {
    return Role::AsyncReset;
  }
  auto contains = [term](const SNLDesignModeling::BitTerms& terms) {
    return std::find(terms.begin(), terms.end(), term) != terms.end();
  };
  for (const auto& port : memory.readPorts) {
    if (contains(port.address)) return Role::MemoryReadAddress;
    if (contains(port.data)) return Role::MemoryReadData;
    if (contains(port.enables)) return Role::Enable;
  }
  for (const auto& port : memory.writePorts) {
    if (contains(port.address)) return Role::MemoryWriteAddress;
    if (contains(port.data)) return Role::MemoryWriteData;
    if (contains(port.mask) || contains(port.enables)) {
      return Role::MemoryWriteEnable;
    }
  }
  return Role::Other;
}
}

SNLDesignModeling::SNLTermRole SNLDesignModeling::getTermRole(
    const SNLBitTerm* term) {
  if (!term) return SNLTermRole::Other;
  auto* design = term->getDesign();
  if (hasMemoryInterface(design)) {
    return getMemoryTermRole(term, getMemoryInterface(design));
  }
  if (auto property = getProperty(design)) {
    return property->getModeling()->getTermRole_(term).role;
  }
  return SNLTermRole::Other;
}

SNLDesignModeling::SNLTermRole SNLDesignModeling::getTermRole(
    const SNLInstTerm* term) {
  return term ? getTermRole(term->getBitTerm()) : SNLTermRole::Other;
}

SNLDesignModeling::SNLActiveLevel SNLDesignModeling::getResetActiveLevel(
    const SNLBitTerm* term) {
  auto role = getTermRole(term);
  if (role != SNLTermRole::AsyncReset && role != SNLTermRole::AsyncSet &&
      role != SNLTermRole::SyncReset && role != SNLTermRole::SyncSet) {
    return SNLActiveLevel::NA;
  }
  if (term && hasMemoryInterface(term->getDesign())) {
    auto memory = getMemoryInterface(term->getDesign());
    if (term == memory.reset) {
      return memory.resetMode == MemoryResetMode::AsyncLow
          ? SNLActiveLevel::Low : SNLActiveLevel::High;
    }
  }
  if (term) {
    if (auto property = getProperty(term->getDesign())) {
      return property->getModeling()->getTermRole_(term).activeLevel;
    }
  }
  return SNLActiveLevel::NA; // LCOV_EXCL_LINE defensive inconsistent role fallback
}

SNLDesignModeling::SNLActiveLevel SNLDesignModeling::getResetActiveLevel(
    const SNLInstTerm* term) {
  return term ? getResetActiveLevel(term->getBitTerm()) : SNLActiveLevel::NA;
}

#define DEFINE_TERM_ROLE_PREDICATE(NAME, ROLE)                    \
  bool SNLDesignModeling::NAME(const SNLBitTerm* term) {          \
    return getTermRole(term) == SNLTermRole::ROLE;                \
  }                                                               \
  bool SNLDesignModeling::NAME(const SNLInstTerm* term) {         \
    return getTermRole(term) == SNLTermRole::ROLE;                \
  }

DEFINE_TERM_ROLE_PREDICATE(isClock, Clock)
DEFINE_TERM_ROLE_PREDICATE(isAsyncReset, AsyncReset)
DEFINE_TERM_ROLE_PREDICATE(isAsyncSet, AsyncSet)
DEFINE_TERM_ROLE_PREDICATE(isSyncReset, SyncReset)
DEFINE_TERM_ROLE_PREDICATE(isSyncSet, SyncSet)
DEFINE_TERM_ROLE_PREDICATE(isEnable, Enable)
#undef DEFINE_TERM_ROLE_PREDICATE

bool SNLDesignModeling::isReset(const SNLBitTerm* term) {
  return isAsyncReset(term) || isSyncReset(term);
}
bool SNLDesignModeling::isReset(const SNLInstTerm* term) {
  return isAsyncReset(term) || isSyncReset(term);
}
bool SNLDesignModeling::isDataInput(const SNLBitTerm* term) {
  auto role = getTermRole(term);
  return role == SNLTermRole::DataInput || role == SNLTermRole::MemoryWriteData;
}
bool SNLDesignModeling::isDataInput(const SNLInstTerm* term) {
  return term && isDataInput(term->getBitTerm());
}
bool SNLDesignModeling::isDataOutput(const SNLBitTerm* term) {
  auto role = getTermRole(term);
  return role == SNLTermRole::DataOutput || role == SNLTermRole::MemoryReadData;
}
bool SNLDesignModeling::isDataOutput(const SNLInstTerm* term) {
  return term && isDataOutput(term->getBitTerm());
}

namespace {
NajaCollection<SNLBitTerm*> getTermsWithRole(
    const SNLDesign* design, SNLDesignModeling::SNLTermRole role) {
  if (!design) return NajaCollection<SNLBitTerm*>();
  if (SNLDesignModeling::hasMemoryInterface(design)) {
    auto memory = SNLDesignModeling::getMemoryInterface(design);
    return design->getBitTerms().getSubCollection(
        [role, memory = std::move(memory)](const SNLBitTerm* term) {
          return getMemoryTermRole(term, memory) == role;
        });
  }
  return design->getBitTerms().getSubCollection([role](const SNLBitTerm* term) {
    return SNLDesignModeling::getTermRole(term) == role;
  });
}
}

NajaCollection<SNLBitTerm*> SNLDesignModeling::getClockTerms(const SNLDesign* design) {
  return getTermsWithRole(design, SNLTermRole::Clock);
}
NajaCollection<SNLBitTerm*> SNLDesignModeling::getAsyncResetTerms(const SNLDesign* design) {
  return getTermsWithRole(design, SNLTermRole::AsyncReset);
}
NajaCollection<SNLBitTerm*> SNLDesignModeling::getAsyncSetTerms(const SNLDesign* design) {
  return getTermsWithRole(design, SNLTermRole::AsyncSet);
}
NajaCollection<SNLBitTerm*> SNLDesignModeling::getSyncResetTerms(const SNLDesign* design) {
  return getTermsWithRole(design, SNLTermRole::SyncReset);
}
NajaCollection<SNLBitTerm*> SNLDesignModeling::getSyncSetTerms(const SNLDesign* design) {
  return getTermsWithRole(design, SNLTermRole::SyncSet);
}
NajaCollection<SNLBitTerm*> SNLDesignModeling::getDataInputTerms(const SNLDesign* design) {
  if (!design) return NajaCollection<SNLBitTerm*>();
  if (hasMemoryInterface(design)) {
    auto memory = getMemoryInterface(design);
    return design->getBitTerms().getSubCollection(
        [memory = std::move(memory)](const SNLBitTerm* term) {
          return getMemoryTermRole(term, memory) == SNLTermRole::MemoryWriteData;
        });
  }
  return design->getBitTerms().getSubCollection([](const SNLBitTerm* term) {
    return SNLDesignModeling::isDataInput(term);
  });
}
NajaCollection<SNLBitTerm*> SNLDesignModeling::getOutputTerms(const SNLDesign* design) {
  if (!design) return NajaCollection<SNLBitTerm*>();
  if (hasMemoryInterface(design)) {
    auto memory = getMemoryInterface(design);
    return design->getBitTerms().getSubCollection(
        [memory = std::move(memory)](const SNLBitTerm* term) {
          return getMemoryTermRole(term, memory) == SNLTermRole::MemoryReadData;
        });
  }
  return design->getBitTerms().getSubCollection([](const SNLBitTerm* term) {
    return SNLDesignModeling::isDataOutput(term);
  });
}

bool SNLDesignModeling::hasMemoryInterface(const SNLDesign* design) {
  if (!design) {
    return false;
  }
  if (auto property = getProperty(design)) {
    if (property->getModeling()->hasMemoryInterface_()) {
      return true;
    }
  }
  return NLDB0::isMemory(design);
}

SNLDesignModeling::MemoryInterface SNLDesignModeling::getMemoryInterface(
    const SNLDesign* design) {
  if (!design) {
    throw NLException("SNLDesignModeling::getMemoryInterface: null design");
  }
  if (auto property = getProperty(design)) {
    if (property->getModeling()->hasMemoryInterface_()) {
      return property->getModeling()->getMemoryInterface_();
    }
  }
  if (NLDB0::isMemory(design)) {
    return buildDB0MemoryInterface(design);
  }
  throw NLException(
      "SNLDesignModeling::getMemoryInterface: design has no memory memInterface");
}

SNLDesignModeling::MemoryInterface SNLDesignModeling::getMemoryInterface(
    const SNLInstance* instance) {
  if (!instance) {
    throw NLException("SNLDesignModeling::getMemoryInterface: null instance");
  }
  auto memInterface = getMemoryInterface(instance->getModel());
  if (NLDB0::isMemory(instance->getModel())) {
    const auto signature = NLDB0::getMemorySignature(instance);
    memInterface.width = signature.width;
    memInterface.depth = signature.depth;
    memInterface.abits = signature.abits;
    memInterface.resetMode = convertMemoryResetMode(signature.resetMode);
  }
  // Elaboration may instantiate a generic memory primitive with only a subset
  // of ports connected. Return the effective instance memInterface, not the full
  // primitive declaration, so clients only model observable/driveable ports.
  if (!isConnectedInstanceBitTerm(instance, memInterface.clock)) {
    memInterface.clock = nullptr;
  }
  if (memInterface.reset && !isConnectedInstanceBitTerm(instance, memInterface.reset)) {
    memInterface.reset = nullptr;
  }
  std::vector<MemoryReadPort> connectedReadPorts;
  connectedReadPorts.reserve(memInterface.readPorts.size());
  for (const auto& readPort : memInterface.readPorts) {
    if (isConnectedReadPort(instance, readPort)) {
      connectedReadPorts.push_back(readPort);
    }
  }
  memInterface.readPorts = std::move(connectedReadPorts);
  std::vector<MemoryWritePort> connectedWritePorts;
  connectedWritePorts.reserve(memInterface.writePorts.size());
  for (const auto& writePort : memInterface.writePorts) {
    if (isConnectedWritePort(instance, writePort)) {
      connectedWritePorts.push_back(writePort);
    }
  }
  memInterface.writePorts = std::move(connectedWritePorts);
  return memInterface;
}

void SNLDesignModeling::setTruthTable(SNLDesign* design,
                                      const SNLTruthTable& truthTable) {
  if (!design->isPrimitive()) {
    throw NLException("Cannot add truth table on non-primitive design");
  }
  // Check no truth table already exists
  if (getTruthTableProperty(design)) {
    throw NLException("Design already has a Truth Table");
  }
  const auto& outputs =
      design->getBitTerms().getSubCollection([](const SNLBitTerm* t) {
        return t->getDirection() == SNLTerm::Direction::Output;
      });
  if (outputs.size() != 1) {
    std::ostringstream reason;
    reason << "cannot add truth table on Design <"
           << design->getName().getString() << "> that has <" << outputs.size()
           << "> outputs";
    throw NLException(reason.str());
  }
  createTruthTableProperty(design, truthTable);
}

void SNLDesignModeling::setTruthTables(
    SNLDesign* design,
    const std::vector<SNLTruthTable>& truthTables) {
  if (!design->isPrimitive()) {
    throw NLException("Cannot add truth table on non-primitive design");
  }
  // Check no truth table already exists
  if (getTruthTableProperty(design)) {
    throw NLException("Design already has a Truth Table");
  }
  const auto& outputs =
      design->getBitTerms().getSubCollection([](const SNLBitTerm* t) {
        return t->getDirection() != SNLTerm::Direction::Input;
      });
  if (outputs.size() != truthTables.size()) {
    std::ostringstream reason;
    reason << "cannot add truth tables on Design <"
           << design->getName().getString() << "> that has <" << outputs.size()
           << "> outputs, but provided <" << truthTables.size()
           << "> truth tables";
    throw NLException(reason.str());
  }
  createTruthTableProperty(design, truthTables);
}

bool SNLDesignModeling::areDependenciesDefined(const SNLBitTerm* term) {
  auto design = term->getDesign();
  if (SNLDesignModeling::getTruthTableCount(design) == 0) {
    return false;
  }

  if (term->getDirection() != SNLTerm::Direction::Input) {
    SNLTruthTable tt =
        SNLDesignModeling::getTruthTable(design, term->getOrderID());
    return true;
  }

  size_t flatID = term->getOrderID();
  bool foundExplicitDependencies = false;
  for (const auto& candidate : design->getBitTerms()) {
    if (candidate->getDirection() == SNLTerm::Direction::Input) {
      continue;
    }
    SNLTruthTable tt =
        SNLDesignModeling::getTruthTable(design, candidate->getOrderID());
    const auto& deps = tt.getDependencies();
    if (naja::NL::NLBitDependencies::countBitsForVector(deps) == 0) {
      return false;
    }
    foundExplicitDependencies = true;
    auto decodedDeps = naja::NL::NLBitDependencies::decodeBits(deps);
    if (std::find(decodedDeps.begin(), decodedDeps.end(), flatID) !=
        decodedDeps.end()) {
      return true;
    }
  }
  return foundExplicitDependencies;
}

size_t SNLDesignModeling::getTruthTableCount(const SNLDesign* design) {
  if (NLDB0::isDB0Primitive(design)) {
    if (isDB0SequentialPrimitive(design) || NLDB0::isMemory(design) ||
        NLDB0::isConst(design)) {
      return 0;
    }
    if (NLDB0::isMux2(design) || NLDB0::isTableSelect(design)) {
      size_t tableCount = 0;
      for (const auto* term : design->getBitTerms()) {
        if (term->getDirection() != SNLTerm::Direction::Input) {
          ++tableCount;
        }
      }
      return tableCount;
    }
    size_t tableCount = 0;
    for (const auto* term : design->getBitTerms()) {
      if (term->getDirection() == SNLTerm::Direction::Input) {
        continue;
      }
      auto tt = getTruthTable(design, term->getOrderID());
      if (!tt.isNull()) {
        ++tableCount;
      }
    }
    return tableCount;
  }
  auto property = getTruthTableProperty(design);
  size_t tableIdx = 0;
  if (property != nullptr) {
    size_t valIdx = 0;
    size_t total = property->getValues().size();

    if (total == 0) {
      return 0;
    }

    while (valIdx < total) {
      uint32_t nInputs =
          static_cast<uint32_t>(property->getUInt64Value(valIdx));
      advanceTruthTableValueIndex(design, valIdx, nInputs);

      // LCOV_EXCL_START
      if (valIdx >= total + 1) {
        std::ostringstream reason;
        std::string result;
        for (size_t i = 0; i < total; ++i)
          result += std::to_string(property->getUInt64Value(i)) + " ";
        reason << "Maldformed truth table for design <"
               << design->getName().getString() << "> " << result
               << "\nWith valIdx " << valIdx << " and total " << total;
        throw NLException(reason.str());
      }
      // LCOV_EXCL_STOP

      ++tableIdx;
    }
  }
  return tableIdx;
}

SNLTruthTable SNLDesignModeling::getTruthTable(const SNLDesign* design) {
  if (NLDB0::isDB0Primitive(design)) {
    if (isDB0SequentialPrimitive(design) || NLDB0::isMemory(design) ||
        NLDB0::isConst(design)) {
      return SNLTruthTable();
    }
    if (NLDB0::isTableSelect(design)) {
      const SNLBitTerm* outputTerm = nullptr;
      for (auto* term : design->getBitTerms()) {
        if (term->getDirection() != SNLTerm::Direction::Input) {
          if (outputTerm != nullptr) {
            throw NLException(
                "SNLDesignModeling::getTruthTable: table select has per-output truth tables");
          }
          outputTerm = term;
        }
      }
      return outputTerm ? NLDB0::getTableSelectTruthTable(
                              design, outputTerm->getOrderID())
                        : SNLTruthTable();
    }
    return NLDB0::getPrimitiveTruthTable(design);
  }
  auto property = getTruthTableProperty(design);
  if (property) {
    size_t tableSize = property->getValues().size() - 1;
    uint64_t declaredInputs = property->getUInt64Value(0);
    uint64_t num_bits = 1u << declaredInputs;
    size_t expectedChunks =
        TT_NCHUNKS_FROM_BITS(num_bits) + getDependencyChunkCount(design);
    if (expectedChunks != tableSize) {
      std::ostringstream reason;
      reason << "Truth table size " << tableSize
             << " does not match number of chunks " << expectedChunks
             << " which suggests per output functionality";
      throw NLException(reason.str());
    }

    // multi‐chunk (>64‐bit) table?
    if (property->getValues().size() > 3) {
      uint32_t numInputs = static_cast<uint32_t>(declaredInputs);
      uint32_t nBits = 1u << numInputs;
      // LCOV_EXCL_START
      if (nBits <= 64) {
        std::ostringstream reason;
        reason << "Truth table size " << nBits << " is not larger than 64 bits";
        throw NLException(reason.str());
      }
      // LCOV_EXCL_STOP

      size_t nChunks = TT_NCHUNKS_FROM_BITS(nBits);
      size_t bitsIdx = 1;  // skip the size entry
      std::vector<bool> bits(nBits, false);
      TT_FILL_BITS(bits, nBits, property, bitsIdx);

      size_t depsIdx = bitsIdx + nChunks;
      std::vector<uint64_t> deps;
      size_t nDeps = getDependencyChunkCount(design);
      for (size_t i = depsIdx; i < depsIdx + nDeps; ++i) {
        if (i < property->getValues().size()) {
          deps.push_back(property->getUInt64Value(i));
        } else {
          // LCOV_EXCL_START
          std::ostringstream reason;
          reason << "Truth table size " << property->getValues().size()
                 << " is smaller then requested index " << i;
          throw NLException(reason.str());
          // LCOV_EXCL_STOP
        }
      }
      return SNLTruthTable(numInputs, bits,
                           numInputs == 0 ? std::vector<uint64_t>{} : deps);
    }

    // single‐chunk
    if (declaredInputs <= 6) {
      std::vector<uint64_t> deps;
      size_t nDeps = getDependencyChunkCount(design);
      for (size_t i = 2; i < 2 + nDeps; ++i) {
        if (i < property->getValues().size()) {
          deps.push_back(property->getUInt64Value(i));
        }
      }
      return SNLTruthTable(static_cast<uint32_t>(declaredInputs),
                           property->getUInt64Value(1),
                           declaredInputs == 0 ? std::vector<uint64_t>{} : deps);
    } else {
      // LCOV_EXCL_START
      std::ostringstream reason;
      reason << "Truth table size " << declaredInputs
             << " is larger than 64 bits";
      throw NLException(reason.str());
      // LCOV_EXCL_STOP
    }
  }
  return SNLTruthTable();
}

SNLTruthTable SNLDesignModeling::getTruthTable(const SNLDesign* design,
                                               size_t flatTermID) {
  auto resolveOutputIndex = [&](size_t orderID,
                                NLID::DesignObjectID& outputID,
                                NLID::DesignObjectID& outputCount) {
    outputID = 0;
    outputCount = 0;
    bool found = false;
    size_t bitTermIdx = 0;
    for (const auto& term : design->getBitTerms()) {
      if (term->getDirection() != SNLTerm::Direction::Input) {
        if (bitTermIdx == orderID) {
          outputID = outputCount;
          found = true;
        }
        ++outputCount;
      }
      ++bitTermIdx;
    }
    return found; };

  if (NLDB0::isDB0Primitive(design) &&
      (isDB0SequentialPrimitive(design) || NLDB0::isMemory(design) ||
       NLDB0::isConst(design))) {
    return SNLTruthTable();
  }
  auto property = getTruthTableProperty(design);
  NLID::DesignObjectID outputID = 0;
  NLID::DesignObjectID outputCount = 0;
  const bool isOutputTerm = resolveOutputIndex(flatTermID, outputID, outputCount);
  if (NLDB0::isDB0Primitive(design)) {
    // throw an error in case outputIndex is not 1
    // check if FA design
    if (NLDB0::isFA(design)) {
      if (NLDB0::getFAOutputS()->getID() == flatTermID) {
        return NLDB0::getFASumTruthTable();
      } else if (NLDB0::getFAOutputCO()->getID() == flatTermID) {
        return NLDB0::getFACoutTruthTable();
      } else {
        std::ostringstream reason;
        reason << "Term ID " << flatTermID << " is not an output in FA design <"
               << design->getName().getString() << ">";
        throw NLException(reason.str());
      }
    }
    if (NLDB0::isMux2(design)) {
      if (!isOutputTerm) {
        std::ostringstream reason;
        reason << "Term ID " << flatTermID
               << " is not an output in mux2 design <"
               << design->getName().getString() << ">";
        throw NLException(reason.str());
      }
      return NLDB0::getPrimitiveTruthTable(design);
    } else if (NLDB0::isTableSelect(design)) {
      if (!isOutputTerm) {
        std::ostringstream reason;
        reason << "Term ID " << flatTermID
               << " is not an output in table select design <"
               << design->getName().getString() << ">";
        throw NLException(reason.str());
      }
      return NLDB0::getTableSelectTruthTable(design, flatTermID);
    } else {
      if (outputCount != 1) {
        std::ostringstream reason;
        reason << "Design <" << design->getName().getString()
               << "> is a DB0 primitive but has " << outputCount
               << " outputs instead of 1";
        throw NLException(reason.str());
      }
      return NLDB0::getPrimitiveTruthTable(design);
    }
  }
  if (!isOutputTerm) {
    std::ostringstream reason;
    reason << "Term ID " << flatTermID << " is not an output in design <"
           << design->getName().getString() << ">";
    throw NLException(reason.str());
  }

  if (property) {
    if (!NLDB0::isDB0Primitive(design) && getTruthTableCount(design) == 1) {
      return getTruthTable(design);
    }

    size_t tableIdx = 0;
    size_t valIdx = 0;
    size_t total = property->getValues().size();

    while (true) {
      // LCOV_EXCL_START
      if (valIdx >= total + 1) {
        std::ostringstream reason;
        reason << "Output ID " << outputID << " is out of range for design <"
               << design->getName().getString() << ">";
        throw NLException(reason.str());
      }
      // LCOV_EXCL_STOP

      if (tableIdx == outputID) {
        break;
      }
      uint32_t nInputs =
          static_cast<uint32_t>(property->getUInt64Value(valIdx));
      advanceTruthTableValueIndex(design, valIdx, nInputs);
      ++tableIdx;
    }

    uint64_t declaredInputs = property->getUInt64Value(valIdx);

    // single‐chunk fast‐path?
    if (declaredInputs <= 6) {
      std::vector<uint64_t> deps;
      size_t nDeps = getDependencyChunkCount(design);
      for (size_t i = 0; i < nDeps; ++i) {
        deps.push_back(property->getUInt64Value(valIdx + 2 + i));
      }
      return SNLTruthTable(static_cast<uint32_t>(declaredInputs),
                           property->getUInt64Value(valIdx + 1),
                           declaredInputs == 0 ? std::vector<uint64_t>{} : deps);
    }

    // multi‐chunk
    uint32_t numInputs = static_cast<uint32_t>(declaredInputs);
    uint32_t nBits = 1u << numInputs;
    size_t nChunks = TT_NCHUNKS_FROM_BITS(nBits);
    size_t bitsIdx = valIdx + 1;
    std::vector<bool> bits(nBits, false);
    TT_FILL_BITS(bits, nBits, property, bitsIdx);

    std::vector<uint64_t> deps;
    size_t nDeps = getDependencyChunkCount(design);
    for (size_t i = bitsIdx + nChunks; i < bitsIdx + nChunks + nDeps; ++i) {
      if (i < total) {
        deps.push_back(property->getUInt64Value(i));
      } else {
        // LCOV_EXCL_START
        std::ostringstream reason;
        reason << "Truth table size " << total
               << " is smaller then requested index " << i;
        throw NLException(reason.str());
        // LCOV_EXCL_STOP
      }
    }
    return SNLTruthTable(numInputs, bits,
                         numInputs == 0 ? std::vector<uint64_t>{} : deps);
  }

  return SNLTruthTable();
}

bool SNLDesignModeling::hasModeling(const SNLDesign* design) {
  auto property = getProperty(design);
  if (property) {
    return true;
  } else {
    return getTruthTableProperty(design) || hasMemoryInterface(design);
  }
}

bool SNLDesignModeling::isSequential(const SNLDesign* design) {
  if (isDB0SequentialPrimitive(design)) {
    return true;
  }
  if (hasMemoryInterface(design)) {
    return true;
  }
  auto property = getProperty(design);
  if (property) {
    auto modeling = property->getModeling();
    const auto arcs = modeling->getTimingArcs();
    return not arcs->inputToClockArcs_.empty() or
           not arcs->clockToInputArcs_.empty();
  }
  return false;
}

bool SNLDesignModeling::compareInstanceModeling(
    const SNLInstance* instance1,
    const SNLInstance* instance2,
    std::string& reason) {
  const auto* state1 = SNLInstanceStateProperty::get(instance1);
  const auto* state2 = SNLInstanceStateProperty::get(instance2);
  if ((state1 == nullptr) != (state2 == nullptr)) {
    reason = "instance state property mismatch";
    return false;
  }
  if (state1 && (state1->initValue_ != state2->initValue_ ||
                 state1->resetValue_ != state2->resetValue_)) {
    reason = "instance state value mismatch";
    return false;
  }

  const auto* driver1 = SNLConstantDriverProperty::get(instance1);
  const auto* driver2 = SNLConstantDriverProperty::get(instance2);
  if ((driver1 == nullptr) != (driver2 == nullptr)) {
    reason = "constant driver property mismatch";
    return false;
  }
  if (driver1 && driver1->driver_ != driver2->driver_) {
    reason = "constant driver value mismatch";
    return false;
  }
  return true;
}

void SNLDesignModeling::cloneInstanceModeling(
    const SNLInstance* source,
    SNLInstance* target) {
  if (const auto* state = SNLInstanceStateProperty::get(source)) {
    auto* clone = SNLInstanceStateProperty::create(target);
    clone->initValue_ = state->initValue_;
    clone->resetValue_ = state->resetValue_;
  }
  if (const auto* driver = SNLConstantDriverProperty::get(source)) {
    SNLConstantDriverProperty::create(target, driver->driver_);
  }
}

void SNLDesignModeling::validateInstanceModelingForModel(
    const SNLInstance* instance,
    const SNLDesign* model) {
  if (const auto* state = SNLInstanceStateProperty::get(instance)) {
    if (state->initValue_ &&
        state->initValue_->getWidth() != getInitializationWidth(instance, model)) {
      throw NLException(
        "SNLInstance::setModel error: initialization is incompatible with new model");
    }
    if (state->resetValue_) {
      if (!hasMemoryReset(instance, model) ||
          state->resetValue_->getWidth() != getInitializationWidth(instance, model)) {
        throw NLException(
          "SNLInstance::setModel error: reset value is incompatible with new model");
      }
    }
  }

  if (const auto* driver = SNLConstantDriverProperty::get(instance)) {
    auto* output = NLDB0::getConstOutput(model);
    if (!output || driver->driver_.value.getWidth() != output->getWidth()) {
      throw NLException(
        "SNLInstance::setModel error: constant driver is incompatible with new model");
    }
  }
}

bool SNLDesignModeling::hasInit(const SNLInstance* instance) {
  const auto* property = SNLInstanceStateProperty::get(instance);
  return property && property->initValue_.has_value();
}

void SNLDesignModeling::setInitValue(
    SNLInstance* instance,
    const NLLogicVector& value) {
  if (!instance || value.empty()) {
    throw NLException("SNLDesignModeling::setInitValue: invalid arguments");
  }
  const auto* model = instance->getModel();
  size_t expectedWidth = 0;
  try {
    expectedWidth = getInitializationWidth(instance, model);
  } catch (const NLException&) {
    throw NLException("SNLDesignModeling::setInitValue: instance is not a DB0 state element");
  }
  if (value.getWidth() != expectedWidth) {
    throw NLException("SNLDesignModeling::setInitValue: value width mismatch");
  }
  SNLInstanceStateProperty::getOrCreate(instance)->initValue_ = value;
}

std::optional<NLLogicVector> SNLDesignModeling::getInitValue(
    const SNLInstance* instance) {
  if (!hasInit(instance)) {
    return std::nullopt;
  }
  return SNLInstanceStateProperty::get(instance)->initValue_;
}

void SNLDesignModeling::setResetValue(
    SNLInstance* instance,
    const NLLogicVector& value) {
  if (!instance || !NLDB0::isMemory(instance->getModel()) || value.empty()) {
    throw NLException("SNLDesignModeling::setResetValue: invalid arguments");
  }
  const auto signature = NLDB0::getMemorySignature(instance);
  if (signature.resetMode == NLDB0::MemoryResetMode::None ||
      value.getWidth() != signature.width * signature.depth) {
    throw NLException("SNLDesignModeling::setResetValue: value width or reset mode mismatch");
  }
  SNLInstanceStateProperty::getOrCreate(instance)->resetValue_ = value;
}

std::optional<NLLogicVector> SNLDesignModeling::getResetValue(
    const SNLInstance* instance) {
  if (!instance || !NLDB0::isMemory(instance->getModel())) {
    return std::nullopt;
  }
  const auto signature = NLDB0::getMemorySignature(instance);
  if (signature.resetMode == NLDB0::MemoryResetMode::None) {
    return std::nullopt;
  }
  if (const auto* property = SNLInstanceStateProperty::get(instance);
      property && property->resetValue_) {
    return property->resetValue_;
  }
  return NLLogicVector::filled(
    signature.width * signature.depth, NLLogicValue::Zero);
}

SNLInstance* SNLDesignModeling::createConstantDriver(
    SNLDesign* design,
    const NLLogicVector& value,
    NLConstantDriverKind kind,
    const NLName& name) {
  if (!design || value.empty()) {
    throw NLException("SNLDesignModeling::createConstantDriver: invalid arguments");
  }
  if (kind == NLConstantDriverKind::Supply &&
      !value.isAll(NLLogicValue::Zero) &&
      !value.isAll(NLLogicValue::One)) {
    throw NLException(
      "SNLDesignModeling::createConstantDriver: supply must be uniform zero or one");
  }
  auto* model = NLDB0::getOrCreateConst(value.getWidth());
  auto* instance = SNLInstance::create(design, model, name);
  setConstantDriver(instance, value, kind);
  return instance;
}

SNLInstance* SNLDesignModeling::createConstantDriver(
    SNLNet* net,
    NLLogicValue value,
    NLConstantDriverKind kind,
    const NLName& name) {
  if (!net) {
    throw NLException("SNLDesignModeling::createConstantDriver: invalid net");
  }
  auto* instance = createConstantDriver(
    net->getDesign(), NLLogicVector::filled(net->getWidth(), value), kind, name);
  instance->setTermNet(NLDB0::getConstOutput(instance->getModel()), net);
  return instance;
}

void SNLDesignModeling::setConstantDriver(
    SNLInstance* instance,
    const NLLogicVector& value,
    NLConstantDriverKind kind) {
  if (!isConstantDriver(instance) || value.empty()) {
    throw NLException("SNLDesignModeling::setConstantDriver: invalid arguments");
  }
  auto* output = NLDB0::getConstOutput(instance->getModel());
  if (!output || value.getWidth() != output->getWidth()) {
    throw NLException("SNLDesignModeling::setConstantDriver: value width mismatch");
  }
  if (kind == NLConstantDriverKind::Supply &&
      !value.isAll(NLLogicValue::Zero) &&
      !value.isAll(NLLogicValue::One)) {
    throw NLException(
      "SNLDesignModeling::setConstantDriver: supply must be uniform zero or one");
  }
  const NLConstantDriver driver {value, kind};
  if (auto* property = SNLConstantDriverProperty::get(instance)) {
    property->driver_ = driver;
  } else {
    SNLConstantDriverProperty::create(instance, driver);
  }
}

bool SNLDesignModeling::isConstantDriver(const SNLInstance* instance) {
  return instance && NLDB0::isConst(instance->getModel());
}

NLConstantDriverKind SNLDesignModeling::getConstantDriverKind(
    const SNLInstance* instance) {
  if (!isConstantDriver(instance)) {
    throw NLException("SNLDesignModeling::getConstantDriverKind: not a constant driver");
  }
  if (const auto* property = SNLConstantDriverProperty::get(instance)) {
    return property->driver_.kind;
  }
  throw NLException("SNLDesignModeling::getConstantDriverKind: missing typed driver value");
}

NLLogicVector SNLDesignModeling::getConstantDriverValue(
    const SNLInstance* instance) {
  if (!isConstantDriver(instance)) {
    throw NLException("SNLDesignModeling::getConstantDriverValue: not a constant driver");
  }
  const auto* property = SNLConstantDriverProperty::get(instance);
  if (!property) {
    throw NLException("SNLDesignModeling::getConstantDriverValue: missing typed driver value");
  }
  const auto& value = property->driver_.value;
  const auto* output = NLDB0::getConstOutput(instance->getModel());
  const size_t expectedWidth = output ? output->getWidth() : 0;
  if (value.getWidth() != expectedWidth) {
    throw NLException("SNLDesignModeling::getConstantDriverValue: VALUE width mismatch");
  }
  const auto kind = getConstantDriverKind(instance);
  if (kind == NLConstantDriverKind::Supply &&
      !value.isAll(NLLogicValue::Zero) &&
      !value.isAll(NLLogicValue::One)) {
    throw NLException("SNLDesignModeling::getConstantDriverValue: invalid supply value");
  }
  return value;
}

std::optional<NLLogicValue> SNLDesignModeling::getConstantValue(
    const SNLBitNet* net) {
  if (!net) {
    return std::nullopt;
  }
  std::optional<NLLogicValue> resolved;
  for (auto* term: net->getBitTerms()) {
    if (term->getDirection() != SNLTerm::Direction::Output) {
      return std::nullopt;
    }
  }
  for (auto* instTerm: net->getInstTerms()) {
    if (instTerm->getBitTerm()->getDirection() != SNLTerm::Direction::Output) {
      continue;
    }
    auto* instance = instTerm->getInstance();
    NLLogicValue candidate;
    if (isConstantDriver(instance)) {
      auto value = getConstantDriverValue(instance);
      size_t bit = 0;
      if (auto* busBit = dynamic_cast<SNLBusTermBit*>(instTerm->getBitTerm())) {
        bit = static_cast<size_t>(busBit->getBit());
      }
      if (bit >= value.getWidth()) {
        return std::nullopt; // LCOV_EXCL_LINE
      }
      candidate = value.getBit(bit);
    } else {
      // Hierarchical outputs are resolved by equipotential traversal in their
      // model.  Only leaf models can directly describe a legacy constant.
      if (!instance->isLeaf()) {
        return std::nullopt;
      }
      try {
        if (isConst0(instance->getModel())) {
          candidate = NLLogicValue::Zero;
        } else if (isConst1(instance->getModel())) {
          candidate = NLLogicValue::One;
        } else {
          return std::nullopt;
        }
      } catch (const NLException&) {
        return std::nullopt;
      }
    }
    if (resolved && *resolved != candidate) {
      return std::nullopt;
    }
    resolved = candidate;
  }
  return resolved;
}

bool SNLDesignModeling::isConstant(const SNLNet* net) {
  if (!net) return false;
  bool hasBits = false;
  for (auto* bit: net->getBits()) {
    hasBits = true;
    if (!getConstantValue(bit)) return false;
  }
  return hasBits;
}

bool SNLDesignModeling::isConstant(const SNLNet* net, NLLogicValue value) {
  if (!net) return false;
  bool hasBits = false;
  for (auto* bit: net->getBits()) {
    hasBits = true;
    if (getConstantValue(bit) != value) return false;
  }
  return hasBits;
}

bool SNLDesignModeling::isConst0(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  // return false if number of values large than 2
  if (property && property->getValues().size() > 3) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() && truthTable == SNLTruthTable::Logic0();
}

bool SNLDesignModeling::isConst1(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  // return false if number of values large than 3
  if (property && property->getValues().size() > 3) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() && truthTable == SNLTruthTable::Logic1();
}

bool SNLDesignModeling::isConst(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  // return false if number of values large than 3
  if (property && property->getValues().size() > 3) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() && (truthTable == SNLTruthTable::Logic0() ||
                                        truthTable == SNLTruthTable::Logic1());
}

bool SNLDesignModeling::isInv(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  // return false if number of values large than 3
  if (property && property->getValues().size() > 3) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() && !truthTable.isGeneric() &&
         truthTable.size() == 1 && truthTable.bits().size() == 2 &&
         static_cast<uint64_t>(truthTable.bits()) == 0b01;
}

bool SNLDesignModeling::isBuf(const SNLDesign* design) {
  auto property = getTruthTableProperty(design);
  // return false if number of values large than 3
  if (property && property->getValues().size() > 3) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() && !truthTable.isGeneric() &&
         truthTable.size() == 1 && truthTable.bits().size() == 2 &&
         static_cast<uint64_t>(truthTable.bits()) == 0b10;
}

namespace {
bool isTruthTableFamily(
    const SNLDesign* design,
    bool (SNLTruthTable::*predicate)() const) {
  try {
    auto truthTable = SNLDesignModeling::getTruthTable(design);
    return truthTable.isInitialized() && (truthTable.*predicate)();
  } catch (const NLException&) {
    return false;
  }
}
}  // namespace

bool SNLDesignModeling::isAnd(const SNLDesign* design) {
  return isTruthTableFamily(design, &SNLTruthTable::isAnd);
}

bool SNLDesignModeling::isNand(const SNLDesign* design) {
  return isTruthTableFamily(design, &SNLTruthTable::isNand);
}

bool SNLDesignModeling::isOr(const SNLDesign* design) {
  return isTruthTableFamily(design, &SNLTruthTable::isOr);
}

bool SNLDesignModeling::isNor(const SNLDesign* design) {
  return isTruthTableFamily(design, &SNLTruthTable::isNor);
}

bool SNLDesignModeling::isXor(const SNLDesign* design) {
  return isTruthTableFamily(design, &SNLTruthTable::isXor);
}

bool SNLDesignModeling::isXnor(const SNLDesign* design) {
  return isTruthTableFamily(design, &SNLTruthTable::isXnor);
}

bool SNLDesignModeling::isMux(const SNLDesign* design) {
  return NLDB0::isMux2(design) ||
         isTruthTableFamily(design, &SNLTruthTable::isMux);
}

}  // namespace naja::NL
