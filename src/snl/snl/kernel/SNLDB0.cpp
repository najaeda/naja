// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDB0.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"

namespace naja { namespace SNL {

SNLDB* SNLDB0::create(SNLUniverse* universe) {
  SNLDB* db = SNLDB::create(universe);
  assert(db->getID() == 0);

  auto primitivesLibrary =
    SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName(PrimitivesLibraryName));

  auto assign = SNLDesign::create(primitivesLibrary, SNLDesign::Type::Primitive);
  auto assignInput = SNLScalarTerm::create(assign, SNLTerm::Direction::Input);
  auto assignOutput = SNLScalarTerm::create(assign, SNLTerm::Direction::Output);

  SNLScalarNet* assignFT = SNLScalarNet::create(assign);
  assignInput->setNet(assignFT);
  assignOutput->setNet(assignFT);
  return db;
}

SNLDB* SNLDB0::getDB0() {
  return SNLUniverse::getDB0();
}

bool SNLDB0::isDB0(const SNLDB* db) {
  return db and db == getDB0();
}

SNLLibrary* SNLDB0::getPrimitivesLibrary() {
  auto db0 = SNLDB0::getDB0();
  if (db0) {
    return db0->getLibrary(SNLName(PrimitivesLibraryName));
  }
  return nullptr;
}
bool SNLDB0::isDB0Primitive(const SNLDesign* design) {
  return design and design->getLibrary() == getPrimitivesLibrary();
}

SNLDesign* SNLDB0::getAssign() {
  auto primitives = getPrimitivesLibrary();
  if (primitives) {
    return primitives->getDesign(SNLID::DesignID(0));
  }
  return nullptr;
}

bool SNLDB0::isAssign(const SNLDesign* design) {
  return design and design == getAssign();
}

SNLScalarTerm* SNLDB0::getAssignInput() {
  auto assign = getAssign();
  if (assign) {
    return assign->getScalarTerm(SNLID::DesignObjectID(0));
  }
  return nullptr;
}

SNLScalarTerm* SNLDB0::getAssignOutput() {
  auto assign = getAssign();
  if (assign) {
    return assign->getScalarTerm(SNLID::DesignObjectID(1));
  }
  return nullptr;
}

SNLLibrary* SNLDB0::getANDLibrary() {
  auto library = getPrimitivesLibrary();
  if (library) {
    return library->getLibrary(SNLName(ANDName));
  }
  return nullptr;
}

SNLDesign* SNLDB0::getAND(size_t nbInputs) {
  assert(nbInputs>0);
  auto primitives = getPrimitivesLibrary();
  if (primitives) {
    auto andLibrary = primitives->getLibrary(SNLName(ANDName));
    if (not andLibrary) {
      andLibrary = SNLLibrary::create(primitives, SNLLibrary::Type::Primitives, SNLName(ANDName));
    }
    std::string andGateName(std::string(ANDName) + "_" + std::to_string(nbInputs));
    auto andGate = andLibrary->getDesign(SNLName(andGateName));
    if (not andGate) {
      andGate = SNLDesign::create(andLibrary, SNLDesign::Type::Primitive, SNLName(andGateName));
      SNLScalarTerm::create(andGate, SNLTerm::Direction::Output);
      SNLBusTerm::create(andGate, SNLTerm::Direction::Input, SNLID::Bit(nbInputs-1), 0);
    }
    return andGate;
  }
  return nullptr;
}

bool SNLDB0::isAND(const SNLDesign* design) {
  return design->getLibrary() == getANDLibrary();
}

SNLScalarTerm* SNLDB0::getANDOutput(const SNLDesign* gate) {
  if (isAND(gate)) {
    return gate->getScalarTerm(SNLID::DesignObjectID(0));
  }
  return nullptr;
}

SNLBusTerm* SNLDB0::getANDInputs(const SNLDesign* gate) {
  if (isAND(gate)) {
    return gate->getBusTerm(SNLID::DesignObjectID(1));
  }
  return nullptr;
}

}} // namespace SNL // namespace naja