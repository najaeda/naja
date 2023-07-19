// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLScalarTerm.h"

#include <iostream>
#include <sstream>

#include "SNLException.h"
#include "SNLDesign.h"
#include "SNLMacros.h"

namespace naja { namespace SNL {

SNLScalarTerm::SNLScalarTerm(SNLDesign* design, Direction direction, const SNLName& name):
  super(),
  design_(design),
  name_(name),
  direction_(direction)
{}

SNLScalarTerm::SNLScalarTerm(SNLDesign* design, SNLID::DesignObjectID id, Direction direction, const SNLName& name):
  super(),
  design_(design),
  id_(id),
  name_(name),
  direction_(direction)
{}

SNLScalarTerm* SNLScalarTerm::create(SNLDesign* design, Direction direction, const SNLName& name) {
  preCreate(design, name);
  SNLScalarTerm* net = new SNLScalarTerm(design, direction, name);
  net->postCreateAndSetID();
  return net;
}

SNLScalarTerm* SNLScalarTerm::create(SNLDesign* design, SNLID::DesignObjectID id, Direction direction, const SNLName& name) {
  preCreate(design, id, name);
  SNLScalarTerm* net = new SNLScalarTerm(design, id, direction, name);
  net->postCreate();
  return net;
}

void SNLScalarTerm::preCreate(SNLDesign* design, const SNLName& name) {
  super::preCreate();
  if (not design) {
    throw SNLException("malformed SNLScalarTerm creator with NULL design argument");
  }
  if (not name.empty() and design->getTerm(name)) {
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLScalarTerm named: " + name.getString();
    throw SNLException(reason);
  }
}

void SNLScalarTerm::preCreate(SNLDesign* design, SNLID::DesignObjectID id, const SNLName& name) {
  preCreate(design, name);
  if (design->getTerm(SNLID::DesignObjectID(id))) {
    std::string reason = "SNLDesign " + design->getString() + " contains already a SNLScalarTerm with ID: " + std::to_string(id);
    throw SNLException(reason);
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

DESIGN_OBJECT_SET_NAME(SNLScalarTerm, Term, term)

SNLID SNLScalarTerm::getSNLID() const {
  return SNLDesignObject::getSNLID(SNLID::Type::Term, id_, 0, 0);
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

}} // namespace SNL // namespace naja
