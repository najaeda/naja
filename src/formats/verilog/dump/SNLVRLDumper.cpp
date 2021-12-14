#include "SNLVRLDumper.h"

#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLBusTerm.h"
#include "SNLBusNet.h"

namespace {

void dumpDirection(const SNL::SNLTerm* term, std::ostream& o) {
  switch (term->getDirection()) {
    case SNL::SNLTerm::Direction::Input:
      o << "input";
      break;
    case SNL::SNLTerm::Direction::Output:
      o << "output";
      break;
    case SNL::SNLTerm::Direction::InOut:
      o << "inout";
      break;
  }
}

}

namespace SNL {

std::string SNLVRLDumper::createDesignName(const SNLDesign* design) {
  auto library = design->getLibrary();
  auto designID = design->getID();
  std::string designName = "module" + std::to_string(designID);
  int conflict = 0;
  while (library->getDesign(designName)) {
    designName += "_" + std::to_string(conflict++); 
  }
  return designName;
}

std::string SNLVRLDumper::createInstanceName(const SNLInstance* instance) {
  auto design = instance->getDesign();
  auto instanceID = instance->getID();
  std::string instanceName = "inst" + std::to_string(instanceID);
  int conflict = 0;
  while (design->getInstance(instanceName)) {
    instanceName += "_" + std::to_string(conflict++); 
  }
  return instanceName;
}

void SNLVRLDumper::dumpInterface(const SNLDesign* design, std::ostream& o) {
  o << "(";
  bool first = true;
  for (auto term: design->getTerms()) {
    if (not first) {
      o << " ,";
    } else {
      first = false;
    }
    dumpDirection(term, o);
    o << " ";
    if (auto bus = dynamic_cast<SNLBusTerm*>(term)) {
      o << "[" << bus->getMSB() << ":" << bus->getLSB() << "] ";
    }
    o << term->getName();
  }
  o << ");";
}

void SNLVRLDumper::dumpNets(const SNLDesign* design, std::ostream& o) {
  for (auto net: design->getNets()) {
    o << "wire ";
    if (auto bus = dynamic_cast<SNLBusNet*>(net)) {
      o << "[" << bus->getMSB() << ":" << bus->getLSB() << "] ";
    }
    o << net->getName();
    o << ";" << std::endl;
  }
  o << std::endl;
}

void SNLVRLDumper::dumpInstanceInterface(const SNLInstance* instance, std::ostream& o) {
  o << "()";
}

void SNLVRLDumper::dumpInstance(const SNLInstance* instance, std::ostream& o) {
  SNLName instanceName;
  if (instance->isAnonymous()) {
    instanceName = createInstanceName(instance);
  } else {
    instanceName = instance->getName();
  }
  auto model = instance->getModel();
  if (model->isAnonymous()) {
    o << instanceName << std::endl;
  } else {
    o << model->getName() << " " << instanceName;
  }
  dumpInstanceInterface(instance, o);
  o << ";" << std::endl;
}

void SNLVRLDumper::dumpInstances(const SNLDesign* design, std::ostream& o) {
  for (auto instance: design->getInstances()) {
    dumpInstance(instance, o);
  }
}

void SNLVRLDumper::dumpDesign(const SNLDesign* design, std::ostream& o) {
  if (design->isAnonymous()) {
    createDesignName(design);
  }
  o << "module " << design->getName();

  dumpInterface(design, o);

  o << std::endl;

  dumpNets(design, o);

  dumpInstances(design, o);

  o << "endmodule //" << design->getName();
  o << std::endl;
}

}
