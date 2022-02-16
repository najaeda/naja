#include "SNLUniverse.h"

#include <iostream>

#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
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
  {
    SNLScalarTerm::create(model1, SNLTerm::Direction::Input, SNLName("i0"));
    SNLScalarTerm::create(model1, SNLTerm::Direction::Input, SNLName("i1"));
    SNLScalarTerm::create(model1, SNLTerm::Direction::Output, SNLName("o"));
    SNLInstance::create(model1, prim0); // anonymous
    SNLInstance::create(model1, prim1, SNLName("ins"));  
  }

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
  {
    auto i = SNLScalarTerm::create(top, SNLTerm::Direction::Input, SNLName("i"));
    auto o = SNLScalarTerm::create(top, SNLTerm::Direction::Input, SNLName("o"));
    auto ins1 = SNLInstance::create(top, model1, SNLName("ins1"));
    auto ins2 = SNLInstance::create(top, model2, SNLName("ins2"));
    auto net1 = SNLScalarNet::create(top); //anonymous
    auto net2 = SNLScalarNet::create(top); //anonymous
    auto net3 = SNLScalarNet::create(top, SNLName("n"));
    i->setNet(net1);
    ins1->getInstTerm(ins1->getModel()->getScalarTerm(SNLName("i0")))->setNet(net1);
    ins1->getInstTerm(ins1->getModel()->getScalarTerm(SNLName("i1")))->setNet(net3);
    ins2->getInstTerm(ins2->getModel()->getScalarTerm(SNLName("i1")))->setNet(net3);
    ins2->getInstTerm(ins2->getModel()->getBusTerm(SNLName("o"))->getBit(0))->setNet(net3);
    o->setNet(net2);
  }

  auto topIns1 = top->getInstance(SNLName("ins1"));
  std::cout << topIns1->getName().getString() << " instance terminals:" << std::endl;
  for (auto instTerm: topIns1->getInstTerms()) {
    std::cout << "  - " << instTerm->getTerm()->getName().getString() << std::endl;
  }

  auto topNet1 = top->getScalarNet(0); // net1 is an anonymous net at ID 0
  std::cout << topNet1->getString() << " components:" << std::endl;
  for (auto component: topNet1->getComponents()) {
    std::cout << "  - " << component->getString() << std::endl;
  }
  
  return 0;
}
