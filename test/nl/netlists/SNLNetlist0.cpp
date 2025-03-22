// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLNetlist0.h"

#include "NLUniverse.h"
#include "NLLibrary.h"

#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h" 

using namespace naja::NL;

SNLDesign* SNLNetlist0::create(NLDB* db) {
  auto primsLib = NLLibrary::create(db, NLLibrary::Type::Primitives, NLName("PRIMS"));
  auto designsLib = NLLibrary::create(db, NLName(DesignsLibName));

  auto and2 = SNLDesign::create(primsLib, SNLDesign::Type::Primitive, NLName("AND2"));
  {
    auto i0 = SNLScalarTerm::create(and2, SNLTerm::Direction::Input , NLName("i0"));
    auto i1 = SNLScalarTerm::create(and2, SNLTerm::Direction::Input, NLName("i1"));
    auto o = SNLScalarTerm::create(and2, SNLTerm::Direction::Output, NLName("o"));
  }

  auto inv = SNLDesign::create(primsLib, SNLDesign::Type::Primitive, NLName("INV"));
  {
    auto i = SNLScalarTerm::create(inv, SNLTerm::Direction::Input , NLName("i"));
    auto o = SNLScalarTerm::create(inv, SNLTerm::Direction::Output, NLName("o"));
  }

  //Module0
  auto module0 = SNLDesign::create(designsLib, NLName("Module0"));
  {
    auto i0 = SNLScalarTerm::create(module0, SNLTerm::Direction::Input , NLName("i0"));
    auto i0Net = SNLScalarNet::create(module0, i0->getName());
    i0->setNet(i0Net);
    auto i1 = SNLScalarTerm::create(module0, SNLTerm::Direction::Input, NLName("i1"));
    auto i1Net = SNLScalarNet::create(module0, i1->getName());
    i1->setNet(i1Net);
    auto o = SNLScalarTerm::create(module0, SNLTerm::Direction::Output, NLName("o"));
    auto and2Inst = SNLInstance::create(module0, and2, NLName("a"));
    auto invInst = SNLInstance::create(module0, inv, NLName("i"));
    and2Inst->getInstTerm(and2Inst->getModel()->getScalarTerm(NLName("i0")))->setNet(i0Net);
    and2Inst->getInstTerm(and2Inst->getModel()->getScalarTerm(NLName("i1")))->setNet(i1Net);
  }

  auto top = SNLDesign::create(designsLib, NLName(TopName));
  {
    auto i = SNLBusTerm::create(top, SNLTerm::Direction::Input, 0, 1, NLName(TopIName));
    auto iNet = SNLBusNet::create(top, 0, 1, NLName(TopIName));
    i->setNet(iNet);
    auto o = SNLScalarTerm::create(top, SNLTerm::Direction::InOut, NLName(TopOName));
    auto oNet = SNLScalarNet::create(top, NLName(TopOName));
    o->setNet(oNet);
    auto net = SNLScalarNet::create(top, NLName(TopNetName));
    auto module0Ins0 = SNLInstance::create(top, module0, NLName(TopIns0Name));
    module0Ins0->setTermNet(module0->getScalarTerm(NLName(ModuleI0Name)), iNet->getBit(0));
    module0Ins0->setTermNet(module0->getScalarTerm(NLName(ModuleI1Name)), iNet->getBit(1));
    module0Ins0->setTermNet(module0->getScalarTerm(NLName(ModuleOName)), net);
    auto module0Ins1 = SNLInstance::create(top, module0, NLName(TopIns1Name));
    module0Ins1->setTermNet(module0->getScalarTerm(NLName(ModuleI0Name)), net);
    module0Ins1->setTermNet(module0->getScalarTerm(NLName(ModuleI1Name)), net);
    module0Ins1->setTermNet(module0->getScalarTerm(NLName(ModuleOName)), oNet);
  }
  
  return top;
}

NLDB* SNLNetlist0::getDB() {
  auto universe = NLUniverse::get();
  if (universe) {
    return universe->getDB(1);
  }
  return nullptr;
}

NLLibrary* SNLNetlist0::getDesignsLib() {
  auto db = getDB();
  if (db) {
    return db->getLibrary(NLName(DesignsLibName));
  }
  return nullptr;
}

SNLDesign* SNLNetlist0::getTop() {
  auto designsLib = getDesignsLib();
  if (designsLib) {
    return designsLib->getSNLDesign(NLName(TopName));
  }
  return nullptr;
}

SNLInstance* SNLNetlist0::getTopIns0() {
  auto top = getTop();
  if (top) {
    return top->getInstance(NLName(TopIns0Name));
  }
  return nullptr;
}

SNLInstance* SNLNetlist0::getTopIns1() {
  auto top = getTop();
  if (top) {
    return top->getInstance(NLName(TopIns1Name));
  }
  return nullptr;
}

SNLBusTerm* SNLNetlist0::getTopOTerm() {
  auto top = getTop();
  if (top) {
    return top->getBusTerm(NLName(TopOName));
  }
  return nullptr;
}

SNLBusTerm* SNLNetlist0::getTopITerm() {
  auto top = getTop();
  if (top) {
    return top->getBusTerm(NLName(TopIName));
  }
  return nullptr;
}

SNLScalarNet* SNLNetlist0::getTopONet() {
  auto top = getTop();
  if (top) {
    return top->getScalarNet(NLName(TopOName));
  }
  return nullptr;
}

SNLBusNet* SNLNetlist0::getTopINet() {
  auto top = getTop();
  if (top) {
    return top->getBusNet(NLName(TopIName));
  }
  return nullptr;
}
