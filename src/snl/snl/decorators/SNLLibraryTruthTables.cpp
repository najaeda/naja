// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLLibraryTruthTables.h"

#include <sstream>

#include "NajaPrivateProperty.h"

#include "SNLLibrary.h"
#include "SNLDesignModeling.h"
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
    static void preCreate(naja::SNL::SNLLibrary* library, const std::string& name) {
      Inherit::preCreate(library, name);
      if (not (library->isPrimitives())) {
        std::ostringstream reason;
        reason << "Impossible to add Library Modeling on a non primitives library <"
          << library->getName().getString() << ">";
        throw naja::SNL::SNLException(reason.str());
      }
    }
    std::string getName() const override {
      return Name;
    }
    std::string getString() const override {
      return Name;
    }
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

SNLDesign* SNLLibraryTruthTables::getDesignForTruthTable(const SNLLibrary* library, const SNLTruthTable& tt) {
  SNLLibraryTruthTables::LibraryTruthTables truthTables = SNLLibraryTruthTables::getTruthTables(library);
  if (truthTables.empty()) {
    return nullptr;
  }
  auto it = truthTables.find(tt);
  if (it == truthTables.end()) {
    return nullptr;
  }
  return it->second;
}

}} // namespace SNL // namespace naja