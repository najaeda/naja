// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLLibraryTruthTables.h"

#include <sstream>

#include "SNLLibrary.h"
#include "SNLDesignModeling.h"
#include "SNLException.h"

namespace naja { namespace SNL {

SNLLibraryTruthTables::TruthTables SNLLibraryTruthTables::construct(SNLLibrary* library) {
  if (library->getType() != SNLLibrary::Type::Primitives) {
    return {};
  }
  TruthTables truthTables;
  for (auto design : library->getDesigns()) {
    SNLTruthTable tt = SNLDesignModeling::getTruthTable(design);
    if (tt.isInitialized()) {
      auto it = truthTables.find(tt);
      if (it != truthTables.end()) {
        std::ostringstream oss;
        oss << "Duplicate truth table: " << tt.getString();
        oss << " in designs: " << it->second->getString() << " and " << design->getString();
        throw SNLException(oss.str());
      }
      truthTables[tt] = design;
    }
  }
  return truthTables;
}

SNLDesign* SNLLibraryTruthTables::getDesignForTruthTable(const TruthTables& truthTables, const SNLTruthTable& tt) {
  auto it = truthTables.find(tt);
  if (it == truthTables.end()) {
    return nullptr;
  }
  return it->second;
}

}} // namespace SNL // namespace naja