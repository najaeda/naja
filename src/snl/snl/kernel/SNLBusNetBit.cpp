// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBusNetBit.h"

#include <iostream>

#include "NajaCollection.h"
#include "SNLBusNet.h"
#include "SNLException.h"

namespace naja { namespace SNL {

SNLBusNetBit::SNLBusNetBit(
    SNLBusNet* bus,
    SNLID::Bit bit):
  super(),
  bus_(bus),
  bit_(bit)
{}

SNLBusNetBit* SNLBusNetBit::create(SNLBusNet* bus, SNLID::Bit bit) {
  preCreate(bus, bit);
  SNLBusNetBit* busNetBit = new SNLBusNetBit(bus, bit);
  busNetBit->postCreate();
  return busNetBit;
}

void SNLBusNetBit::preCreate(const SNLBusNet* bus, SNLID::Bit bit) {
  super::preCreate();
}

void SNLBusNetBit::postCreate() {
  super::postCreate();
}

void SNLBusNetBit::destroyFromBus() {
  preDestroy();
  delete this;
}

void SNLBusNetBit::destroy() {
  throw SNLException("Unexpected call of SNLBusNetBit::destroy()");
}

void SNLBusNetBit::preDestroy() {
  super::preDestroy();
}

SNLID::DesignObjectID SNLBusNetBit::getID() const {
  return getBus()->getID();
}

SNLID SNLBusNetBit::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::NetBit, getBus()->getID(), 0, getBit());
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

SNLName SNLBusNetBit::getName() const {
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

bool SNLBusNetBit::isAnonymous() const {
  return getBus()->isAnonymous();
}

}} // namespace SNL // namespace naja
