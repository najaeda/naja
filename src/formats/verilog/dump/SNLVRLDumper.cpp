#include "SNLVRLDumper.h"

#include "SNLLibrary.h"
#include "SNLDesign.h"

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

void SNLVRLDumper::dumpTerm(const SNLTerm* term, std::ostream& o) {
}

void SNLVRLDumper::dumpNet(const SNLNet* net, std::ostream& o) {

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
    o << model->getName() << " " << instanceName << std::endl;
  }
}

void SNLVRLDumper::dumpDesign(const SNLDesign* design, std::ostream& o) {
  if (design->isAnonymous()) {
    createDesignName(design);
  }
  o << "module " << design->getName() << std::endl;

  for (auto term: design->getTerms()) {
    dumpTerm(term, o);
  }

  for (auto net: design->getNets()) {
    dumpNet(net, o);
  }

  for (auto instance: design->getInstances()) {
    dumpInstance(instance, o);
  }
  o << "endmodule //" << design->getName();
  o << std::endl;
}

}
