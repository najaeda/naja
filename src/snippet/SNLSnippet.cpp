#include "SNLUniverse.h"

using namespace SNL;

int main() {
  SNLUniverse::create();
  SNLDB* db = SNLDB::create(SNLUniverse::get());


  return 0;
}
