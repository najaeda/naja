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
  return "";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void SNLBusNetBit::debugDump(size_t indent, std::ostream& stream) const {
}
//LCOV_EXCL_STOP

bool SNLBusNetBit::isAnonymous() const {
  return getBus()->isAnonymous();
}

}} // namespace SNL // namespace naja
