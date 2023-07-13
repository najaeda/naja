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

#include "SNLSharedPath.h"

#include "SNLDesign.h"
#include "SNLException.h"

namespace naja { namespace SNL {

SNLSharedPath::SNLSharedPath(SNLInstance* tailInstance, SNLSharedPath* headSharedPath):
  headSharedPath_(headSharedPath),
  tailInstance_(tailInstance) {
  if (headSharedPath_ and
    (headSharedPath_->getModel() not_eq tailInstance_->getDesign())) {
    throw SNLException("Cannot construct Path with incomaptible headPath and tailInstance");
  }
  if (headSharedPath_) {
    key_ = headSharedPath_->getHeadInstance()->getSNLID();
  } else {
    key_ = tailInstance_->getSNLID();
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
  SNLDesign* design = tailInstance_->getDesign();
  for (auto instance: design->getSlaveInstances()) {
    auto sharedPath = instance->getSharedPath(this);
    if (sharedPath) {
      sharedPath->destroy();
    }
  }
}

void SNLSharedPath::destroy() {
  tailInstance_->removeSharedPath(this);
  commonDestroy();
  delete this;
}

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

}} // namespace SNL // namespace naja
