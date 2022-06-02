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

  universe->assign_ = SNLDesign::create(primitivesLibrary, SNLDesign::Type::Primitive);
  universe->assignInput_ = SNLScalarTerm::create(universe->assign_, SNLTerm::Direction::Input);
  universe->assignOutput_ = SNLScalarTerm::create(universe->assign_, SNLTerm::Direction::Output);

  SNLScalarNet* assignFT = SNLScalarNet::create(universe->assign_);
  universe->assignInput_->setNet(assignFT);
  universe->assignOutput_->setNet(assignFT);

  return db;
}

SNLDB* SNLDB0::getSNLDB0() {
  auto universe = SNLUniverse::get();
  if (universe) {
    auto db0 = universe->getDB(0);
    assert(SNLUniverse::isDB0(db0));
    return db0;
  }
  return nullptr;
}

SNLLibrary* SNLDB0::getPrimitivesLibrary() {
  auto db0 = SNLDB0::getSNLDB0();
  if (db0) {
    return db0->getLibrary(SNLName(PrimitivesLibraryName));
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

SNLDesign* SNLDB0::getAND(size_t size) {
  assert(size>0);
  auto primitives = getPrimitivesLibrary();
  if (primitives) {
    auto andLibrary = primitives->getLibrary(SNLName(ANDName));
    if (not andLibrary) {
      andLibrary = SNLLibrary::create(primitives, SNLLibrary::Type::Primitives, SNLName(ANDName));
    }
    std::string andGateName(std::string(ANDName) + "_" + std::to_string(size));
    auto andGate = andLibrary->getDesign(SNLName(andGateName));
    if (not andGate) {
      andGate = SNLDesign::create(andLibrary, SNLDesign::Type::Primitive, SNLName(andGateName));
      SNLScalarTerm::create(andGate, SNLTerm::Direction::Output);
      SNLBusTerm::create(andGate, SNLTerm::Direction::Input, SNLID::Bit(size-1), 0);
      SNLBusTerm::create(andGate, SNLTerm::Direction::Input, SNLID::Bit(size-1), 0);
    }
    return andGate;
  }
  return nullptr;
}

}} // namespace SNL // namespace naja