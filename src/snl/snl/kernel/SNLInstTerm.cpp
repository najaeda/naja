// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
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

SNLInstTerm::SNLInstTerm(SNLInstance* instance, SNLBitTerm* term):
  instance_(instance),
  term_(term)
{}

SNLInstTerm* SNLInstTerm::create(SNLInstance* instance, SNLBitTerm* term) {
  preCreate(instance, term);
  SNLInstTerm* instTerm = new SNLInstTerm(instance, term);
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
  return SNLDesignObject::getSNLID(SNLID::Type::InstTerm, getTerm()->getID(), getInstance()->getID(), getTerm()->getBit());
}

SNLTerm::Direction SNLInstTerm::getDirection() const {
  return getTerm()->getDirection();
}

NET_COMPONENT_SET_NET(SNLInstTerm)

bool SNLInstTerm::isAnonymous() const {
  return getTerm()->isAnonymous();
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
  str << getTerm()->getString();
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
  if (not getTerm()->isAnonymous()) {
    str << " " << getTerm()->getString();
  }
  str << " " << getTerm()->getID();
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
