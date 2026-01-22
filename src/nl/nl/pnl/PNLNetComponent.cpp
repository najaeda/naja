// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLNetComponent.h"

#include "PNLBitNet.h"
#include "PNLDesign.h"

namespace naja::NL {

PNLNetComponent::Direction::Direction(const DirectionEnum& dirEnum):
  dirEnum_(dirEnum) 
{}

//LCOV_EXCL_START
std::string PNLNetComponent::Direction::getString() const {
  switch (dirEnum_) {
    case Direction::Input: return "Input";
    case Direction::Output: return "Output";
    case Direction::InOut: return "InOut";
    case Direction::Tristate: return "Tristate";
    case Direction::Undefined: return "Undefined";
  }
  return "Unknown";
}
//LCOV_EXCL_STOP

void PNLNetComponent::preCreate() {
  super::preCreate();
}

void PNLNetComponent::postCreate() {
  super::postCreate();
}

void PNLNetComponent::preDestroy() {
  setNet(nullptr);
  super::preDestroy();
}

}  // namespace naja::NL