#include "SNLUniverse.h"

#include <iostream>

#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"
#include "SNLInstTerm.h"

using namespace naja::SNL;

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
  std::cout << model1->getName().getString() << " terms:" << std::endl;
  for (auto term: model1->getTerms()) {
    std::cout << "  - " << term->getString() << std::endl;
  }

  auto model2 = SNLDesign::create(mylib, SNLName("Model2"));
  SNLBusTerm::create(model2, SNLTerm::Direction::Input, 4, 0, SNLName("i0"));
  SNLScalarTerm::create(model2, SNLTerm::Direction::Input, SNLName("i1"));
  SNLBusTerm::create(model2, SNLTerm::Direction::Output, 31, 0, SNLName("o"));
  std::cout << model2->getName().getString() << " terms:" << std::endl;
  for (auto term: model2->getTerms()) {
    std::cout << "  - " << term->getString() << std::endl;
  }

  auto top = SNLDesign::create(mylib, SNLName("top"));
  auto i = SNLScalarTerm::create(top, SNLTerm::Direction::Input, SNLName("i"));
  auto o = SNLScalarTerm::create(top, SNLTerm::Direction::Input, SNLName("o"));
  auto ins1 = SNLInstance::create(top, model1, SNLName("ins1"));
  auto ins2 = SNLInstance::create(top, model2, SNLName("ins2"));
  auto net1 = SNLScalarNet::create(top);
  auto net2 = SNLScalarNet::create(top);
  i->setNet(net1);
  ins1->getInstTerm(ins1->getModel()->getScalarTerm(SNLName("i0")))->setNet(net1);
  o->setNet(net2);

  std::cout << ins1->getName().getString() << " instance terminals:" << std::endl;
  for (auto instTerm: ins1->getInstTerms()) {
    std::cout << "  - " << instTerm->getTerm()->getName().getString() << std::endl;
  }

  std::cout << net1->getName().getString() << " components:" << std::endl;
  for (auto component: net1->getComponents()) {
    std::cout << "  - " << component->getString() << std::endl;
  }
  
  return 0;
}
