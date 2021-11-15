#include "SNLDB0.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLScalarNet.h"

namespace SNL {

SNLDB* SNLDB0::create(SNLUniverse* universe) {
  SNLDB* db = SNLDB::create(universe);
  assert(db->getID() == 0);

  auto db0RootLibrary = SNLLibrary::create(db);
  auto primitivesLibrary = SNLLibrary::create(db0RootLibrary, "Primitives");

  universe->assign_ = SNLDesign::create(primitivesLibrary);
  universe->assignInput_ = SNLScalarTerm::create(universe->assign_, SNLTerm::Direction::Input);
  universe->assignOutput_ = SNLScalarTerm::create(universe->assign_, SNLTerm::Direction::Output);

  SNLScalarNet* assignFT = SNLScalarNet::create(universe->assign_);
  universe->assignInput_->setNet(assignFT);
  universe->assignOutput_->setNet(assignFT);

  return db;
}

}
