// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NLDB0.h"

#include "NLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"

namespace naja { namespace SNL {

NLDB* NLDB0::create(NLUniverse* universe) {
  NLDB* db = NLDB::create(universe);
  assert(db->getID() == 0);

  auto primitivesLibrary =
    NLLibrary::create(db, NLLibrary::Type::Primitives, NLName(PrimitivesLibraryName));

  auto assign = SNLDesign::create(primitivesLibrary, SNLDesign::Type::Primitive);
  auto assignInput = SNLScalarTerm::create(assign, SNLTerm::Direction::Input);
  auto assignOutput = SNLScalarTerm::create(assign, SNLTerm::Direction::Output);

  SNLScalarNet* assignFT = SNLScalarNet::create(assign);
  assignInput->setNet(assignFT);
  assignOutput->setNet(assignFT);
  return db;
}

NLDB* NLDB0::getDB0() {
  return NLUniverse::getDB0();
}

bool NLDB0::isDB0(const NLDB* db) {
  return db and db == getDB0();
}

NLLibrary* NLDB0::getPrimitivesLibrary() {
  auto db0 = NLDB0::getDB0();
  if (db0) {
    return db0->getLibrary(NLName(PrimitivesLibraryName));
  }
  return nullptr;
}
bool NLDB0::isDB0Primitive(const SNLDesign* design) {
  return design and design->getLibrary() == getPrimitivesLibrary();
}

SNLDesign* NLDB0::getAssign() {
  auto primitives = getPrimitivesLibrary();
  if (primitives) {
    return primitives->getDesign(NLID::DesignID(0));
  }
  return nullptr;
}

bool NLDB0::isAssign(const SNLDesign* design) {
  return design and design == getAssign();
}

SNLScalarTerm* NLDB0::getAssignInput() {
  auto assign = getAssign();
  if (assign) {
    return assign->getScalarTerm(NLID::DesignObjectID(0));
  }
  return nullptr;
}

SNLScalarTerm* NLDB0::getAssignOutput() {
  auto assign = getAssign();
  if (assign) {
    return assign->getScalarTerm(NLID::DesignObjectID(1));
  }
  return nullptr;
}

NLLibrary* NLDB0::getANDLibrary() {
  auto library = getPrimitivesLibrary();
  if (library) {
    return library->getLibrary(NLName(ANDName));
  }
  return nullptr;
}

SNLDesign* NLDB0::getAND(size_t nbInputs) {
  assert(nbInputs>0);
  auto primitives = getPrimitivesLibrary();
  if (primitives) {
    auto andLibrary = primitives->getLibrary(NLName(ANDName));
    if (not andLibrary) {
      andLibrary = NLLibrary::create(primitives, NLLibrary::Type::Primitives, NLName(ANDName));
    }
    std::string andGateName(std::string(ANDName) + "_" + std::to_string(nbInputs));
    auto andGate = andLibrary->getDesign(NLName(andGateName));
    if (not andGate) {
      andGate = SNLDesign::create(andLibrary, SNLDesign::Type::Primitive, NLName(andGateName));
      SNLScalarTerm::create(andGate, SNLTerm::Direction::Output);
      SNLBusTerm::create(andGate, SNLTerm::Direction::Input, NLID::Bit(nbInputs-1), 0);
    }
    return andGate;
  }
  return nullptr;
}

bool NLDB0::isAND(const SNLDesign* design) {
  return design->getLibrary() == getANDLibrary();
}

SNLScalarTerm* NLDB0::getANDOutput(const SNLDesign* gate) {
  if (isAND(gate)) {
    return gate->getScalarTerm(NLID::DesignObjectID(0));
  }
  return nullptr;
}

SNLBusTerm* NLDB0::getANDInputs(const SNLDesign* gate) {
  if (isAND(gate)) {
    return gate->getBusTerm(NLID::DesignObjectID(1));
  }
  return nullptr;
}

}} // namespace SNL // namespace naja
