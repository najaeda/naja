#include "SNLDB0.h"

#include "SNLUniverse.h"
#include "SNLScalarTerm.h"

namespace SNL {

SNLDB* SNLDB0::create(SNLUniverse* universe) {
  SNLDB* db = SNLDB::create(universe);
  assert(db->getID() == 0);

  auto db0RootLibrary = SNLLibrary::create(db);
  auto primitivesLibrary = SNLLibrary::create(db0RootLibrary, "Primitives");

  universe->assign_ = SNLDesign::create(primitivesLibrary);
  universe->assignInput_ = SNLScalarTerm::create(universe->assign_, SNLTerm::Direction::Input);
  universe->assignOutput_ = SNLScalarTerm::create(universe->assign_, SNLTerm::Direction::Output);

  return db;
}

}
