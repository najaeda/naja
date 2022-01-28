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

#include "SNLBusTermBit.h"

#include <sstream>

#include "SNLException.h"
#include "SNLBusTerm.h"

namespace SNL {

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
  return SNLDesignObject::getSNLID(SNLID::Type::Net, getID(), 0, getBit());
}

size_t SNLBusTermBit::getPosition() const {
  return getBus()->getPosition() + size_t(std::abs(getBit() - getBus()->getMSB()));
}

SNLDesign* SNLBusTermBit::getDesign() const {
  return getBus()->getDesign();
}

constexpr const char* SNLBusTermBit::getTypeName() const {
  return "SNLBusTermBit";
}

SNLName SNLBusTermBit::getName() const {
  return getBus()->getName();
}

SNLTerm::Direction SNLBusTermBit::getDirection() const {
  return getBus()->getDirection();
}

std::string SNLBusTermBit::getString() const {
  std::ostringstream str;
  if (not getBus()->isAnonymous()) {
    str << getBus()->getName().getString();
  }
  str << "(" << getBus()->getID() << ")";
  str << "[" << getBit() << "]";
  return str.str();
}

std::string SNLBusTermBit::getDescription() const {
  return "";
}

bool SNLBusTermBit::isAnonymous() const {
  return getBus()->isAnonymous();
}

}
