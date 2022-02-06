/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SNLParameter.h"

#include "SNLDesign.h"
#include "SNLException.h"

namespace SNL {

SNLParameter::SNLParameter(SNLDesign* design, const SNLName& name, const std::string& value):
  design_(design), name_(name), value_(value)
{}

SNLParameter* SNLParameter::create(SNLDesign* design, const SNLName& name, const std::string& value) {
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
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLParameter named: " + name.getString();
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
