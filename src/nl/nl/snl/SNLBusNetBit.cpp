// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBusNetBit.h"

#include "NajaCollection.h"
#include "NLException.h"

#include "SNLBusNet.h"

namespace naja::NL {

SNLBusNetBit::SNLBusNetBit(
    SNLBusNet* bus,
    NLID::Bit bit):
  super(),
  bus_(bus),
  bit_(bit)
{}

SNLBusNetBit* SNLBusNetBit::create(SNLBusNet* bus, NLID::Bit bit) {
  preCreate(bus, bit);
  SNLBusNetBit* busNetBit = new SNLBusNetBit(bus, bit);
  busNetBit->postCreate();
  return busNetBit;
}

void SNLBusNetBit::preCreate(const SNLBusNet* bus, NLID::Bit bit) {
  super::preCreate();
}

void SNLBusNetBit::postCreate() {
  super::postCreate();
}

void SNLBusNetBit::commonPreDestroy() {
  super::preDestroy();
}

void SNLBusNetBit::destroyFromBus() {
  commonPreDestroy();
  delete this;
}

void SNLBusNetBit::preDestroy() {
  commonPreDestroy();
  getBus()->removeBit(this);
}

NLID::DesignObjectID SNLBusNetBit::getID() const {
  return getBus()->getID();
}

NLID SNLBusNetBit::getNLID() const {
  return SNLDesignObject::getNLID(NLID::Type::NetBit, getBus()->getID(), 0, getBit());
}

SNLDesign* SNLBusNetBit::getDesign() const {
  return getBus()->getDesign();
}

NajaCollection<SNLBitNet*> SNLBusNetBit::getBits() const {
  return NajaCollection(new NajaSingletonCollection(const_cast<SNLBusNetBit*>(this))).getParentTypeCollection<SNLBitNet*>();
}

//LCOV_EXCL_START
const char* SNLBusNetBit::getTypeName() const {
  return "SNLBusNetBit";
}
//LCOV_EXCL_STOP

NLName SNLBusNetBit::getName() const {
  return getBus()->getName();
}

//LCOV_EXCL_START
std::string SNLBusNetBit::getString() const {
  return getBus()->getName().getString() + "[" + std::to_string(getBit()) + "]";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLBusNetBit::getDescription() const {
  return "<" + std::string(getTypeName()) + " " + getBus()->getName().getString() + "[" + std::to_string(getBit()) + "]>";
}
//LCOV_EXCL_STOP

bool SNLBusNetBit::deepCompare(const SNLNet* other, std::string& reason) const {
  const SNLBusNetBit* otherBusNetBit = dynamic_cast<const SNLBusNetBit*>(other);
  if (not otherBusNetBit) {
    //LCOV_EXCL_START
    reason = "other term is not a SNLBusNetBit";
    return false;
    //LCOV_EXCL_STOP
  }
  if (getBit() not_eq otherBusNetBit->getBit()) {
    //LCOV_EXCL_START
    reason = "bit mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  return true;
}

//LCOV_EXCL_START
void SNLBusNetBit::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
  if (recursive and not getComponents().empty()) {
    stream << std::string(indent+2, ' ') << "<components>" << std::endl;
    for (auto component: getComponents()) {
      component->debugDump(indent+4, false, stream);
    }
    stream << std::string(indent+2, ' ') << "</components>" << std::endl;
  }
}
//LCOV_EXCL_STOP

bool SNLBusNetBit::isUnnamed() const {
  return getBus()->isUnnamed();
}

void SNLBusNetBit::setName(const NLName& name) {
  throw NLException("Unauthorized setName of SNLBusNetBit");  
}

}  // namespace naja::NL