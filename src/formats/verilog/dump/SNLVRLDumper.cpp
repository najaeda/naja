#include "SNLVRLDumper.h"

#include "SNLDesign.h"

namespace SNL {

SNLName SNLVRLDumper::createInstanceName(const SNLInstance* instance) {
  auto design = instance->getDesign();
  auto instanceID = instance->getID();
  SNLName instanceName = "inst" + std::to_string(instanceID);
  int conflict = 0;
  while (design->getInstance(instanceName)) {
    instanceName += "_" + std::to_string(conflict++); 
  }
  return instanceName;
}

void SNLVRLDumper::dumpDesign(const SNLDesign* design, std::ostream& o) {
  if (design->isAnonymous()) {
    return;
  }
  o << "module " << design->getName() << std::endl;

  SNLCollection<SNLNet> nets = design->getNets();
  SNLIterator<SNLNet> netIt = nets.getIterator();

  SNLCollection<SNLInstance> instances = design->getInstances();
  SNLIterator<SNLInstance> instanceIt = instances.getIterator();
  while (instanceIt.isValid()) {
    auto instance = instanceIt.getElement();
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
    ++instanceIt;
  }
  o << "endmodule //" << design->getName();
  o << std::endl;
}

}
