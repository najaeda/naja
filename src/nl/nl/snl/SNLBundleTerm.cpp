// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLBundleTerm.h"

#include <algorithm>
#include <sstream>

#include "NLException.h"

#include "SNLAttributes.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLMacros.h"

namespace naja::NL {

namespace {

void appendBundleBits(SNLTerm* term, std::vector<SNLBitTerm*>& bits) {
  if (auto scalar = dynamic_cast<SNLScalarTerm*>(term)) {
    bits.push_back(scalar);
    return;
  }
  if (auto bus = dynamic_cast<SNLBusTerm*>(term)) {
    for (auto bit: bus->getBits()) {
      bits.push_back(bit);
    }
    return;
  }
  if (auto bundle = dynamic_cast<SNLBundleTerm*>(term)) {
    for (auto member: bundle->getMembers()) {
      appendBundleBits(member, bits);
    }
  }
}

}

SNLBundleTerm::SNLBundleTerm(SNLDesign* design, Direction direction, const NLName& name):
  super(),
  design_(design),
  name_(name),
  direction_(direction)
{}

SNLBundleTerm::SNLBundleTerm(
  SNLDesign* design,
  NLID::DesignObjectID id,
  Direction direction,
  const NLName& name):
  super(),
  design_(design),
  id_(id),
  name_(name),
  direction_(direction)
{}

SNLBundleTerm* SNLBundleTerm::create(
  SNLDesign* design,
  Direction direction,
  const NLName& name) {
  preCreate(design, name);
  auto* term = new SNLBundleTerm(design, direction, name);
  term->postCreateAndSetID();
  return term;
}

SNLBundleTerm* SNLBundleTerm::create(
  SNLDesign* design,
  NLID::DesignObjectID id,
  Direction direction,
  const NLName& name) {
  preCreate(design, id, name);
  auto* term = new SNLBundleTerm(design, id, direction, name);
  term->postCreate();
  return term;
}

void SNLBundleTerm::preCreate(const SNLDesign* design, const NLName& name) {
  super::preCreate();
  if (not design) {
    throw NLException("malformed SNLBundleTerm creator with NULL design argument");
  }
  if (not design->isPrimitive()) {
    throw NLException("SNLBundleTerm is only supported on primitive designs");
  }
  if (not name.empty() and design->getTerm(name)) {
    std::string reason = "cannot create SNLBundleTerm with name " + name.getString();
    reason += ", a terminal with this name already exists";
    throw NLException(reason);
  }
}

void SNLBundleTerm::preCreate(
  const SNLDesign* design,
  NLID::DesignObjectID id,
  const NLName& name) {
  preCreate(design, name);
  if (design->getTerm(id)) {
    std::string reason = "cannot create SNLBundleTerm with id " + std::to_string(id);
    reason += ", a terminal with this id already exists";
    throw NLException(reason);
  }
}

void SNLBundleTerm::postCreateAndSetID() {
  super::postCreate();
  getDesign()->addTermAndSetID(this);
}

void SNLBundleTerm::postCreate() {
  super::postCreate();
  getDesign()->addTerm(this);
}

void SNLBundleTerm::commonPreDestroy() {
  auto members = members_;
  for (auto* member: members) {
    if (auto* scalar = dynamic_cast<SNLScalarTerm*>(member)) {
      scalar->destroyFromBundle();
    } else if (auto* bus = dynamic_cast<SNLBusTerm*>(member)) {
      bus->destroyFromBundle();
    }
  }
  super::preDestroy();
}

void SNLBundleTerm::destroyFromDesign() {
  getDesign()->terms_.erase(*this);
  commonPreDestroy();
  delete this;
}

void SNLBundleTerm::preDestroy() {
  getDesign()->removeTerm(this);
  commonPreDestroy();
}

SNLTerm* SNLBundleTerm::clone(SNLDesign*) const {
  throw NLException("SNLBundleTerm cloning is handled explicitly by SNLDesign");
}

void SNLBundleTerm::addMember(SNLTerm* member) {
  members_.push_back(member);
  rebuildBits();
}

void SNLBundleTerm::removeMember(SNLTerm* member) {
  auto it = std::find(members_.begin(), members_.end(), member);
  if (it != members_.end()) {
    members_.erase(it);
  }
  rebuildBits();
}

void SNLBundleTerm::rebuildBits() {
  bitTerms_.clear();
  for (auto* member: members_) {
    appendBundleBits(member, bitTerms_);
  }
}

DESIGN_OBJECT_SET_NAME(SNLBundleTerm, Term, term)

void SNLBundleTerm::setNet(SNLNet* net) {
  if (net == nullptr) {
    return;
  }
  throw NLException("SNLBundleTerm cannot be directly connected to a net");
}

NLID::Bit SNLBundleTerm::getWidth() const {
  NLID::Bit width = 0;
  for (auto* member: members_) {
    width += member->getWidth();
  }
  return width;
}

SNLTerm* SNLBundleTerm::getMember(size_t index) const {
  if (index < members_.size()) {
    return members_[index];
  }
  return nullptr;
}

NajaCollection<SNLTerm*> SNLBundleTerm::getMembers() const {
  return NajaCollection(new NajaSTLCollection(&members_));
}

NajaCollection<SNLBitTerm*> SNLBundleTerm::getBits() const {
  return NajaCollection(new NajaSTLCollection(&bitTerms_));
}

NLID SNLBundleTerm::getNLID() const {
  return SNLDesignObject::getNLID(NLID::Type::Term, getID(), 0, 0);
}

//LCOV_EXCL_START
const char* SNLBundleTerm::getTypeName() const {
  return "SNLBundleTerm";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLBundleTerm::getString() const {
  return getName().getString();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLBundleTerm::getDescription() const {
  std::ostringstream stream;
  stream << "<" << std::string(getTypeName());
  if (not isUnnamed()) {
    stream << " " << getName().getString();
  }
  stream << " " << getID();
  if (not getDesign()->isUnnamed()) {
    stream << " " << getDesign()->getName().getString();
  }
  stream << " " << getDesign()->getID();
  stream << ">";
  return stream.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void SNLBundleTerm::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
  if (recursive) {
    for (auto* member: members_) {
      member->debugDump(indent+2, false, stream);
    }
  }
}
//LCOV_EXCL_STOP

bool SNLBundleTerm::deepCompare(const SNLNetComponent* other, std::string& reason) const {
  auto* otherBundle = dynamic_cast<const SNLBundleTerm*>(other);
  if (not otherBundle) {
    reason = "other term is not a SNLBundleTerm";
    return false;
  }
  if (getDirection() != otherBundle->getDirection()) {
    reason = "direction mismatch";
    return false;
  }
  if (getID() != otherBundle->getID()) {
    reason = "ID mismatch";
    return false;
  }
  if (getFlatID() != otherBundle->getFlatID()) {
    reason = "flatID mismatch";
    return false;
  }
  if (getName() != otherBundle->getName()) {
    reason = "name mismatch";
    return false;
  }
  if (members_.size() != otherBundle->members_.size()) {
    reason = "member count mismatch";
    return false;
  }
  for (size_t i=0; i<members_.size(); ++i) {
    if (members_[i] == nullptr or otherBundle->members_[i] == nullptr) {
      reason = "null bundle member";
      return false;
    }
    if (members_[i]->getTypeName() != otherBundle->members_[i]->getTypeName()) {
      reason = "member type mismatch";
      return false;
    }
    std::string memberReason;
    if (not members_[i]->deepCompare(otherBundle->members_[i], memberReason)) {
      reason = "bundle member mismatch: " + memberReason;
      return false;
    }
  }
  return SNLAttributes::compareAttributes(this, other, reason);
}

}  // namespace naja::NL
