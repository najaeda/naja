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

#include "SNLBusTerm.h"

#include "SNLException.h"
#include "SNLDesign.h"
#include "SNLBusTermBit.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"

namespace naja { namespace SNL {

SNLBusTerm::SNLBusTerm(
    SNLDesign* design,
    Direction direction,
    SNLID::Bit msb,
    SNLID::Bit lsb,
    const SNLName& name):
  super(),
  design_(design),
  name_(name),
  direction_(direction),
  msb_(msb),
  lsb_(lsb)
{}

SNLBusTerm::SNLBusTerm(
    SNLDesign* design,
    SNLID::DesignObjectID id,
    Direction direction,
    SNLID::Bit msb,
    SNLID::Bit lsb,
    const SNLName& name):
  super(),
  design_(design),
  id_(id),
  name_(name),
  direction_(direction),
  msb_(msb),
  lsb_(lsb)
{}

SNLBusTerm* SNLBusTerm::create(
    SNLDesign* design,
    Direction direction,
    SNLID::Bit msb,
    SNLID::Bit lsb,
    const SNLName& name) {
  preCreate(design, name);
  SNLBusTerm* term = new SNLBusTerm(design, direction, msb, lsb, name);
  term->postCreateAndSetID();
  return term;
}

SNLBusTerm* SNLBusTerm::create(
    SNLDesign* design,
    SNLID::DesignObjectID id,
    Direction direction,
    SNLID::Bit msb,
    SNLID::Bit lsb,
    const SNLName& name) {
  preCreate(design, id, name);
  SNLBusTerm* term = new SNLBusTerm(design, id, direction, msb, lsb, name);
  term->postCreate();
  return term;
}

void SNLBusTerm::preCreate(const SNLDesign* design, const SNLName& name) {
  super::preCreate();
  if (not design) {
    throw SNLException("malformed SNLBusTerm creator with NULL design argument");
  }
  //verify that there is not an instance of name in this design
  if (not name.empty() and design->getTerm(name)) {
    std::string reason = "cannot create SNLBusTerm with name " + name.getString();
    reason += "A terminal with this name already exists.";
    throw SNLException(reason);
  }
}

void SNLBusTerm::preCreate(const SNLDesign* design, SNLID::DesignObjectID id, const SNLName& name) {
  preCreate(design, name);
  if (design->getTerm(id)) {
    std::string reason = "cannot create SNLBusTerm with id " + std::to_string(id);
    reason += "A terminal with this id already exists.";
    throw SNLException(reason);
  }
}

void SNLBusTerm::createBits() {
  bits_.resize(getSize(), nullptr);
  for (size_t i=0; i<getSize(); i++) {
    SNLID::Bit bit = (getMSB()>getLSB())?getMSB()-int(i):getMSB()+int(i);
    bits_[i] = SNLBusTermBit::create(this, bit);
  }
}

void SNLBusTerm::postCreateAndSetID() {
  super::postCreate();
  createBits();
  getDesign()->addTermAndSetID(this);
}

void SNLBusTerm::postCreate() {
  super::postCreate();
  createBits();
  getDesign()->addTerm(this);
}

void SNLBusTerm::commonPreDestroy() {
  super::preDestroy();
  for (SNLBusTermBit* bit: bits_) {
    bit->destroyFromBus();
  }
}

void SNLBusTerm::destroyFromDesign() {
  commonPreDestroy();
  delete this;
}

void SNLBusTerm::preDestroy() {
  getDesign()->removeTerm(this);
  commonPreDestroy();
}

void SNLBusTerm::setNet(SNLNet* net) {
  if (not net) {
    //disconnect all
    for (SNLBusTermBit* bit: bits_) {
      bit->setNet(nullptr);
    }
    return;
  }
  if (getDesign() not_eq net->getDesign()) {
    throw SNLException("setNet error: incompatible term and net");
  }
  if (getSize() not_eq net->getSize()) {
    throw SNLException("setNet only supported when term and bit have same size");
  }
  if (auto bitNet = dynamic_cast<SNLBitNet*>(net)) {
    getBit(getMSB())->setNet(bitNet);
  } else {
    auto busNet = static_cast<SNLBusNet*>(net);
    auto termIt = getMSB();
    auto netIt = busNet->getMSB();
    auto termStop = getMSB()>getLSB()?-1:1;
    auto netStop = busNet->getMSB()>busNet->getLSB()?-1:1;
    while (termIt != getLSB()+termStop and netIt != busNet->getLSB()+netStop) {
      auto bit = getBit(termIt);
      if (bit) {
        bit->setNet(busNet->getBit(netIt));
      }
      getMSB()>getLSB()?--termIt:++termIt;
      busNet->getMSB()>busNet->getLSB()?--netIt:++netIt;
    }
  }
}

size_t SNLBusTerm::getSize() const {
  return static_cast<size_t>(std::abs(getLSB() - getMSB()) + 1);
}

SNLID SNLBusTerm::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::Term, id_, 0, 0);
}

//LCOV_EXCL_START
const char* SNLBusTerm::getTypeName() const {
  return "SNLBusTerm";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLBusTerm::getString() const {
  return getName().getString() + "[" + std::to_string(getMSB()) + ":" + std::to_string(getLSB()) + "]";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLBusTerm::getDescription() const {
  return "<"
    + std::string(getTypeName())
    + " " + name_.getString()
    + " [" + std::to_string(getMSB()) + ":" + std::to_string(getLSB()) + "]"
    + " " + direction_.getString()
    + " " + design_->getName().getString() + ">";  
}
//LCOV_EXCL_STOP

SNLBusTermBit* SNLBusTerm::getBit(SNLID::Bit bit) const {
  if (SNLDesign::isBetween(bit, getMSB(), getLSB())) {
    size_t pos = static_cast<size_t>(std::abs(getMSB()-bit));
    return getBitAtPosition(pos);
  }
  return nullptr;
}

SNLBusTermBit* SNLBusTerm::getBitAtPosition(size_t position) const {
  if (position < bits_.size()) {
    return bits_[position];
  }
  return nullptr;
}

NajaCollection<SNLBusTermBit*> SNLBusTerm::getBits() const {
  return NajaCollection(new NajaSTLCollection(&bits_));
}

}} // namespace SNL // namespace naja
