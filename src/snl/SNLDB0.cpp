#include "SNLDB0.h"

#include "SNLDB.h"

namespace SNL {

SNLDB* SNLDB0::create(SNLUniverse* universe) {
  SNLDB* db = SNLDB::create(universe);
  assert(db->getID() == 0);

  SNLLibrary* db0RootLibrary = SNLLibrary::create(db, "Root");
  /*SNLLibrary* primitivesLibrary = */ SNLLibrary::create(db0RootLibrary, "Primitives");

  return db;
}

}
