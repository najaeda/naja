// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBusTerm.h"

#include "NLException.h"

#include "SNLDesign.h"
#include "SNLBusTermBit.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLAttributes.h"
#include "SNLUtils.h"
#include "SNLMacros.h"

namespace naja { namespace SNL {

SNLBusTerm::SNLBusTerm(
    SNLDesign* design,
    Direction direction,
    NLID::Bit msb,
    NLID::Bit lsb,
    const NLName& name):
  super(),
  design_(design),
  name_(name),
  direction_(direction),
  msb_(msb),
  lsb_(lsb)
{}

SNLBusTerm::SNLBusTerm(
    SNLDesign* design,
    NLID::DesignObjectID id,
    Direction direction,
    NLID::Bit msb,
    NLID::Bit lsb,
    const NLName& name):
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
    NLID::Bit msb,
    NLID::Bit lsb,
    const NLName& name) {
  preCreate(design, name);
  SNLBusTerm* term = new SNLBusTerm(design, direction, msb, lsb, name);
  term->postCreateAndSetID();
  return term;
}

SNLBusTerm* SNLBusTerm::create(
    SNLDesign* design,
    NLID::DesignObjectID id,
    Direction direction,
    NLID::Bit msb,
    NLID::Bit lsb,
    const NLName& name) {
  preCreate(design, id, name);
  SNLBusTerm* term = new SNLBusTerm(design, id, direction, msb, lsb, name);
  term->postCreate();
  return term;
}

void SNLBusTerm::preCreate(const SNLDesign* design, const NLName& name) {
  super::preCreate();
  if (not design) {
    throw NLException("malformed SNLBusTerm creator with NULL design argument");
  }
  //verify that there is not an instance of name in this design
  if (not name.empty() and design->getTerm(name)) {
    std::string reason = "cannot create SNLBusTerm with name " + name.getString();
    reason += "A terminal with this name already exists.";
    throw NLException(reason);
  }
}

void SNLBusTerm::preCreate(const SNLDesign* design, NLID::DesignObjectID id, const NLName& name) {
  preCreate(design, name);
  if (design->getTerm(id)) {
    std::string reason = "cannot create SNLBusTerm with id " + std::to_string(id);
    reason += "A terminal with this id already exists.";
    throw NLException(reason);
  }
}

void SNLBusTerm::createBits() {
  size_t size = static_cast<size_t>(getWidth());
  bits_.resize(size, nullptr);
  for (size_t i=0; i<size; i++) {
    NLID::Bit bit = (getMSB()>getLSB())?getMSB()-int(i):getMSB()+int(i);
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
  newSNLBusTerm->setFlatID(getFlatID());
  newSNLBusTerm->createBits();
  SNLAttributes::cloneAttributes(this, newSNLBusTerm);
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
    throw NLException("setNet error: incompatible term and net");
  }
  if (getWidth() not_eq net->getWidth()) {
    throw NLException("setNet only supported when term and net have same width");
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

NLID::Bit SNLBusTerm::getWidth() const {
  return SNLUtils::getWidth(getMSB(), getLSB());
}

NLID SNLBusTerm::getNLID() const {
  return SNLDesignObject::getNLID(NLID::Type::Term, id_, 0, 0);
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

SNLBusTermBit* SNLBusTerm::getBit(NLID::Bit bit) const {
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

bool SNLBusTerm::deepCompare(const SNLTerm* other, std::string& reason) const {
  const SNLBusTerm* otherBusTerm = dynamic_cast<const SNLBusTerm*>(other);
  if (not otherBusTerm) {
    //LCOV_EXCL_START
    reason = "other term is not a SNLBusTerm";
    return false;
    //LCOV_EXCL_STOP
  }
  if (getDirection() not_eq otherBusTerm->getDirection()) {
    //LCOV_EXCL_START
    reason = "direction mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  if (getMSB() not_eq otherBusTerm->getMSB()) {
    //LCOV_EXCL_START
    reason = "MSB mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  if (getLSB() not_eq otherBusTerm->getLSB()) {
    //LCOV_EXCL_START
    reason = "LSB mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  if (getName() not_eq otherBusTerm->getName()) {
    //LCOV_EXCL_START
    reason = "name mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  if (bits_.size() not_eq otherBusTerm->bits_.size()) {
    //LCOV_EXCL_START
    reason = "size mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  if (getFlatID() not_eq otherBusTerm->getFlatID()) {
    //LCOV_EXCL_START
    reason = "flatID mismatch between ";
    reason += getString() + " FlatID: " + std::to_string(getFlatID());
    reason += " and " + otherBusTerm->getString();
    reason += " FlatID: " + std::to_string(otherBusTerm->getFlatID());
    return false;
    //LCOV_EXCL_STOP
  }
  for (size_t i=0; i<bits_.size(); i++) {
    if (not bits_[i] and not otherBusTerm->bits_[i]) {
      continue;
    } else if (not bits_[i] or not otherBusTerm->bits_[i]) {
      //LCOV_EXCL_START
      reason = "bit mismatch";
      return false;
      //LCOV_EXCL_STOP
    }
    if (not bits_[i]->deepCompare(otherBusTerm->bits_[i], reason)) {
      return false; //LCOV_EXCL_LINE
    }
  }
  return SNLAttributes::compareAttributes(this, other, reason);
}

}} // namespace SNL // namespace naja
