#include "SNLParameter.h"

namespace SNL {

SNLParameter::SNLParameter(const SNLName& name, const SNLName& value):
  name_(name), value_(value)
{}

SNLParameter* SNLParameter::create(SNLDesign* design, const SNLName& name, const SNLName& value) {
  preCreate(design, name);
  SNLParameter* parameter = new SNLParameter(name, value);
  return parameter;
}

void SNLParameter::preCreate(SNLDesign* design, const SNLName& name) {

}

}
