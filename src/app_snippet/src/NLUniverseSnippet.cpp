// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NLUniverseSnippet.h"

#include <iostream>

#include "NLUniverse.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLInstTerm.h"

using namespace naja::NL;

void NLUniverseSnippet::create() {
  NLUniverse::create();
  auto db = NLDB::create(NLUniverse::get());
  auto primLib = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("primitives"));
  auto prim0 = SNLDesign::create(primLib, SNLDesign::Type::Primitive);
  auto prim1 = SNLDesign::create(primLib, SNLDesign::Type::Primitive);

  auto mylib = NLLibrary::create(db, NLName("mylib"));

  auto model1 = SNLDesign::create(mylib, NLName("Model1"));
  {
    SNLScalarTerm::create(model1, SNLTerm::Direction::Input, NLName("i0"));
    SNLScalarTerm::create(model1, SNLTerm::Direction::Input, NLName("i1"));
    SNLScalarTerm::create(model1, SNLTerm::Direction::Output, NLName("o"));
    SNLParameter::create(model1, NLName("PARAM0"), SNLParameter::Type::Decimal, "18");
    SNLParameter::create(model1, NLName("PARAM1"), SNLParameter::Type::String,  "OPTION2");
    SNLInstance::create(model1, prim0); // anonymous
    SNLInstance::create(model1, prim1, NLName("ins"));  
  }

  std::cout << model1->getName().getString() << " terms:" << std::endl;
  for (auto term: model1->getTerms()) {
    std::cout << "  - " << term->getString() << std::endl;
  }

  auto model2 = SNLDesign::create(mylib, NLName("Model2"));
  SNLBusTerm::create(model2, SNLTerm::Direction::Input, 4, 0, NLName("i0"));
  SNLScalarTerm::create(model2, SNLTerm::Direction::Input, NLName("i1"));
  SNLBusTerm::create(model2, SNLTerm::Direction::Output, 31, 0, NLName("o"));
  std::cout << model2->getName().getString() << " terms:" << std::endl;
  for (auto term: model2->getTerms()) {
    std::cout << "  - " << term->getString() << std::endl;
  }

  auto top = SNLDesign::create(mylib, NLName("top"));
  {
    auto i = SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("i"));
    auto o = SNLScalarTerm::create(top, SNLTerm::Direction::Input, NLName("o"));
    auto ins1 = SNLInstance::create(top, model1, NLName("ins1"));
    auto ins2 = SNLInstance::create(top, model2, NLName("ins2"));
    auto net1 = SNLScalarNet::create(top); //anonymous
    auto net2 = SNLScalarNet::create(top); //anonymous
    auto net3 = SNLScalarNet::create(top, NLName("n"));
    i->setNet(net1);
    ins1->getInstTerm(ins1->getModel()->getScalarTerm(NLName("i0")))->setNet(net1);
    ins1->getInstTerm(ins1->getModel()->getScalarTerm(NLName("i1")))->setNet(net3);
    ins2->getInstTerm(ins2->getModel()->getScalarTerm(NLName("i1")))->setNet(net3);
    ins2->getInstTerm(ins2->getModel()->getBusTerm(NLName("o"))->getBit(0))->setNet(net3);
    o->setNet(net2);
  }

  auto topIns1 = top->getInstance(NLName("ins1"));
  std::cout << topIns1->getName().getString() << " instance terminals:" << std::endl;
  for (auto instTerm: topIns1->getInstTerms()) {
    std::cout << "  - " << instTerm->getBitTerm()->getName().getString() << std::endl;
  }

  auto topNet1 = top->getScalarNet(0); // net1 is an anonymous net at ID 0
  std::cout << topNet1->getString() << " components:" << std::endl;
  for (auto component: topNet1->getComponents()) {
    std::cout << "  - " << component->getString() << std::endl;
  }
}
