// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

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