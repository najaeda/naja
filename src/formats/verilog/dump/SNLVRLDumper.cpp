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
    return;
  }
  o << "module " << design->getName() << std::endl;

  for (auto instance: design->getInstances()) {
    dumpInstance(instance, o);
  }
  o << "endmodule //" << design->getName();
  o << std::endl;
}

}
