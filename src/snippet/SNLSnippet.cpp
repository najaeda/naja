#include "SNLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"

using namespace SNL;

int main() {
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto mylib = SNLLibrary::create(db, "mylib");
  auto model1 = SNLDesign::create(mylib, "Model1");
  SNLScalarTerm::create(model1, SNLTerm::Direction::Input, "i0");
  SNLScalarTerm::create(model1, SNLTerm::Direction::Input, "i1");
  SNLScalarTerm::create(model1, SNLTerm::Direction::Output, "o");
  auto model2 = SNLDesign::create(mylib, "Model2");
  SNLBusTerm::create(model2, SNLTerm::Direction::Input, 4, 0, "i0");
  SNLScalarTerm::create(model2, SNLTerm::Direction::Input, "i1");
  SNLBusTerm::create(model2, SNLTerm::Direction::Output, 32, 0, "o");
  
  return 0;
}
