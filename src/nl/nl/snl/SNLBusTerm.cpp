// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBusTerm.h"

#include <cstddef>
#include <sstream>

#include "NLException.h"

#include "SNLDesign.h"
#include "SNLBusTermBit.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"
#include "SNLAttributes.h"
#include "SNLUtils.h"
#include "SNLMacros.h"

namespace naja::NL {

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
    if (bit) {
      bit->destroyFromBus();
    }
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

void SNLBusTerm::removeBit(SNLBusTermBit* bit) {
  size_t pos = bit->getPositionInBus();
  if (pos < bits_.size()) {
    bits_[pos] = nullptr;
  }
  for (auto instance: getDesign()->getSlaveInstances()) {
    instance->removeInstTerm(bit);
  }
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
      if (bit) {
        bit->setNet(nullptr);
      }
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

void SNLBusTerm::setMSB(NLID::Bit msb) {
  if (msb == msb_) {
    return;
  }
  if (not SNLDesign::isBetween(msb, getMSB(), getLSB())) {
    std::ostringstream reason;
    reason << "setMSB error: " << getString() << " cannot be resized to [" << msb << ":" << getLSB() << "]";
    throw NLException(reason.str());
  }
  size_t removeCount = static_cast<size_t>(std::abs(getMSB() - msb));

  std::vector<SNLBusTermBit*> removedBits(bits_.begin(), bits_.begin() + removeCount);
  for (auto bit: removedBits) {
    if (auto net = bit->getNet()) {
      if (not net->isConstant()) {
        std::ostringstream reason;
        reason << "setMSB error: " << bit->getString() << " is connected to a non-constant net";
        throw NLException(reason.str());
      }
      if (not net->getInstTerms().empty()) {
        std::ostringstream reason;
        reason << "setMSB error: " << bit->getString() << " net is connected to instances";
        throw NLException(reason.str());
      }
    }
  }

  for (auto instance: getDesign()->getSlaveInstances()) {
    for (auto bit: removedBits) {
      auto instTerm = instance->getInstTerm(bit);
      if (instTerm and instTerm->getNet()) {
        std::ostringstream reason;
        reason << "setMSB error: " << instTerm->getString() << " is connected";
        throw NLException(reason.str());
      }
    }
  }

  for (auto instance: getDesign()->getSlaveInstances()) {
    for (auto bit: removedBits) {
      instance->removeInstTerm(bit);
    }
    size_t base = getFlatID();
    size_t oldWidth = bits_.size();
    for (size_t i=removeCount; i<oldWidth; ++i) {
      size_t from = base + i;
      size_t to = base + i - removeCount;
      instance->instTerms_[to] = instance->instTerms_[from];
      instance->instTerms_[from] = nullptr;
    }
  }

  for (auto bit: removedBits) {
    bit->setNet(nullptr);
    bit->destroyFromBus();
  }

  bits_.erase(bits_.begin(), bits_.begin() + removeCount);
  msb_ = msb;
}

void SNLBusTerm::setLSB(NLID::Bit lsb) {
  if (lsb == lsb_) {
    return;
  }
  if (not SNLDesign::isBetween(lsb, getMSB(), getLSB())) {
    std::ostringstream reason;
    reason << "setLSB error: " << getString() << " cannot be resized to [" << getMSB() << ":" << lsb << "]";
    throw NLException(reason.str());
  }
  size_t removeCount = static_cast<size_t>(std::abs(getLSB() - lsb));

  auto eraseBegin = bits_.end() - static_cast<std::ptrdiff_t>(removeCount);
  std::vector<SNLBusTermBit*> removedBits(eraseBegin, bits_.end());
  for (auto bit: removedBits) {
    if (auto net = bit->getNet()) {
      if (not net->isConstant()) {
        std::ostringstream reason;
        reason << "setLSB error: " << bit->getString() << " is connected to a non-constant net";
        throw NLException(reason.str());
      }
      if (not net->getInstTerms().empty()) {
        std::ostringstream reason;
        reason << "setLSB error: " << bit->getString() << " net is connected to instances";
        throw NLException(reason.str());
      }
    }
  }

  for (auto instance: getDesign()->getSlaveInstances()) {
    for (auto bit: removedBits) {
      auto instTerm = instance->getInstTerm(bit);
      if (instTerm and instTerm->getNet()) {
        std::ostringstream reason;
        reason << "setLSB error: " << instTerm->getString() << " is connected";
        throw NLException(reason.str());
      }
    }
  }

  for (auto instance: getDesign()->getSlaveInstances()) {
    for (auto bit: removedBits) {
      instance->removeInstTerm(bit);
    }
  }

  for (auto bit: removedBits) {
    bit->setNet(nullptr);
    bit->destroyFromBus();
  }

  bits_.erase(eraseBegin, bits_.end());
  lsb_ = lsb;
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
  auto filter = [](const SNLBusTermBit* bit) { return bit != nullptr; };
  return NajaCollection(new NajaSTLCollection(&bits_)).getSubCollection(filter);
}

NajaCollection<SNLBitTerm*> SNLBusTerm::getBits() const {
  return getBusBits().getParentTypeCollection<SNLBitTerm*>();
}

bool SNLBusTerm::deepCompare(const SNLNetComponent* other, std::string& reason) const {
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

}  // namespace naja::NL
