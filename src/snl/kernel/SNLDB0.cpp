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
#include "SNLScalarNet.h"

namespace SNL {

SNLDB* SNLDB0::create(SNLUniverse* universe) {
  SNLDB* db = SNLDB::create(universe);
  assert(db->getID() == 0);

  auto db0RootLibrary = SNLLibrary::create(db);
  auto primitivesLibrary = SNLLibrary::create(db0RootLibrary, SNLName("Primitives"));

  universe->assign_ = SNLDesign::create(primitivesLibrary);
  universe->assignInput_ = SNLScalarTerm::create(universe->assign_, SNLTerm::Direction::Input);
  universe->assignOutput_ = SNLScalarTerm::create(universe->assign_, SNLTerm::Direction::Output);

  SNLScalarNet* assignFT = SNLScalarNet::create(universe->assign_);
  universe->assignInput_->setNet(assignFT);
  universe->assignOutput_->setNet(assignFT);

  return db;
}

}
