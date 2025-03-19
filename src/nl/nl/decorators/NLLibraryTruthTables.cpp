// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NLLibraryTruthTables.h"

#include <sstream>

#include "NajaPrivateProperty.h"

#include "NLLibrary.h"
#include "NLException.h"

#include "SNLDesignTruthTable.h"

namespace {

class NLLibraryModelingProperty: public naja::NajaPrivateProperty {
  public:
    using Inherit = naja::NajaPrivateProperty;
    static const inline std::string Name = "NLLibraryModelingProperty";
    static NLLibraryModelingProperty* create(
      naja::NL::NLLibrary* library,
      const naja::NL::NLLibraryTruthTables::LibraryTruthTables& truthTables) {
      preCreate(library, Name);
      NLLibraryModelingProperty* property = new NLLibraryModelingProperty();
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
    naja::NL::NLLibraryTruthTables::LibraryTruthTables getTruthTables() const {
      return truthTables_;
    }
  private:
    naja::NL::NLLibraryTruthTables::LibraryTruthTables  truthTables_ {};
};

NLLibraryModelingProperty* getProperty(const naja::NL::NLLibrary* library) {
  auto property =
    static_cast<NLLibraryModelingProperty*>(library->getProperty(NLLibraryModelingProperty::Name));
  if (property) {
    return property;
  }
  return nullptr;
}

} // namespace

namespace naja { namespace NL {

NLLibraryTruthTables::LibraryTruthTables NLLibraryTruthTables::construct(NLLibrary* library) {
  if (library->getType() != NLLibrary::Type::Primitives) {
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
  NLLibraryModelingProperty::create(library, truthTables);
  return truthTables;
}

NLLibraryTruthTables::LibraryTruthTables NLLibraryTruthTables::getTruthTables(const NLLibrary* library) {
  auto property = getProperty(library);
  if (property) {
    return property->getTruthTables();
  }
  return {};
}

std::pair<SNLDesign*, NLLibraryTruthTables::Indexes>
NLLibraryTruthTables::getDesignForTruthTable(const NLLibrary* library, const SNLTruthTable& tt) {
  NLLibraryTruthTables::LibraryTruthTables truthTables = NLLibraryTruthTables::getTruthTables(library);
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

}} // namespace NL // namespace naja
