// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLInstance.h"

#include <sstream>

#include "PNLDesign.h"

namespace naja { namespace NL {

//LCOV_EXCL_START
const char* PNLInstance::getTypeName() const {
  return "PNLInstance";
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string PNLInstance::getString() const {
  return getName().getString();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
std::string PNLInstance::getDescription() const {
  std::ostringstream description;
  description << "<" << getTypeName();
  description << " " + name_.getString();
  description << " " + std::to_string(getID());
  description << " " + design_->getName().getString();
  description << " " + model_->getName().getString();
  description << ">";
  return description.str();
}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
void PNLInstance::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
}
//LCOV_EXCL_STOP

}} // namespace NL // namespace naja