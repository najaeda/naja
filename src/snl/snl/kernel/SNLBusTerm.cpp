// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBusTerm.h"

#include "SNLException.h"
#include "SNLDesign.h"
#include "SNLBusTermBit.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLUtils.h"
#include "SNLMacros.h"

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
  size_t size = static_cast<size_t>(getSize());
  bits_.resize(size, nullptr);
  for (size_t i=0; i<size; i++) {
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

SNLTerm* SNLBusTerm::clone(SNLDesign* design) const {
  auto newSNLBusTerm = new SNLBusTerm(design, id_, direction_, msb_, lsb_, name_);
  newSNLBusTerm->createBits();
  return newSNLBusTerm;
}

DESIGN_OBJECT_SET_NAME(SNLBusTerm, Term, term)

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
    throw SNLException("setNet only supported when term and net have same size");
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

SNLID::Bit SNLBusTerm::getSize() const {
  return SNLUtils::getSize(getMSB(), getLSB());
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

//LCOV_EXCL_START
void SNLBusTerm::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
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

NajaCollection<SNLBusTermBit*> SNLBusTerm::getBusBits() const {
  return NajaCollection(new NajaSTLCollection(&bits_));
}

NajaCollection<SNLBitTerm*> SNLBusTerm::getBits() const {
  return getBusBits().getParentTypeCollection<SNLBitTerm*>();
}

}} // namespace SNL // namespace naja
