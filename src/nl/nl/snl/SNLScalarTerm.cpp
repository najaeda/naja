// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLScalarTerm.h"

#include <iostream>
#include <sstream>

#include "NLException.h"

#include "SNLDesign.h"
#include "SNLAttributes.h"
#include "SNLMacros.h"

namespace naja { namespace NL {

SNLScalarTerm::SNLScalarTerm(SNLDesign* design, Direction direction, const NLName& name):
  super(),
  design_(design),
  name_(name),
  direction_(direction)
{}

SNLScalarTerm::SNLScalarTerm(SNLDesign* design, NLID::DesignObjectID id, Direction direction, const NLName& name):
  super(),
  design_(design),
  id_(id),
  name_(name),
  direction_(direction)
{}

SNLScalarTerm* SNLScalarTerm::create(SNLDesign* design, Direction direction, const NLName& name) {
  preCreate(design, name);
  SNLScalarTerm* net = new SNLScalarTerm(design, direction, name);
  net->postCreateAndSetID();
  return net;
}

SNLScalarTerm* SNLScalarTerm::create(SNLDesign* design, NLID::DesignObjectID id, Direction direction, const NLName& name) {
  preCreate(design, id, name);
  SNLScalarTerm* net = new SNLScalarTerm(design, id, direction, name);
  net->postCreate();
  return net;
}

void SNLScalarTerm::preCreate(SNLDesign* design, const NLName& name) {
  super::preCreate();
  if (not design) {
    throw NLException("malformed SNLScalarTerm creator with NULL design argument");
  }
  if (not name.empty() and design->getTerm(name)) {
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLScalarTerm named: " + name.getString();
    throw NLException(reason);
  }
}

void SNLScalarTerm::preCreate(SNLDesign* design, NLID::DesignObjectID id, const NLName& name) {
  preCreate(design, name);
  if (design->getTerm(NLID::DesignObjectID(id))) {
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLScalarTerm with ID: " + std::to_string(id);
    throw NLException(reason);
  }
}

void SNLScalarTerm::postCreateAndSetID() {
  super::postCreate();
  getDesign()->addTermAndSetID(this);
}

void SNLScalarTerm::postCreate() {
  super::postCreate();
  getDesign()->addTerm(this);
}

void SNLScalarTerm::commonPreDestroy() {
#ifdef SNL_DESTROY_DEBUG
  std::cerr << "Destroying " << getDescription() << std::endl; 
#endif
  super::preDestroy();
}

void SNLScalarTerm::destroyFromDesign() {
  commonPreDestroy();
  delete this;
}

void SNLScalarTerm::preDestroy() {
  commonPreDestroy();
  getDesign()->removeTerm(this);
}

SNLTerm* SNLScalarTerm::clone(SNLDesign* design) const {
  auto newScalarTerm = new SNLScalarTerm(design, id_, direction_, name_);
  newScalarTerm->setFlatID(getFlatID());
  SNLAttributes::cloneAttributes(this, newScalarTerm);
  return newScalarTerm;
}

DESIGN_OBJECT_SET_NAME(SNLScalarTerm, Term, term)

NLID SNLScalarTerm::getNLID() const {
  return SNLDesignObject::getNLID(NLID::Type::Term, id_, 0, 0);
}

NajaCollection<SNLBitTerm*> SNLScalarTerm::getBits() const {
  return NajaCollection(new NajaSingletonCollection(const_cast<SNLScalarTerm*>(this))).getParentTypeCollection<SNLBitTerm*>();
}

//LCOV_EXCL_START
const char* SNLScalarTerm::getTypeName() const {
  return "SNLScalarTerm";
}
//LCOV_EXCL_STOP
 
//LCOV_EXCL_START
std::string SNLScalarTerm::getString() const {
  return getName().getString();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string SNLScalarTerm::getDescription() const {
  std::ostringstream stream;
  stream << "<" << std::string(getTypeName());
  if (not isAnonymous()) {
    stream << " " + getName().getString();
  }
  stream << " " << getID();
  if (not getDesign()->isAnonymous()) {
    stream << " " + getDesign()->getName().getString();
  }
  stream << " " << getDesign()->getID();
  stream << ">";
  return stream.str(); 
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void SNLScalarTerm::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
}
//LCOV_EXCL_STOP

bool SNLScalarTerm::deepCompare(const SNLTerm* other, std::string& reason) const {
  const SNLScalarTerm* otherScalarTerm = dynamic_cast<const SNLScalarTerm*>(other);
  if (not otherScalarTerm) {
    //LCOV_EXCL_START
    reason = "other term is not a SNLScalarTerm";
    return false;
    //LCOV_EXCL_STOP
  }
  if (direction_ != otherScalarTerm->direction_) {
    //LCOV_EXCL_START
    reason = "direction mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  if (getID() != otherScalarTerm->getID()) {
    //LCOV_EXCL_START
    reason = "ID mismatch";
    return false;
    //LCOV_EXCL_STOP
  }
  if (getFlatID() != otherScalarTerm->getFlatID()) {
    //LCOV_EXCL_START
    reason = "flatID mismatch between ";
    reason += getString() + " FlatID: " + std::to_string(getFlatID());
    reason += " and " + otherScalarTerm->getString();
    reason += " FlatID: " + std::to_string(otherScalarTerm->getFlatID());
    return false;
    //LCOV_EXCL_STOP
  }
  return SNLAttributes::compareAttributes(this, other, reason);
}

}} // namespace NL // namespace naja