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

#include "SNLInstance.h"

namespace naja { namespace SNL {

#if 0

SNLSharedPath::SNLSharedPath(SNLInstance* tailInstance):
  headSharedPath_(nullptr),
  tailInstance_(tailInstance) {
  tailInstance_->addSharedPath(this);
}

SNLSharedPath::SNLSharedPath(SNLSharedPath* headSharedPath, SNLInstance* tailInstance):
  headSharedPath_(headSharedPath) {
#if 0
  tailInstance_(tailInstance),
  if (headSharedPath_ and
      (headSharedPath_->getDesign() not_eq headInstance_->getModel())) {
    //FIXME
  }
  headInstance_->addSharedPath(this);
#endif
}

SNLInstance* SNLSharedPath::getTailInstance() const {
  return tailSharedPath_?tailSharedPath_->getTailInstance():headInstance_;
}

SNLSharedPath* SNLSharedPath::getHeadSharedPath() const {
  return headSharedPath_;
}

SNLSharedPath* SNLSharedPath::getTailSharedPath() const {
  if (not headSharedPath_) {
    return nullptr;
  }

  SNLSharedPath* headSharedPath = headSharedPath_->getHeadSharedPath();
  SNLSharedPath* tailSharedPath = tailInstance_->getSharedPath(headSharedPath);


  fdfdlkfjdsklsjf

  if (not headSharedPath) headSharedPath = new SNLSharedPath(headInstance_, tailSharedPath);
  return headSharedPath;
}

SNLDesign* SNLSharedPath::getDesign() const {
  return headInstance_->getDesign();
}

SNLDesign* SNLSharedPath::getModel() const {
  SNLDesign* model = nullptr;
  SNLSharedPath* sharedPath = const_cast<SNLSharedPath*>(this);
  while (sharedPath) {
    model = sharedPath->getHeadInstance()->getModel();
    sharedPath = sharedPath->getTailSharedPath();
  }
  return model;
}

#endif

}} // namespace SNL // namespace naja
