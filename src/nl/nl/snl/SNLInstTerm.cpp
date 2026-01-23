// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLInstTerm.h"

#include <sstream>

#include "NajaLog.h"

#include "NLException.h"

#include "SNLDesign.h"
#include "SNLBitTerm.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBitNet.h"
#include "SNLMacros.h"

namespace naja::NL {

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
  NAJA_LOG_TRACE("Destroying {}", getDescription());
  super::preDestroy();
}

void SNLInstTerm::destroyFromInstance() {
  preDestroy();
  delete this;
}

void SNLInstTerm::destroy() {
  throw NLException("Unauthorized destroy of SNLInstTerm");
}

SNLDesign* SNLInstTerm::getDesign() const {
  return getInstance()->getDesign();
}

NLID SNLInstTerm::getNLID() const {
  return SNLDesignObject::getNLID(NLID::Type::InstTerm, getBitTerm()->getID(), getInstance()->getID(), getBitTerm()->getBit());
}

SNLTerm::Direction SNLInstTerm::getDirection() const {
  return getBitTerm()->getDirection();
}

NET_COMPONENT_SET_NET(SNLInstTerm)

bool SNLInstTerm::isUnnamed() const {
  return getBitTerm()->isUnnamed();
}

void SNLInstTerm::setName(const NLName& name) {
  throw NLException("Unauthorized setName of SNLInstTerm");
}

//LCOV_EXCL_START
const char* SNLInstTerm::getTypeName() const {
  return "SNLInstTerm";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLInstTerm::getString() const {
  std::ostringstream str;
  str << getInstance()->getString();
  str << "/";
  str << getBitTerm()->getString();
  return str.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLInstTerm::getDescription() const {
  std::ostringstream str;
  str << "<" << getTypeName();
  if (not getInstance()->isUnnamed()) {
    str << " " << getInstance()->getName().getString();
  }
  str << " " << getInstance()->getID();
  if (not getBitTerm()->isUnnamed()) {
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

bool SNLInstTerm::deepCompare(const SNLNetComponent* other, std::string& reason) const {
  const SNLInstTerm* otherInstTerm = dynamic_cast<const SNLInstTerm*>(other);
  if (not otherInstTerm) {
    //LCOV_EXCL_START
    reason = "other term is not a SNLInstTerm";
    return false;
    //LCOV_EXCL_STOP
  }
  if (not instance_->deepCompare(otherInstTerm->getInstance(), reason)) {
    //LCOV_EXCL_START
    reason = "Instance mismatch between ";
    reason += getString() + " and " + otherInstTerm->getString();
    reason += " Instance: " + instance_->getDescription();
    reason += " and Instance: " + otherInstTerm->getInstance()->getDescription();
    reason += " are not the same";
    return false;
    //LCOV_EXCL_STOP
  }
  if (not bitTerm_->deepCompare(otherInstTerm->getBitTerm(), reason)) {
    //LCOV_EXCL_START
    reason = "Instance mismatch between ";
    reason += getString() + " and " + otherInstTerm->getString();
    reason += " Term: " + bitTerm_->getDescription();
    reason += " and Term: " + otherInstTerm->getBitTerm()->getDescription();
    reason += " are not the same";
    return false;
    //LCOV_EXCL_STOP
  }
  return true;
}

}  // namespace naja::NL