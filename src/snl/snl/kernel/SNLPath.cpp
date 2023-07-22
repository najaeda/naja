// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLPath.h"

#include <sstream>

#include "SNLDesign.h"
#include "SNLSharedPath.h"
#include "SNLInstance.h"
#include "SNLException.h"

namespace naja { namespace SNL {

SNLPath::SNLPath(SNLSharedPath* sharedPath):
  sharedPath_(sharedPath)
{}

SNLSharedPath* SNLPath::createInstanceSharedPath(SNLInstance* instance) {
  auto sharedPath = instance->getSharedPath(nullptr);
  if (not sharedPath) {
    sharedPath = new SNLSharedPath(instance);
  }
  return sharedPath;
}

SNLPath::SNLPath(SNLInstance* instance): SNLPath() {
  if (instance) {
    sharedPath_ = createInstanceSharedPath(instance);
  }
}

SNLPath::SNLPath(SNLInstance* headInstance, const SNLPath& tailPath): SNLPath() {
  if (not headInstance) {
    throw SNLException("cannot create SNLPath with null head instance");
  }

  if (not tailPath.sharedPath_) {
    sharedPath_ = createInstanceSharedPath(headInstance);
  } else {
    SNLInstance* tailInstance = tailPath.getTailInstance();
    SNLSharedPath* headSharedPath = SNLPath(headInstance, tailPath.getHeadPath()).sharedPath_;
    sharedPath_ = tailInstance->getSharedPath(headSharedPath);
    if (not sharedPath_) {
      sharedPath_ = new SNLSharedPath(tailInstance, headSharedPath);
    }
  }
}

SNLPath::SNLPath(const SNLPath& headPath, SNLInstance* tailInstance): SNLPath() {
  if (not tailInstance) {
    throw SNLException("Cannot create SNLPath with null tailInstance");
  }

  if (not headPath.sharedPath_) {
    sharedPath_ = createInstanceSharedPath(tailInstance);
  } else { 
    SNLSharedPath* headSharedPath = headPath.sharedPath_;
    if (headSharedPath->getModel() not_eq tailInstance->getDesign()) {
      std::ostringstream error;
      error << "Cannot create SNLPath with incompatible headPath: ";
      error << headSharedPath->getString() << " with model ";
      error << headSharedPath->getModel()->getString();
      error << " and tail instance: " << tailInstance->getDescription();
      error << " with design: " << tailInstance->getDesign()->getString();
      throw SNLException(error.str());
    }

    sharedPath_ = tailInstance->getSharedPath(headSharedPath);
    if (not sharedPath_) {
      sharedPath_ = new SNLSharedPath(tailInstance, headSharedPath);
    }
  }
}

SNLPath::SNLPath(const SNLDesign* top, const PathStringDescriptor& descriptor): SNLPath() {
  if (top and not descriptor.empty()) {
    using Instances = std::vector<SNLInstance*>;
    Instances instances;
    auto design = top;
    for (auto instanceName: descriptor) {
      if (instanceName.empty()) {
        throw SNLException("Anonymous instance in SNLPath constructor.");
      }
      auto instance = design->getInstance(SNLName(instanceName));
      if (not instance) {
        throw SNLException("Unfound instance in SNLPath constructor.");
      }
      instances.push_back(instance);
      design = instance->getModel();
    }
    SNLPath path;
    for (auto instance: instances) {
      path = SNLPath(path, instance);
    }
    sharedPath_ = path.sharedPath_;
  }
}

SNLInstance* SNLPath::getHeadInstance() const {
  return sharedPath_?sharedPath_->getHeadInstance():nullptr;
}

SNLPath SNLPath::getTailPath() const {
  return SNLPath(sharedPath_?sharedPath_->getTailSharedPath():nullptr);
}

SNLPath SNLPath::getHeadPath() const {
  return SNLPath(sharedPath_?sharedPath_->getHeadSharedPath():nullptr);
}

SNLInstance* SNLPath::getTailInstance() const {
  return sharedPath_?sharedPath_->getTailInstance():nullptr;
}

bool SNLPath::empty() const {
  return not sharedPath_;
}

size_t SNLPath::size() const {
  if (sharedPath_) {
    return sharedPath_->size();
  }
  return 0;
}

SNLDesign* SNLPath::getDesign() const {
  return sharedPath_?sharedPath_->getDesign():nullptr; 
}

SNLDesign* SNLPath::getModel() const {
  return sharedPath_?sharedPath_->getModel():nullptr; 
}

bool SNLPath::operator==(const SNLPath& path) const {
  return sharedPath_ == path.sharedPath_;
}

bool SNLPath::operator<(const SNLPath& path) const {
  if (sharedPath_) {
    if (path.sharedPath_) {
      //both non null
      //start by comparing sizes
      auto thisSize = size();
      auto otherSize = path.size();
      if (thisSize not_eq otherSize) {
        return thisSize < otherSize;
      } else {
        //same size... compare instances one by one
        auto thisSharedPath = sharedPath_;
        auto otherSharedPath = path.sharedPath_;
        while (thisSharedPath) {
          auto thisTailInstance = thisSharedPath->getTailInstance();
          auto otherTailInstance = otherSharedPath->getTailInstance();
          if (thisTailInstance < otherTailInstance) {
            return true;
          }
          thisSharedPath = thisSharedPath->getHeadSharedPath();
          otherSharedPath = otherSharedPath->getHeadSharedPath();
        }
        return false;
      }
    } else {
      return false;
    }
  } else {
    //this is empty path
    //if other path is non null => true
    //if other path is null => false
    return path.sharedPath_ != nullptr;
  }
  return false;
}

//LCOV_EXCL_START
std::string SNLPath::getString(const char separator) {
  if (sharedPath_) {
    return "<" + sharedPath_->getString(separator) + ">";
  }
  return "<>";
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja