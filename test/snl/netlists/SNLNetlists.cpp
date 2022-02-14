#include "SNLNetlists.h"

#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h" 
#include "SNLInstTerm.h" 

using namespace naja::SNL;

SNLDesign* SNLNetlists::createNetlist0(SNLDB* db) {
  auto primsLib = SNLLibrary::create(db, SNLLibrary::Type::Primitives, SNLName("PRIMS"));
  auto designsLib = SNLLibrary::create(db, SNLName("DESIGNS"));

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

  auto top = SNLDesign::create(designsLib, SNLName("top"));
  {
    auto i = SNLBusTerm::create(top, SNLTerm::Direction::Input, 0, 1, SNLName("i"));
    auto o = SNLBusTerm::create(top, SNLTerm::Direction::InOut, 0, 1, SNLName("o"));
    auto module0Ins0 = SNLInstance::create(top, module0, SNLName("ins0"));
    auto module0Ins1 = SNLInstance::create(top, module0, SNLName("ins1"));
  }
  
  return top;
}
