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

#include "SNLNetComponent.h"

#include "SNLBitNet.h"
#include "SNLDesign.h"
#include "SNLException.h"

namespace naja { namespace SNL {

SNLNetComponent::Direction::Direction(const DirectionEnum& dirEnum):
  dirEnum_(dirEnum) 
{}

//LCOV_EXCL_START
std::string SNLNetComponent::Direction::getString() const {
  switch (dirEnum_) {
    case Direction::Input: return "Input";
    case Direction::Output: return "Output";
    case Direction::InOut: return "InOut";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

void SNLNetComponent::preCreate() {
  super::preCreate();
}

void SNLNetComponent::postCreate() {
  super::postCreate();
}

void SNLNetComponent::preDestroy() {
  setNet(nullptr);
  super::preDestroy();
}

}} // namespace SNL // namespace naja