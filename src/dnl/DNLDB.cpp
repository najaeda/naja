#include "DNLDB.h"

namespace naja { namespace DNL {

DNLDB* DNLDB::create() {
  preCreate();
  auto db = new DNLDB();
  db->postCreate();
  return db;
}

void DNLDB::preCreate() {}

void DNLDB::postCreate() {}

void DNLDB::preDestroy() {}

void DNLDB::destroy() {
  preDestroy();
  delete this;
}

void addInstance() {
  
}

}} //namespace DNL // namespace naja