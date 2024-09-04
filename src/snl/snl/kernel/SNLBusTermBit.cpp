// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBusTermBit.h"

#include <sstream>

#include "SNLException.h"
#include "SNLBusTerm.h"

namespace naja { namespace SNL {

SNLBusTermBit::SNLBusTermBit(
    SNLBusTerm* bus,
    SNLID::Bit bit):
  super(),
  bus_(bus),
  bit_(bit)
{}

SNLBusTermBit* SNLBusTermBit::create(SNLBusTerm* bus, SNLID::Bit bit) {
  preCreate(bus, bit);
  SNLBusTermBit* busTermBit = new SNLBusTermBit(bus, bit);
  busTermBit->postCreate();
  return busTermBit;
}

void SNLBusTermBit::preCreate(const SNLBusTerm* bus, SNLID::Bit bit) {
  super::preCreate();
}

void SNLBusTermBit::postCreate() {
  super::postCreate();
}

void SNLBusTermBit::destroyFromBus() {
  preDestroy();
  delete this;
}

void SNLBusTermBit::destroy() {
  throw SNLException("Unauthorized destroy of SNLBusTermBit");
}

void SNLBusTermBit::preDestroy() {
  super::preDestroy();
}

SNLID::DesignObjectID SNLBusTermBit::getID() const {
  return getBus()->getID();
}

SNLID SNLBusTermBit::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::TermBit, getID(), 0, getBit());
}

size_t SNLBusTermBit::getFlatID() const {
  return getBus()->getFlatID() + getPositionInBus();
}

size_t SNLBusTermBit::getPositionInBus() const {
  return size_t(std::abs(getBit() - getBus()->getMSB())); 
}

NajaCollection<SNLBitTerm*> SNLBusTermBit::getBits() const {
  return NajaCollection(new NajaSingletonCollection(const_cast<SNLBusTermBit*>(this))).getParentTypeCollection<SNLBitTerm*>();
}

SNLDesign* SNLBusTermBit::getDesign() const {
  return getBus()->getDesign();
}

SNLName SNLBusTermBit::getName() const {
  return getBus()->getName();
}

SNLTerm::Direction SNLBusTermBit::getDirection() const {
  return getBus()->getDirection();
}

//LCOV_EXCL_START
const char* SNLBusTermBit::getTypeName() const {
  return "SNLBusTermBit";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLBusTermBit::getString() const {
  return getBus()->getName().getString() + "[" + std::to_string(getBit()) + "]";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLBusTermBit::getDescription() const {
  std::ostringstream stream;
  stream << "<" << std::string(getTypeName()) << " ";
  if (not getBus()->isAnonymous()) {
    stream << getBus()->getName().getString();
  } else {
    stream << "(" << getBus()->getID() << ")";
  }
  stream << "[" << getBit() << "]";
  stream << ">";
  return stream.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void SNLBusTermBit::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
}
//LCOV_EXCL_STOP

bool SNLBusTermBit::isAnonymous() const {
  return getBus()->isAnonymous();
}

void SNLBusTermBit::setName(const SNLName& name) {
  throw SNLException("Unauthorized setName of SNLBusTermBit");  
}

bool SNLBusTermBit::deepCompare(const SNLTerm* other, std::string& reason) const {
  const SNLBusTermBit* otherBusTermBit = dynamic_cast<const SNLBusTermBit*>(other);
  if (not otherBusTermBit) {
    //LCOV_EXCL_START
    reason = "other term is not a SNLBusTermBit";
    return false;
    //LCOV_EXCL_STOP
  }
  if (getBit() not_eq otherBusTermBit->getBit()) {
    //LCOV_EXCL_START
    reason = "bit mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  if (getFlatID() not_eq otherBusTermBit->getFlatID()) {
    //LCOV_EXCL_START
    reason = "flatID mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  return true;
}

}} // namespace SNL // namespace naja