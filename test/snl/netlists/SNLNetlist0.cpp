#include "SNLNetlist0.h"

#include "SNLUniverse.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h" 

using namespace naja::SNL;

SNLDesign* SNLNetlist0::create(SNLDB* db) {
  auto primsLib = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  auto designsLib = SNLLibrary::create(db, SNLName(DesignsLibName));

  auto and2 = SNLDesign::create(primsLib, SNLDesign::Type::Primitive, SNLName("AND2"));
  {
    auto i0 = SNLScalarTerm::create(and2, SNLTerm::Direction::Input , SNLName("i0"));
    auto i1 = SNLScalarTerm::create(and2, SNLTerm::Direction::Input, SNLName("i1"));
    auto o = SNLScalarTerm::create(and2, SNLTerm::Direction::Output, SNLName("o"));
  }

  auto inv = SNLDesign::create(primsLib, SNLDesign::Type::Primitive, SNLName("INV"));
  {
    auto i = SNLScalarTerm::create(inv, SNLTerm::Direction::Input , SNLName("i"));
    auto o = SNLScalarTerm::create(inv, SNLTerm::Direction::Output, SNLName("o"));
  }

  //Module0
  auto module0 = SNLDesign::create(designsLib, SNLName("Module0"));
  {
    auto i0 = SNLScalarTerm::create(module0, SNLTerm::Direction::Input , SNLName("i0"));
    auto i0Net = SNLScalarNet::create(module0, i0->getName());
    i0->setNet(i0Net);
    auto i1 = SNLScalarTerm::create(module0, SNLTerm::Direction::Input, SNLName("i1"));
    auto i1Net = SNLScalarNet::create(module0, i1->getName());
    i1->setNet(i1Net);
    auto o = SNLScalarTerm::create(module0, SNLTerm::Direction::Output, SNLName("o"));
    auto and2Inst = SNLInstance::create(module0, and2, SNLName("a"));
    auto invInst = SNLInstance::create(module0, inv, SNLName("i"));
    and2Inst->getInstTerm(and2Inst->getModel()->getScalarTerm(SNLName("i0")))->setNet(i0Net);
    and2Inst->getInstTerm(and2Inst->getModel()->getScalarTerm(SNLName("i1")))->setNet(i1Net);
  }

  auto top = SNLDesign::create(designsLib, SNLName(TopName));
  {
    auto i = SNLBusTerm::create(top, SNLTerm::Direction::Input, 0, 1, SNLName(TopIName));
    auto iNet = SNLBusNet::create(top, 0, 1, SNLName(TopIName));
    i->setNet(iNet);
    auto o = SNLScalarTerm::create(top, SNLTerm::Direction::InOut, SNLName(TopOName));
    auto oNet = SNLScalarNet::create(top, SNLName(TopOName));
    o->setNet(oNet);
    auto net = SNLScalarNet::create(top, SNLName(TopNetName));
    auto module0Ins0 = SNLInstance::create(top, module0, SNLName(TopIns0Name));
    module0Ins0->setTermNet(module0->getScalarTerm(SNLName(ModuleI0Name)), iNet->getBit(0));
    module0Ins0->setTermNet(module0->getScalarTerm(SNLName(ModuleI1Name)), iNet->getBit(1));
    module0Ins0->setTermNet(module0->getScalarTerm(SNLName(ModuleOName)), net);
    auto module0Ins1 = SNLInstance::create(top, module0, SNLName(TopIns1Name));
    module0Ins1->setTermNet(module0->getScalarTerm(SNLName(ModuleI0Name)), net);
    module0Ins1->setTermNet(module0->getScalarTerm(SNLName(ModuleI1Name)), net);
    module0Ins1->setTermNet(module0->getScalarTerm(SNLName(ModuleOName)), oNet);
  }
  
  return top;
}

SNLDB* SNLNetlist0::getDB() {
  auto universe = SNLUniverse::get();
  if (universe) {
    return universe->getDB(1);
  }
  return nullptr;
}

SNLLibrary* SNLNetlist0::getDesignsLib() {
  auto db = getDB();
  if (db) {
    return db->getLibrary(SNLName(DesignsLibName));
  }
  return nullptr;
}

SNLDesign* SNLNetlist0::getTop() {
  auto designsLib = getDesignsLib();
  if (designsLib) {
    return designsLib->getDesign(SNLName(TopName));
  }
  return nullptr;
}

SNLInstance* SNLNetlist0::getTopIns0() {
  auto top = getTop();
  if (top) {
    return top->getInstance(SNLName(TopIns0Name));
  }
  return nullptr;
}

SNLInstance* SNLNetlist0::getTopIns1() {
  auto top = getTop();
  if (top) {
    return top->getInstance(SNLName(TopIns1Name));
  }
  return nullptr;
}

SNLBusTerm* SNLNetlist0::getTopOTerm() {
  auto top = getTop();
  if (top) {
    return top->getBusTerm(SNLName(TopOName));
  }
  return nullptr;
}

SNLBusTerm* SNLNetlist0::getTopITerm() {
  auto top = getTop();
  if (top) {
    return top->getBusTerm(SNLName(TopIName));
  }
  return nullptr;
}

SNLScalarNet* SNLNetlist0::getTopONet() {
  auto top = getTop();
  if (top) {
    return top->getScalarNet(SNLName(TopOName));
  }
  return nullptr;
}

SNLBusNet* SNLNetlist0::getTopINet() {
  auto top = getTop();
  if (top) {
    return top->getBusNet(SNLName(TopIName));
  }
  return nullptr;
}
