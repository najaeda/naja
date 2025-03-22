// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLDesign.h"

#include <sstream>

#include "NLDB.h"
#include "NLLibrary.h"
#include "NLException.h"


namespace naja { namespace NL {

PNLDesign::PNLDesign(NLLibrary* library, const NLName& name):
  super(),
  name_(name),
  library_(library)
{}

PNLDesign* PNLDesign::create(NLLibrary* library, const NLName& name) {
  preCreate(library, name);
  PNLDesign* design = new PNLDesign(library, name);
  design->postCreateAndSetID();
  return design;
}

//This should be removed tomorrow: PNLDesignID should be NLDesign ID
void PNLDesign::postCreateAndSetID() {
  super::postCreate();
  library_->addPNLDesignAndSetID(this);
}

void PNLDesign::preCreate(const NLLibrary* library, const NLName& name) {
  super::preCreate();
  if (not library) {
    throw NLException("malformed design creator with null library");
  }
  //test if design with same name exists in library
  if (not name.empty() and library->getSNLDesign(name)) {
    std::string reason = "NLLibrary " + library->getString() + " contains already a PNLDesign named: " + name.getString();
    throw NLException(reason);
  }
}

NLDB* PNLDesign::getDB() const {
  return library_->getDB();
}

NLID PNLDesign::getNLID() const {
  return NLID(getDB()->getID(), library_->getID(), getID());
}

void PNLDesign::commonPreDestroy() {
  super::preDestroy();
}

void PNLDesign::destroyFromLibrary() {
  commonPreDestroy();
  delete this;
}

bool PNLDesign::deepCompare(
  const PNLDesign* other,
  std::string& reason,
  NLDesign::CompareType type) const {
  if (type==NLDesign::CompareType::Complete and (getID() not_eq other->getID())) {
    return false; //LCOV_EXCL_LINE
  }
  if (type!=NLDesign::CompareType::IgnoreIDAndName and (name_ not_eq other->getName())) {
    return false; //LCOV_EXCL_LINE
  }
  return true;
}

//LCOV_EXCL_START
const char* PNLDesign::getTypeName() const {
  return "PNLDesign";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string PNLDesign::getString() const {
  if (not isAnonymous()) {
    return getName().getString();
  } else {
    return "<anonymous>";
  }
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string PNLDesign::getDescription() const {
  std::ostringstream stream;
  stream << "<" + std::string(getTypeName());
  if (not isAnonymous()) {
    stream << " " + getName().getString();
  }
  stream << " " << getID();
  if (not getLibrary()->isAnonymous()) {
    stream << " " << getLibrary()->getName().getString();
  }
  stream << " " << getLibrary()->getID();
  stream << ">";
  return stream.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void PNLDesign::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
}
//LCOV_EXCL_STOP

}} // namespace NL // namespace naja