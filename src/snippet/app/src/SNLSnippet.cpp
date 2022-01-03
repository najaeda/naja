#include "SNLUniverse.h"

#include <iostream>

#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"

using namespace SNL;

int main() {
  SNLUniverse::create();
  auto db = SNLDB::create(SNLUniverse::get());
  auto primLib = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("primitives"));
  auto prim0 = SNLDesign::create(primLib, SNLDesign::Type::Primitive);
  auto prim1 = SNLDesign::create(primLib, SNLDesign::Type::Primitive);

  auto mylib = SNLLibrary::create(db, SNLName("mylib"));
  auto model1 = SNLDesign::create(mylib, SNLName("Model1"));
  SNLScalarTerm::create(model1, SNLTerm::Direction::Input, SNLName("i0"));
  SNLScalarTerm::create(model1, SNLTerm::Direction::Input, SNLName("i1"));
  SNLScalarTerm::create(model1, SNLTerm::Direction::Output, SNLName("o"));
  std::cout << "Model1 terms:" << std::endl;
  for (auto term: model1->getTerms()) {
    std::cout << "  - " << term->getString() << std::endl;
  }

  auto model2 = SNLDesign::create(mylib, SNLName("Model2"));
  SNLBusTerm::create(model2, SNLTerm::Direction::Input, 4, 0, SNLName("i0"));
  SNLScalarTerm::create(model2, SNLTerm::Direction::Input, SNLName("i1"));
  SNLBusTerm::create(model2, SNLTerm::Direction::Output, 31, 0, SNLName("o"));
  std::cout << "Model2 terms:" << std::endl;
  for (auto term: model2->getTerms()) {
    std::cout << "  - " << term->getString() << std::endl;
  }
  
  return 0;
}
