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

#include "SNLScalarNet.h"

#include <sstream>

#include "SNLDB.h"
#include "SNLException.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"

namespace naja { namespace SNL {

SNLScalarNet::SNLScalarNet(SNLDesign* design, const SNLName& name):
  super(),
  design_(design),
  name_(name)
{}

SNLScalarNet::SNLScalarNet(SNLDesign* design, SNLID::DesignObjectID id, const SNLName& name):
  super(),
  design_(design),
  id_(id),
  name_(name)
{}

SNLScalarNet* SNLScalarNet::create(SNLDesign* design, const SNLName& name) {
  preCreate(design, name);
  SNLScalarNet* net = new SNLScalarNet(design, name);
  net->postCreateAndSetID();
  return net;
}

SNLScalarNet* SNLScalarNet::create(SNLDesign* design, SNLID::DesignObjectID id, const SNLName& name) {
  preCreate(design, id, name);
  SNLScalarNet* net = new SNLScalarNet(design, id, name);
  net->postCreate();
  return net;
}

void SNLScalarNet::preCreate(const SNLDesign* design, const SNLName& name) {
  super::preCreate();
  if (not design) {
    throw SNLException("malformed SNLScalarNet creator with NULL design argument");
  }
  if (not name.empty() and design->getNet(name)) {
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLScalarNet named: " + name.getString();
    throw SNLException(reason);
  }
}

void SNLScalarNet::preCreate(const SNLDesign* design, SNLID::DesignObjectID id, const SNLName& name) {
  preCreate(design, name);
  if (design->getNet(id)) {
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLScalarNet with id: " + std::to_string(id);
    throw SNLException(reason);
  }
}

void SNLScalarNet::postCreateAndSetID() {
  super::postCreate();
  getDesign()->addNetAndSetID(this);
}

void SNLScalarNet::postCreate() {
  super::postCreate();
  getDesign()->addNet(this);
}

void SNLScalarNet::commonPreDestroy() {
  super::preDestroy();
}

void SNLScalarNet::destroyFromDesign() {
  commonPreDestroy();
  delete this;
}

void SNLScalarNet::preDestroy() {
  commonPreDestroy();
  getDesign()->removeNet(this);
}

SNLID SNLScalarNet::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::Net, id_, 0, 0);
}

NajaCollection<SNLBitNet*> SNLScalarNet::getBits() const {
  return NajaCollection(new NajaSingletonCollection(const_cast<SNLScalarNet*>(this))).getParentTypeCollection<SNLBitNet*>();
}

//LCOV_EXCL_START
const char* SNLScalarNet::getTypeName() const {
  return "SNLScalarNet";
}
//LCOV_EXCL_STOP


//LCOV_EXCL_START
std::string SNLScalarNet::getString() const {
  return getName().getString();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLScalarNet::getDescription() const {
  std::ostringstream stream;
  stream << "<" << std::string(getTypeName());
  if (not isAnonymous()) {
    stream << " " + getName().getString();
  }
  stream << " " << getID();
  if (not getDesign()->isAnonymous()) {
    stream << " " + getDesign()->getName().getString();
  }
  stream << " " << getDesign()->getID();
  stream << ">";
  return stream.str(); 
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja
