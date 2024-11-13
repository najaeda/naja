// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLOccurrence.h"

#include <sstream>

#include "SNLDesign.h"
#include "SNLPath.h"
#include "SNLDesignObject.h"
#include "SNLException.h"

namespace naja { namespace SNL {

SNLOccurrence::SNLOccurrence(SNLDesignObject* object):
  object_(object)
{}

SNLOccurrence::SNLOccurrence(const SNLPath& path, SNLDesignObject* object) {
  if (not path.empty() and object) {
    auto model = path.getModel();
    if (object->getDesign() not_eq model) {
      std::ostringstream reason;
      reason << "Incompatible path: " << path.getString()
        << " and object: " << object->getDescription()
        << " in Occurrence constructor, path model: " 
        << model->getDescription() << " is not object design: "
        << object->getDesign()->getDescription();
      throw SNLException(reason.str());
    }
  }
  path_ = path.getSharedPath();
  object_ = object;
}

bool SNLOccurrence::operator==(const SNLOccurrence& rhs) const {
  if (getObject() == nullptr) {
    return rhs.getObject() == nullptr;
  }
  if (rhs.getObject() == nullptr) {
    return false;
  }
  return (*getObject() == *rhs.getObject()) and (getPath() == rhs.getPath());
}

bool SNLOccurrence::operator<(const SNLOccurrence& rhs) const {
  //First start by comparing objects (through their SNLIDs)
  //If equal compare their path. Paths can be nullptr
  if (getObject() == nullptr) {
    return rhs.getObject() not_eq nullptr;
  }
  if (rhs.getObject() == nullptr) {
    return false;
  }
  return (*getObject() < *rhs.getObject()) or
    ((*getObject() == *rhs.getObject()) and (getPath() < rhs.getPath()));
}

bool SNLOccurrence::operator<=(const SNLOccurrence& rhs) const {
  return (*this < rhs) or (*this == rhs);
}

bool SNLOccurrence::operator>(const SNLOccurrence& rhs) const {
  return not (*this <= rhs);
}

bool SNLOccurrence::operator>=(const SNLOccurrence& rhs) const {
  return not (*this < rhs);
}

SNLPath SNLOccurrence::getPath() const {
  if (path_) {
    return SNLPath(path_);
  }
  return SNLPath();
}

std::string SNLOccurrence::getString(const char separator) const {
  std::ostringstream oss;
  oss << getPath().getString(separator);
  if (object_) {
    oss << separator << object_->getString();
  }
  return oss.str();
}

std::string SNLOccurrence::getDescription() const {
  std::ostringstream oss;
  oss << "Occurrence: ";
  if (object_) {
    //LCOV_EXCL_START
    oss << object_->getDescription();
    //LCOV_EXCL_STOP
  } else {
    oss << "null";
  }
  oss << " at path: ";
  oss << getPath().getString();
  return oss.str();
}

}} // namespace SNL // namespace naja
