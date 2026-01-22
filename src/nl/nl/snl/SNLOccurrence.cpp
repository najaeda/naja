// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLOccurrence.h"

#include <sstream>

#include "NLException.h"

#include "SNLDesign.h"
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"
#include "SNLPath.h"

namespace naja::NL {

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
      throw NLException(reason.str());
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
  //First start by comparing objects (through their NLIDs)
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

bool SNLOccurrence::isNetComponentOccurrence() const {
  return dynamic_cast<SNLNetComponent*>(getObject()) != nullptr;
}

SNLNetComponent* SNLOccurrence::getNetComponent() const {
  return dynamic_cast<SNLNetComponent*>(getObject());
}

SNLInstTerm* SNLOccurrence::getInstTerm() const {
  return dynamic_cast<SNLInstTerm*>(getObject());
}

SNLBitTerm* SNLOccurrence::getBitTerm() const {
  return dynamic_cast<SNLBitTerm*>(getObject());
}

SNLOccurrence SNLOccurrence::getComponentBitNetOccurrence() const {
  auto net = getComponentBitNet();
  if (net) {
    return SNLOccurrence(getPath(), net);
  }
  return SNLOccurrence();
}

SNLBitNet* SNLOccurrence::getComponentBitNet() const {
  auto component = dynamic_cast<SNLNetComponent*>(getObject());
  if (component) {
    return component->getNet();
  }
  return nullptr;
}

std::string SNLOccurrence::getString(const char separator) const {
  std::ostringstream oss;
  oss << getPath().getString(separator);
  if (object_) {
    if (not oss.str().empty()) {
      oss << separator;
    }
    oss << object_->getString();
  }
  return oss.str();
}

//LCOV_EXCL_START
std::string SNLOccurrence::getDescription() const {
  std::ostringstream oss;
  oss << "Occurrence: ";
  if (object_) {
    oss << object_->getDescription();
  } else {
    oss << "null";
  }
  oss << " at path: ";
  oss << getPath().getString();
  return oss.str();
}
//LCOV_EXCL_STOP

}  // namespace naja::NL