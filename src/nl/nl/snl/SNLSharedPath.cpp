// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLSharedPath.h"

#include <sstream>

#include "NLException.h"

#include "SNLDesign.h"

namespace naja { namespace NL {

SNLSharedPath::SNLSharedPath(SNLInstance* tailInstance, SNLSharedPath* headSharedPath):
  headSharedPath_(headSharedPath),
  tailInstance_(tailInstance) {
  if (headSharedPath_ and
    (headSharedPath_->getModel() not_eq tailInstance_->getDesign())) {
    std::ostringstream stream;
    stream << "Cannot construct Path with incompatible headPath: ";
    stream << headSharedPath_->getString();
    stream << " with model " << headSharedPath_->getModel()->getString();
    stream << " and " << tailInstance->getString();
    stream << " with parent design: " << tailInstance->getDesign()->getString();
    throw NLException(stream.str());
  }
  tailInstance_->addSharedPath(this);
}

SNLInstance* SNLSharedPath::getHeadInstance() const {
  return headSharedPath_?headSharedPath_->getHeadInstance():tailInstance_;
}

SNLSharedPath* SNLSharedPath::getTailSharedPath() const {
  if (not headSharedPath_) {
    return nullptr;
  }

  SNLSharedPath* headSharedPath = headSharedPath_->getTailSharedPath();
  SNLSharedPath* tailSharedPath = tailInstance_->getSharedPath(headSharedPath);

  if (not tailSharedPath) {
    tailSharedPath = new SNLSharedPath(tailInstance_, headSharedPath);
  }
  return tailSharedPath;
}

void SNLSharedPath::commonDestroy() {
#if 0 //need to confirm with example
  SNLDesign* design = tailInstance_->getDesign();
#ifdef SNL_DESTROY_DEBUG
  std::cerr << "SNLSharedPath::commonDestroy: " << getString() << std::endl;
  std::cerr << "tailInstance design: " << design->getString() << std::endl;
#endif
  for (auto instance: design->getSlaveInstances()) {
#ifdef SNL_DESTROY_DEBUG
    std::cerr << "sharedInstance: " << instance->getString() << std::endl;
#endif
    auto sharedPath = instance->getSharedPath(this);
    if (sharedPath) {
#ifdef SNL_DESTROY_DEBUG
      std::cerr << "found sharedPath: " << sharedPath->getString() << std::endl;
#endif
      sharedPath->destroy();
    }
  }
#endif
}

#if 0
void SNLSharedPath::destroy() {
  tailInstance_->removeSharedPath(this);
  commonDestroy();
  delete this;
}
#endif

void SNLSharedPath::destroyFromInstance() {
  //No need to remove from instance as owner instance is deleted
  commonDestroy();
  delete this;
}

SNLDesign* SNLSharedPath::getDesign() const {
  SNLDesign* design = nullptr;
  SNLSharedPath* sharedPath = const_cast<SNLSharedPath*>(this);
  while (sharedPath) {
    design = sharedPath->getTailInstance()->getDesign();
    sharedPath = sharedPath->getHeadSharedPath();
  }
  return design;
}

SNLDesign* SNLSharedPath::getModel() const {
  return tailInstance_->getModel();
}

size_t SNLSharedPath::size() const {
  if (headSharedPath_) {
    return headSharedPath_->size() + 1;
  }
  return 1;
}

//LCOV_EXCL_START
std::string SNLSharedPath::getString(char separator) {
  if (headSharedPath_) {
    if (tailInstance_) {
      std::ostringstream stream;
      stream << headSharedPath_->getString(separator) << separator;
      if (tailInstance_->isUnnamed()) {
        stream << "<anon:" << tailInstance_->getID() << ">";
      } else {
        stream << tailInstance_->getName().getString();
      }
      return stream.str();
    }
  }
  if (tailInstance_) {
    if (tailInstance_->isUnnamed()) {
      return "<anon:" + std::to_string(tailInstance_->getID()) + ">";
    } else {
      return tailInstance_->getName().getString();
    }
  }
  return "";
}
//LCOV_EXCL_STOP

std::vector<NLID::DesignObjectID> SNLSharedPath::getIDs() const {
  std::vector<NLID::DesignObjectID> result;
  if (headSharedPath_ != nullptr) {
    result = headSharedPath_->getIDs();
    result.push_back(tailInstance_->getID());
    return result;
  }
  if (tailInstance_ != nullptr) {
    result.push_back(tailInstance_->getID());
    return result;
  }
  return result;
}

std::vector<SNLInstance*> SNLSharedPath::getInstances() const {
  std::vector<SNLInstance*> result;
  if (headSharedPath_ != nullptr) {
    result = headSharedPath_->getInstances();
    result.push_back(tailInstance_);
    return result;
  }
  if (tailInstance_ != nullptr) {
    result.push_back(tailInstance_);
    return result;
  }
  return result;
}

}} // namespace NL // namespace naja
