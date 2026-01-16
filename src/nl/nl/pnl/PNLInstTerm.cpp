// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLInstTerm.h"

#include <sstream>

#include "NajaLog.h"

#include "NLException.h"

#include "PNLDesign.h"
#include "PNLBitTerm.h"
#include "PNLBitNet.h"
#include "SNLMacros.h"

namespace naja { namespace NL {

PNLInstTerm::PNLInstTerm(PNLInstance* instance, PNLBitTerm* bitTerm):
  instance_(instance),
  bitTerm_(bitTerm)
{}

PNLInstTerm* PNLInstTerm::create(PNLInstance* instance, PNLBitTerm* bitTerm) {
  preCreate(instance, bitTerm);
  PNLInstTerm* instTerm = new PNLInstTerm(instance, bitTerm);
  instTerm->postCreate();
  return instTerm;
}

void PNLInstTerm::preCreate(const PNLInstance* instance, const PNLBitTerm* term) {
  super::preCreate();
}

void PNLInstTerm::postCreate() {
  super::postCreate();
}

void PNLInstTerm::preDestroy() {
  NAJA_LOG_TRACE("Destroying {}", getDescription());
  super::preDestroy();
}

void PNLInstTerm::destroyFromInstance() {
  preDestroy();
  delete this;
}

void PNLInstTerm::destroy() {
  throw NLException("Unauthorized destroy of PNLInstTerm");
}

PNLDesign* PNLInstTerm::getDesign() const {
  return getInstance()->getDesign();
}

NLID PNLInstTerm::getNLID() const {
  return PNLDesignObject::getNLID(NLID::Type::InstTerm, getBitTerm()->getID(), getInstance()->getID(), getBitTerm()->getBit());
}

PNLTerm::Direction PNLInstTerm::getDirection() const {
  return getBitTerm()->getDirection();
}

PNL_NET_COMPONENT_SET_NET(PNLInstTerm)

//LCOV_EXCL_START
const char* PNLInstTerm::getTypeName() const {
  return "PNLInstTerm";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string PNLInstTerm::getString() const {
  std::ostringstream str;
  str << getInstance()->getString();
  str << "/";
  str << getBitTerm()->getString();
  return str.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string PNLInstTerm::getDescription() const {
  std::ostringstream str;
  str << "<" << getTypeName();
  // if (not getInstance()->isAnonymous()) {
  //   str << " " << getInstance()->getName().getString();
  // }
  str << " " << getInstance()->getID();
  // if (not getBitTerm()->isAnonymous()) {
  //   str << " " << getBitTerm()->getString();
  // }
  str << " " << getBitTerm()->getID();
  str << ">";
  return str.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void PNLInstTerm::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
}
//LCOV_EXCL_STOP

bool PNLInstTerm::isUnnamed() const {
  return getBitTerm()->isUnnamed();
}

NLName PNLInstTerm::getName() const {
  return getBitTerm()->getName();
}

}} // namespace NL // namespace naja
