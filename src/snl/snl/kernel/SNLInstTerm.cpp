// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLInstTerm.h"

#include <sstream>
#include <iostream>

#include "SNLException.h"
#include "SNLDesign.h"
#include "SNLBitTerm.h"
#include "SNLBitNet.h"
#include "SNLMacros.h"

namespace naja { namespace SNL {

SNLInstTerm::SNLInstTerm(SNLInstance* instance, SNLBitTerm* bitTerm):
  instance_(instance),
  bitTerm_(bitTerm)
{}

SNLInstTerm* SNLInstTerm::create(SNLInstance* instance, SNLBitTerm* bitTerm) {
  preCreate(instance, bitTerm);
  SNLInstTerm* instTerm = new SNLInstTerm(instance, bitTerm);
  instTerm->postCreate();
  return instTerm;
}

void SNLInstTerm::preCreate(const SNLInstance* instance, const SNLBitTerm* term) {
  super::preCreate();
}

void SNLInstTerm::postCreate() {
  super::postCreate();
}

void SNLInstTerm::preDestroy() {
#ifdef SNL_DESTROY_DEBUG
  std::cerr << "Destroying " << getDescription() << std::endl; 
#endif
  super::preDestroy();
}

void SNLInstTerm::destroyFromInstance() {
  preDestroy();
  delete this;
}

void SNLInstTerm::destroy() {
  throw SNLException("Unauthorized destroy of SNLInstTerm");
}

SNLDesign* SNLInstTerm::getDesign() const {
  return getInstance()->getDesign();
}

SNLID SNLInstTerm::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::InstTerm, getBitTerm()->getID(), getInstance()->getID(), getBitTerm()->getBit());
}

SNLTerm::Direction SNLInstTerm::getDirection() const {
  return getBitTerm()->getDirection();
}

NET_COMPONENT_SET_NET(SNLInstTerm)

bool SNLInstTerm::isAnonymous() const {
  return getBitTerm()->isAnonymous();
}

void SNLInstTerm::setName(const SNLName& name) {
  throw SNLException("Unauthorized setName of SNLInstTerm");
}

//LCOV_EXCL_START
const char* SNLInstTerm::getTypeName() const {
  return "SNLInstTerm";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLInstTerm::getString() const {
  std::ostringstream str;
  if (not getInstance()->isAnonymous()) {
    str << getInstance()->getName().getString();
  }
  str << "(" << getInstance()->getID() << ")";
  str << ":";
  str << getBitTerm()->getString();
  return str.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLInstTerm::getDescription() const {
  std::ostringstream str;
  str << "<" << getTypeName();
  if (not getInstance()->isAnonymous()) {
    str << " " << getInstance()->getName().getString();
  }
  str << " " << getInstance()->getID();
  if (not getBitTerm()->isAnonymous()) {
    str << " " << getBitTerm()->getString();
  }
  str << " " << getBitTerm()->getID();
  str << ">";
  return str.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void SNLInstTerm::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja
