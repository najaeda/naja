// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLDesign.h"

namespace naja { namespace SNL {

PNLDesign::PNLDesign(SNLLibrary* library):
  super(), library_(library), origin_(0, 0)
{}

PNLDesign* PNLDesign::create(SNLLibrary* library) {
  preCreate(library);
  auto design = new PNLDesign(library);
  design->postCreate();
  return design;
}

void PNLDesign::preCreate(const SNLLibrary* library) {
  super::preCreate();
}

void PNLDesign::postCreate() {
  super::postCreate();
}

void PNLDesign::preDestroy() {
  super::preDestroy();
}

//LCOV_EXCL_START
const char* PNLDesign::getTypeName() const {
  return "PNLDesign";
}
//LCOV_EXCL_STOP

std::string PNLDesign::getString() const {
  return "PNLDesign";
}

std::string PNLDesign::getDescription() const {
  return "PNLDesign";
}

bool PNLDesign::deepCompare(const PNLDesign* other, std::string& reason) const {
  return false;
}

void PNLDesign::debugDump(size_t indent, bool recursive, std::ostream& stream) const {

}

}} // namespace SNL // namespace naja