#include "SNLParameter.h"

#include "SNLDesign.h"
#include "SNLException.h"

namespace SNL {

SNLParameter::SNLParameter(SNLDesign* design, const SNLName& name, const SNLName& value):
  design_(design), name_(name), value_(value)
{}

SNLParameter* SNLParameter::create(SNLDesign* design, const SNLName& name, const SNLName& value) {
  preCreate(design, name);
  SNLParameter* parameter = new SNLParameter(design, name, value);
  parameter->postCreate();
  return parameter;
}

void SNLParameter::postCreate() {
  design_->addParameter(this);
}

void SNLParameter::preCreate(SNLDesign* design, const SNLName& name) {
  if (design->getParameter(name)) {
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLParameter named: " + name;
    throw SNLException(reason);
  }
}

void SNLParameter::destroy() {
  design_->removeParameter(this);
  delete this;
}

void SNLParameter::destroyFromDesign() {
  delete this;
}

}
