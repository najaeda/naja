// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NLDB0.h"

#include "NLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"

namespace naja { namespace NL {

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
  return "Bug";
}

NLDB* NLDB0::create(NLUniverse* universe) {
  NLDB* db = NLDB::create(universe);
  assert(db->getID() == 0);

  auto rootLibrary =
    NLLibrary::create(db, NLLibrary::Type::Primitives, NLName(RootLibraryName));

  auto assign = SNLDesign::create(rootLibrary, SNLDesign::Type::Primitive);
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

NLLibrary* NLDB0::getDB0RootLibrary() {
  auto db0 = NLDB0::getDB0();
  if (db0) {
    return db0->getLibrary(NLName(RootLibraryName));
  }
  return nullptr;
}

bool NLDB0::isDB0Library(const NLLibrary* library) {
  auto topLibrary = getDB0RootLibrary();
  return library == topLibrary;
  if (library->isRoot()) {
    return false;
  }
  return isDB0Library(library->getParentLibrary());
}

bool NLDB0::isDB0Primitive(const SNLDesign* design) {
  return design and isDB0Library(design->getLibrary());
}

SNLDesign* NLDB0::getAssign() {
  auto primitives = getDB0RootLibrary();
  if (primitives) {
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
  assert(nbOutputs>0);
  auto gateLibrary = getOrCreateGateLibrary(type);
  if (not gateLibrary) {
    return nullptr;
  }
  std::string gateName(type.getString() + "_" + std::to_string(nbOutputs));
  auto gate = gateLibrary->getSNLDesign(NLName(gateName));
  if (not gate) {
    gate = SNLDesign::create(gateLibrary, SNLDesign::Type::Primitive, NLName(gateName));
    SNLScalarTerm::create(gate, SNLTerm::Direction::Input);
    SNLBusTerm::create(gate, SNLTerm::Direction::Output, NLID::Bit(nbOutputs-1), 0);
  }
  return gate;
}

SNLDesign* NLDB0::getOrCreateNInputGate(const GateType& type, size_t nbInputs) {
  assert(nbInputs>0);
  auto gateLibrary = getOrCreateGateLibrary(type);
  if (not gateLibrary) {
    return nullptr;
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

bool NLDB0::isGate(const SNLDesign* design) {
  return isNInputGate(design);
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

SNLScalarTerm* NLDB0::getGateSingleTerm(const SNLDesign* gate) {
  if (isGate(gate)) {
    return gate->getScalarTerm(NLID::DesignObjectID(0));
  }
  return nullptr;
}

SNLBusTerm* NLDB0::getGateNTerms(const SNLDesign* gate) {
  if (isGate(gate)) {
    return gate->getBusTerm(NLID::DesignObjectID(1));
  }
  return nullptr;
}

}} // namespace NL // namespace naja
