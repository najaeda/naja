// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesignTruthTable.h"

#include <sstream>

#include "NLException.h"
#include "NajaDumpableProperty.h"
#include "SNLDesign.h"

namespace {
static const std::string SNLDesignTruthTablePropertyName =
    "SNLDesignTruthTableProperty";

naja::NajaDumpableProperty* getProperty(const naja::NL::SNLDesign* design) {
  auto property = static_cast<naja::NajaDumpableProperty*>(
      design->getProperty(SNLDesignTruthTablePropertyName));
  return property ? property : nullptr;
}

void createProperty(naja::NL::SNLDesign* design,
                    const naja::NL::SNLTruthTable& truthTable) {
  if (getProperty(design)) {
    throw naja::NL::NLException("Design already has a Truth Table");
  }
  auto property = naja::NajaDumpableProperty::create(
      design, SNLDesignTruthTablePropertyName);
  property->addUInt64Value(truthTable.size());
  for (auto mask : truthTable.bits().getChunks()) {
    property->addUInt64Value(mask);
  }
}

void createProperty(naja::NL::SNLDesign* design,
                    const std::vector<naja::NL::SNLTruthTable>& truthTables) {
  if (truthTables.empty()) {
    throw naja::NL::NLException("Cannot set empty truth table");
  }
  if (getProperty(design)) {
    throw naja::NL::NLException("Design already has a Truth Table");
  }
  auto property = naja::NajaDumpableProperty::create(
      design, SNLDesignTruthTablePropertyName);
  for (const auto& truthTable : truthTables) {
    property->addUInt64Value(truthTable.size());
    for (auto mask : truthTable.bits().getChunks()) {
      property->addUInt64Value(mask);
    }
  }
}

}  // namespace

