#include "SNLBusNetBit.h"

#include <iostream>

#include "SNLBusNet.h"
#include "SNLException.h"

namespace SNL {

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
  return SNLDesignObject::getSNLID(SNLID::Type::Net, getBus()->getID(), 0, getBit());
}

SNLDesign* SNLBusNetBit::getDesign() const {
  return getBus()->getDesign();
}

constexpr const char* SNLBusNetBit::getTypeName() const {
  return "SNLBusNetBit";
}

SNLName SNLBusNetBit::getName() const {
  return getBus()->getName();
}

std::string SNLBusNetBit::getString() const {
  return std::string();
}

std::string SNLBusNetBit::getDescription() const {
  return "";
}

bool SNLBusNetBit::isAnonymous() const {
  return getBus()->isAnonymous();
}

}
