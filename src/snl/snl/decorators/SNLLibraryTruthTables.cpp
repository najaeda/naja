// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLLibraryTruthTables.h"

#include <sstream>

#include "NajaPrivateProperty.h"

#include "SNLLibrary.h"
#include "SNLDesignTruthTable.h"
#include "SNLException.h"

namespace {

class SNLLibraryModelingProperty: public naja::NajaPrivateProperty {
  public:
    using Inherit = naja::NajaPrivateProperty;
    static const inline std::string Name = "SNLLibraryModelingProperty";
    static SNLLibraryModelingProperty* create(
      naja::SNL::SNLLibrary* library,
      const naja::SNL::SNLLibraryTruthTables::LibraryTruthTables& truthTables) {
      preCreate(library, Name);
      SNLLibraryModelingProperty* property = new SNLLibraryModelingProperty();
      property->truthTables_ = truthTables;
      property->postCreate(library);
      return property;
    }
    std::string getName() const override {
      return Name;
    }
    //LCOV_EXCL_START
    std::string getString() const override {
      return Name;
    }
    //LCOV_EXCL_STOP
    naja::SNL::SNLLibraryTruthTables::LibraryTruthTables getTruthTables() const {
      return truthTables_;
    }
  private:
    naja::SNL::SNLLibraryTruthTables::LibraryTruthTables  truthTables_ {};
};

SNLLibraryModelingProperty* getProperty(const naja::SNL::SNLLibrary* library) {
  auto property =
    static_cast<SNLLibraryModelingProperty*>(library->getProperty(SNLLibraryModelingProperty::Name));
  if (property) {
    return property;
  }
  return nullptr;
}

} // namespace

namespace naja { namespace SNL {

SNLLibraryTruthTables::LibraryTruthTables SNLLibraryTruthTables::construct(SNLLibrary* library) {
  if (library->getType() != SNLLibrary::Type::Primitives) {
    return {};
  }
  LibraryTruthTables truthTables;
  for (auto design : library->getDesigns()) {
    SNLTruthTable tt = SNLDesignTruthTable::getTruthTable(design);
    if (tt.isInitialized()) {
      auto it = truthTables.find(tt);
      if (it != truthTables.end()) {
        it->second.push_back(design);
      } else {
        truthTables[tt] = {design};
      }
    }
  }
  SNLLibraryModelingProperty::create(library, truthTables);
  return truthTables;
}

SNLLibraryTruthTables::LibraryTruthTables SNLLibraryTruthTables::getTruthTables(const SNLLibrary* library) {
  auto property = getProperty(library);
  if (property) {
    return property->getTruthTables();
  }
  return {};
}

std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>
SNLLibraryTruthTables::getDesignForTruthTable(const SNLLibrary* library, const SNLTruthTable& tt) {
  SNLLibraryTruthTables::LibraryTruthTables truthTables = SNLLibraryTruthTables::getTruthTables(library);
  if (truthTables.empty()) {
    return std::pair(nullptr, Indexes());
  }
  Indexes indexes;
  auto it = truthTables.find(tt);
  if (it == truthTables.end()) {
    //retry with reduced truth table
    for (uint32_t i = 0; i < tt.size(); ++i) {
      if (tt.hasNoInfluence(i)) {
        SNLTruthTable reducedTT = tt.removeVariable(i);
        it = truthTables.find(reducedTT);
        if (it != truthTables.end()) {
          indexes.push_back(i);
          break;
        }
      }
    }
  }
  if (it == truthTables.end()) {
    return std::pair(nullptr, Indexes());
  }
  const auto& primitives = it->second;
  if (primitives.empty()) {
    return std::pair(nullptr, Indexes());
  }
  return std::pair(primitives[0], indexes);
}

}} // namespace SNL // namespace naja