namespace naja {
namespace NL {

void SNLDesignTruthTable::setTruthTable(SNLDesign* design,
                                        const SNLTruthTable& truthTable) {
  if (!design->isPrimitive()) {
    throw NLException("Cannot add truth table on non-primitive design");
  }
  // Check no truth table already exists
  if (getProperty(design)) {
    throw NLException("Design already has a Truth Table");
  }
  auto outputs = design->getTerms().getSubCollection(
      [](const SNLTerm* t) {
        return t->getDirection() == SNLTerm::Direction::Output;
      });
  if (outputs.size() != 1) {
    std::ostringstream reason;
    reason << "cannot add truth table on Design <"
           << design->getName().getString() << "> that has <"
           << outputs.size() << "> outputs";
    throw NLException(reason.str());
  }
  createProperty(design, truthTable);
}

void SNLDesignTruthTable::setTruthTables(
    SNLDesign* design,
    const std::vector<SNLTruthTable>& truthTables) {
  if (!design->isPrimitive()) {
    throw NLException("Cannot add truth table on non-primitive design");
  }
  // Check no truth table already exists
  if (getProperty(design)) {
    throw NLException("Design already has a Truth Table");
  }
  auto outputs = design->getTerms().getSubCollection(
      [](const SNLTerm* t) {
        return t->getDirection() == SNLTerm::Direction::Output;
      });
  if (outputs.size() != truthTables.size()) {
    std::ostringstream reason;
    reason << "cannot add truth tables on Design <"
           << design->getName().getString() << "> that has <"
           << outputs.size() << "> outputs, but provided <"
           << truthTables.size() << "> truth tables";
    throw NLException(reason.str());
  }
  createProperty(design, truthTables);
}

SNLTruthTable SNLDesignTruthTable::getTruthTable(const SNLDesign* design) {
  auto property = getProperty(design);
  if (property) {
    // total number of mask‐values trailing the first “size” entry
    size_t tableSize = property->getValues().size() - 1;

    // how many chunks *should* we have, given the stored input‐count?
    uint64_t declaredInputs = property->getUInt64Value(0);
    uint64_t num_bits = 1u << declaredInputs;
    size_t   expectedChunks =
        (declaredInputs == 0 && tableSize == 1)
            ? 1
            : (num_bits / 64 + ((num_bits % 64) > 0 ? 1 : 0));
    if (expectedChunks != tableSize) {
      std::ostringstream reason;
      reason << "Truth table size " << tableSize
             << " does not match number of chunks " << expectedChunks << " which suggests per output functionality";
      throw NLException(reason.str());
    }

    // multi‐chunk (i.e. >64‐bit) table?
    if (property->getValues().size() > 2) {
      uint32_t numInputs = static_cast<uint32_t>(declaredInputs);
      uint32_t nBits     = 1u << numInputs;
      if (nBits <= 64) {
        // LCOV_EXCL_START
        std::ostringstream reason;
        reason << "Truth table size " << nBits
               << " is not larger than 64 bits";
        throw NLException(reason.str());
        // LCOV_EXCL_STOP
      }

      std::vector<bool> bits(nBits, false);
      size_t            nChunks = (nBits + 63) / 64;

      for (size_t c = 0; c < nChunks; ++c) {
        uint64_t mask = property->getUInt64Value(c + 1);
        for (size_t b = 0; b < 64; ++b) {
          size_t pos = c * 64 + b;
          if (pos >= nBits) break;
          if ((mask >> b) & 1) bits[pos] = true;
        }
      }

      return SNLTruthTable(numInputs, bits);
    }
    // single‐chunk
    if (declaredInputs <= 6) {
      return SNLTruthTable(static_cast<uint32_t>(declaredInputs),
                           property->getUInt64Value(1));
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

SNLTruthTable SNLDesignTruthTable::getTruthTable(
    const SNLDesign* design,
    NLID::DesignObjectID termID) {
  auto property = getProperty(design);
  std::map<NLID::DesignObjectID, NLID::DesignObjectID> termID2outputID;
  NLID::DesignObjectID outputIndex = 0;
  for (const auto& term : design->getTerms()) {
    if (term->getDirection() == SNLTerm::Direction::Output) {
      termID2outputID[term->getID()] = outputIndex;
      ++outputIndex;
    }
  }
  if (termID2outputID.find(termID) == termID2outputID.end()) {
    std::ostringstream reason;
    reason << "Term ID " << termID
           << " not found in design <" << design->getName().getString() << ">";
    throw NLException(reason.str());
  }
  NLID::DesignObjectID outputID = termID2outputID[termID];
  if (property) {
    // scan through each stored table until we reach outputID
    size_t tableIdx = 0;
    size_t valIdx   = 0;
    size_t total    = property->getValues().size();

    while (true) {
      if (valIdx >= total) {
        std::ostringstream reason;
        reason << "Output ID " << outputID
               << " is out of range for design <"
               << design->getName().getString() << ">";
        throw NLException(reason.str());
      }
      if (tableIdx >= outputID) {
        break;
      }
      uint32_t nInputs = static_cast<uint32_t>(
          property->getUInt64Value(valIdx));
      size_t   nBits   = 1u << nInputs;
      size_t   nChunks = nBits / 64 + ((nBits % 64) > 0 ? 1 : 0);
      valIdx += 1 + nChunks;
      ++tableIdx;
    }

    uint64_t declaredInputs = property->getUInt64Value(valIdx);
    // single‐chunk fast‐path?
    if (declaredInputs <= 6) {
      return SNLTruthTable(static_cast<uint32_t>(declaredInputs),
                           property->getUInt64Value(valIdx + 1));
    }

    // else multi‐chunk
    uint32_t numInputs = static_cast<uint32_t>(declaredInputs);
    uint32_t nBits     = 1u << numInputs;
    size_t   nChunks   = nBits / 64 + ((nBits % 64) > 0 ? 1 : 0);

    std::vector<bool> bits(nBits, false);
    for (size_t c = 0; c < nChunks; ++c) {
      uint64_t mask = property->getUInt64Value(valIdx + 1 + c);
      for (size_t b = 0; b < 64; ++b) {
        size_t pos = c * 64 + b;
        if (pos >= nBits) break;
        if ((mask >> b) & 1) bits[pos] = true;
      }
    }
    return SNLTruthTable(numInputs, bits);
  }
  return SNLTruthTable();
}

bool SNLDesignTruthTable::isConst0(const SNLDesign* design) {
  auto property = getProperty(design);
  // return false if number of values large than 2
  if (property && property->getValues().size() > 2) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() &&
         truthTable == SNLTruthTable::Logic0();
}

bool SNLDesignTruthTable::isConst1(const SNLDesign* design) {
   auto property = getProperty(design);
  // return false if number of values large than 2
  if (property && property->getValues().size() > 2) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() &&
         truthTable == SNLTruthTable::Logic1();
}

bool SNLDesignTruthTable::isConst(const SNLDesign* design) {
  auto property = getProperty(design);
  // return false if number of values large than 2
  if (property && property->getValues().size() > 2) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() &&
         (truthTable == SNLTruthTable::Logic0() ||
          truthTable == SNLTruthTable::Logic1());
}

bool SNLDesignTruthTable::isInv(const SNLDesign* design) {
  auto property = getProperty(design);
  // return false if number of values large than 2
  if (property && property->getValues().size() > 2) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() &&
         truthTable == SNLTruthTable::Inv();
}

bool SNLDesignTruthTable::isBuf(const SNLDesign* design) {
   auto property = getProperty(design);
  // return false if number of values large than 2
  if (property && property->getValues().size() > 2) {
    return false;
  }
  auto truthTable = getTruthTable(design);
  return truthTable.isInitialized() &&
         truthTable == SNLTruthTable::Buf();
}

}  // namespace NL
}  // namespace naja
