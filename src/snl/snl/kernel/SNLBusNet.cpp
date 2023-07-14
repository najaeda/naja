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

#include "SNLBusNet.h"

#include <algorithm>

#include "SNLDB.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLBusNetBit.h"
#include "SNLException.h"
#include "SNLUtils.h"
#include "SNLMacros.h"

namespace naja { namespace SNL {

SNLBusNet::SNLBusNet(
    SNLDesign* design,
    SNLID::Bit msb,
    SNLID::Bit lsb,
    const SNLName& name):
  super(),
  design_(design),
  name_(name),
  msb_(msb),
  lsb_(lsb)
{}

SNLBusNet::SNLBusNet(
    SNLDesign* design,
    SNLID::DesignObjectID id,
    SNLID::Bit msb,
    SNLID::Bit lsb,
    const SNLName& name):
  super(),
  design_(design),
  id_(id),
  name_(name),
  msb_(msb),
  lsb_(lsb)
{}

SNLBusNet* SNLBusNet::create(
    SNLDesign* design,
    SNLID::Bit msb,
    SNLID::Bit lsb,
    const SNLName& name) {
  preCreate(design, name);
  SNLBusNet* net = new SNLBusNet(design, msb, lsb, name);
  net->postCreateAndSetID();
  return net;
}

SNLBusNet* SNLBusNet::create(
    SNLDesign* design,
    SNLID::DesignID id,
    SNLID::Bit msb,
    SNLID::Bit lsb,
    const SNLName& name) {
  preCreate(design, id, name);
  SNLBusNet* net = new SNLBusNet(design, id, msb, lsb, name);
  net->postCreate();
  return net;
}

void SNLBusNet::preCreate(const SNLDesign* design, const SNLName& name) {
  super::preCreate();
  if (not design) {
    throw SNLException("malformed SNLBusNet creator with NULL design argument");
  }
  if (not name.empty() and design->getNet(name)) {
    std::string reason = "cannot create SNLBusNet with name " + name.getString();
    reason += "A terminal with this name already exists.";
    throw SNLException(reason);
  }
}

void SNLBusNet::preCreate(const SNLDesign* design, SNLID::DesignObjectID id, const SNLName& name) {
  preCreate(design, name);
  if (design->getNet(id)) {
    std::string reason = "cannot create SNLBusNet with id " + std::to_string(id);
    reason += "A terminal with same id already exists.";
    throw SNLException(reason);
  }
}

void SNLBusNet::createBits() {
  //create bits
  size_t size = static_cast<size_t>(getSize());
  bits_.resize(size, nullptr);
  for (size_t i=0; i<size; i++) {
    SNLID::Bit bit = (getMSB()>getLSB())?getMSB()-int(i):getMSB()+int(i);
    bits_[i] = SNLBusNetBit::create(this, bit);
  }
}

void SNLBusNet::postCreateAndSetID() {
  super::postCreate();
  getDesign()->addNetAndSetID(this);
  createBits();
}

void SNLBusNet::postCreate() {
  super::postCreate();
  getDesign()->addNet(this);
  createBits();
}

void SNLBusNet::commonPreDestroy() {
  for (auto bit: bits_) {
    bit->destroyFromBus();
  }
  super::preDestroy();
}

void SNLBusNet::destroyFromDesign() {
  commonPreDestroy();
  delete this;
}

void SNLBusNet::preDestroy() {
  commonPreDestroy();
  getDesign()->removeNet(this);
}

SNLID::Bit SNLBusNet::getSize() const {
  return SNLUtils::getSize(getMSB(), getLSB());
}

SNLID SNLBusNet::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::Net, id_, 0, 0);
}

SNLBusNetBit* SNLBusNet::getBit(SNLID::Bit bit) const {
  if (SNLDesign::isBetween(bit, getMSB(), getLSB())) {
    size_t pos = static_cast<size_t>(std::abs(getMSB()-bit));
    return getBitAtPosition(pos);
  }
  return nullptr;
}

SNLBusNetBit* SNLBusNet::getBitAtPosition(size_t position) const {
  if (position < bits_.size()) {
    return bits_[position];
  }
  return nullptr;
}

NajaCollection<SNLBusNetBit*> SNLBusNet::getBusBits() const {
  return NajaCollection(new NajaSTLCollection(&bits_));
}

NajaCollection<SNLBitNet*> SNLBusNet::getBits() const {
  return getBusBits().getParentTypeCollection<SNLBitNet*>();
}

void SNLBusNet::insertBits(
  std::vector<SNLBitNet*>& bitNets,
  std::vector<SNLBitNet*>::const_iterator position,
  SNLID::Bit msb,
  SNLID::Bit lsb) {
  if (not SNLDesign::isBetween(msb, getMSB(), getLSB())) {
  }
  if (not SNLDesign::isBetween(lsb, getMSB(), getLSB())) {
  }
  int msbPos = std::abs(getMSB()-msb);
  int lsbPos = std::abs(getMSB()-lsb);

  bitNets.insert(position, bits_.begin()+msbPos, bits_.begin()+lsbPos+1); 
}

void SNLBusNet::setType(const Type& type) {
  std::for_each(bits_.begin(), bits_.end(), [type](SNLBusNetBit* b){ if (b) b->setType(type); });
}

DESIGN_OBJECT_SET_NAME(SNLBusNet, Net, net)

bool SNLBusNet::isAssignConstant() const {
  return std::all_of(bits_.begin(), bits_.end(), [](const SNLBusNetBit* b){ return b->getType().isAssign(); });
}

//LCOV_EXCL_START
const char* SNLBusNet::getTypeName() const {
  return "SNLBusNet";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLBusNet::getString() const {
  return getName().getString() + "[" + std::to_string(getMSB()) + ":" + std::to_string(getLSB()) + "]";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLBusNet::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + name_.getString() + " " + design_->getName().getString() + ">";  
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void SNLBusNet::debugDump(size_t indent, std::ostream& stream) const {
  stream << std::string(" ", indent) << getDescription() << std::endl;
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja
