/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
  auto universe = SNLUniverse::get();
  if (universe) {
    auto db0 = universe->getDB(0);
    assert(SNLUniverse::isDB0(db0));
    return db0;
  }
  return nullptr;
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