/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SNLPath.h"

#include "SNLSharedPath.h"
#include "SNLInstance.h"
#include "SNLException.h"

namespace naja { namespace SNL {

SNLPath::SNLPath(SNLSharedPath* sharedPath):
  sharedPath_(sharedPath)
{}

SNLPath::SNLPath(const SNLPath& path):
  sharedPath_(path.sharedPath_)
{}

SNLPath::SNLPath(SNLInstance* instance): SNLPath() {
  if (instance) {
    sharedPath_ = instance->getSharedPath(nullptr);
    if (not sharedPath_) {
      sharedPath_ = new SNLSharedPath(instance);
    }
  }
}

SNLPath::SNLPath(SNLInstance* headInstance, const SNLPath& tailPath): SNLPath() {
  if (not headInstance) {
    throw SNLException("cannot create SNLPath with null head instance");
  }

  if (not tailPath.sharedPath_) {
    sharedPath_ = headInstance->getSharedPath(nullptr);
    if (not sharedPath_) {
      sharedPath_ = new SNLSharedPath(headInstance);
    }
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
    sharedPath_ = tailInstance->getSharedPath(nullptr);
    if (not sharedPath_) {
      sharedPath_ = new SNLSharedPath(tailInstance);
    }
  } else { 
    SNLSharedPath* headSharedPath = headPath.sharedPath_;
    if (headSharedPath->getModel() not_eq tailInstance->getDesign()) {
      throw SNLException("Cannot create SNLPath: incompatible headPath");
    }

    sharedPath_ = tailInstance->getSharedPath(headSharedPath);
    if (not sharedPath_) {
      sharedPath_ = new SNLSharedPath(tailInstance, headSharedPath);
    }
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

SNLDesign* SNLPath::getDesign() const {
  return sharedPath_?sharedPath_->getDesign():nullptr; 
}

SNLDesign* SNLPath::getModel() const {
  return sharedPath_?sharedPath_->getModel():nullptr; 
}

bool SNLPath::operator==(const SNLPath& path) const {
  return sharedPath_ == path.sharedPath_;
}

bool SNLPath::operator!=(const SNLPath& path) const {
  return sharedPath_ != path.sharedPath_;
}

}} // namespace SNL // namespace naja
