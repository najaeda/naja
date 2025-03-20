// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLDesign.h"

#include <sstream>

#include "NLLibrary.h"

namespace naja { namespace NL {

void PNLDesign::destroyFromLibrary() {
  //commonPreDestroy();
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