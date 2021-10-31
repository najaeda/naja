#include "SNLDB0.h"

#include "SNLDB.h"

namespace SNL {

SNLDB* SNLDB0::create(SNLUniverse* universe) {
  SNLDB* db = SNLDB::create(universe);
  assert(db->getID() == 0);

  return db;
}

}
