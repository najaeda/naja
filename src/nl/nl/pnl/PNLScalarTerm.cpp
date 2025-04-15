// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLScalarTerm.h"

#include <iostream>
#include <sstream>

#include "NLException.h"

#include "PNLDesign.h"
//#include "PNLAttributes.h"
#include "SNLMacros.h"

namespace naja { namespace NL {

PNLScalarTerm::PNLScalarTerm(PNLDesign* design, Direction direction, const NLName& name):
  super(),
  design_(design),
  name_(name),
  direction_(direction)
{}

PNLScalarTerm::PNLScalarTerm(PNLDesign* design, NLID::DesignObjectID id, Direction direction, const NLName& name):
  super(),
  design_(design),
  id_(id),
  name_(name),
  direction_(direction)
{}

PNLScalarTerm* PNLScalarTerm::create(PNLDesign* design, Direction direction, const NLName& name) {
  preCreate(design, name);
  PNLScalarTerm* term = new PNLScalarTerm(design, direction, name);
  term->postCreateAndSetID();
  return term;
}

PNLScalarTerm* PNLScalarTerm::create(PNLDesign* design, NLID::DesignObjectID id, Direction direction, const NLName& name) {
  preCreate(design, id, name);
  PNLScalarTerm* term = new PNLScalarTerm(design, id, direction, name);
  term->postCreate();
  return term;
}

void PNLScalarTerm::preCreate(PNLDesign* design, const NLName& name) {
  super::preCreate();
  if (not design) {
    throw NLException("malformed PNLScalarTerm creator with NULL design argument");
  }
  if (not name.empty() and design->getTerm(name)) {
    std::string reason = "PNLDesign " + design->getString() + " contains already a PNLScalarTerm named: " + name.getString();
    throw NLException(reason);
  }
}

void PNLScalarTerm::preCreate(PNLDesign* design, NLID::DesignObjectID id, const NLName& name) {
  preCreate(design, name);
  if (design->getTerm(NLID::DesignObjectID(id))) {
    std::string reason = "PNLDesign " + design->getString() + " contains already a PNLScalarTerm with ID: " + std::to_string(id);
    throw NLException(reason);
  }
}

void PNLScalarTerm::postCreateAndSetID() {
  super::postCreate();
  getDesign()->addTermAndSetID(this);
}

void PNLScalarTerm::postCreate() {
  super::postCreate();
  getDesign()->addTerm(this);
}

void PNLScalarTerm::commonPreDestroy() {
#ifdef PNL_DESTROY_DEBUG
  std::cerr << "Destroying " << getDescription() << std::endl; 
#endif
  super::preDestroy();
}

void PNLScalarTerm::destroyFromDesign() {
  commonPreDestroy();
  delete this;
}

void PNLScalarTerm::preDestroy() {
  commonPreDestroy();
  getDesign()->removeTerm(this);
}

// PNLTerm* PNLScalarTerm::clone(PNLDesign* design) const {
//   auto newScalarTerm = new PNLScalarTerm(design, id_, direction_, name_);
//   newScalarTerm->setFlatID(getFlatID());
//   //PNLAttributes::cloneAttributes(this, newScalarTerm);
//   return newScalarTerm;
// }

DESIGN_OBJECT_SET_NAME(PNLScalarTerm, Term, term)

NLID PNLScalarTerm::getNLID() const {
  return PNLDesignObject::getNLID(NLID::Type::Term, id_, 0, 0);
}

NajaCollection<PNLBitTerm*> PNLScalarTerm::getBits() const {
  return NajaCollection(new NajaSingletonCollection(const_cast<PNLScalarTerm*>(this))).getParentTypeCollection<PNLBitTerm*>();
}

//LCOV_EXCL_START
const char* PNLScalarTerm::getTypeName() const {
  return "PNLScalarTerm";
}
//LCOV_EXCL_STOP
 
//LCOV_EXCL_START
std::string PNLScalarTerm::getString() const {
  return getName().getString();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string PNLScalarTerm::getDescription() const {
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
void PNLScalarTerm::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
}
//LCOV_EXCL_STOP

// bool PNLScalarTerm::deepCompare(const PNLTerm* other, std::string& reason) const {
//   const PNLScalarTerm* otherScalarTerm = dynamic_cast<const PNLScalarTerm*>(other);
//   if (not otherScalarTerm) {
//     //LCOV_EXCL_START
//     reason = "other term is not a PNLScalarTerm";
//     return false;
//     //LCOV_EXCL_STOP
//   }
//   if (direction_ != otherScalarTerm->direction_) {
//     //LCOV_EXCL_START
//     reason = "direction mismatch";
//     return false;
//     //LCOV_EXCL_STOP
//   }
//   if (getID() != otherScalarTerm->getID()) {
//     //LCOV_EXCL_START
//     reason = "ID mismatch";
//     return false;
//     //LCOV_EXCL_STOP
//   }
//   if (getFlatID() != otherScalarTerm->getFlatID()) {
//     //LCOV_EXCL_START
//     reason = "flatID mismatch between ";
//     reason += getString() + " FlatID: " + std::to_string(getFlatID());
//     reason += " and " + otherScalarTerm->getString();
//     reason += " FlatID: " + std::to_string(otherScalarTerm->getFlatID());
//     return false;
//     //LCOV_EXCL_STOP
//   }
//   return true;
//   //return PNLAttributes::compareAttributes(this, other, reason);
// }

}} // namespace NL // namespace naja