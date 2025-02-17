// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLInstance.h"

namespace naja { namespace PNL {

PNLInstance::PNLInstance(PNLDesign* design, PNLDesign* model):
  super(), design_(design), model_(model)
{}

PNLInstance* PNLInstance::create(PNLDesign* design, PNLDesign* model) {
  preCreate(design, model);
  PNLInstance* instance = new PNLInstance(design, model);
  instance->postCreate();
  return instance;
}

void PNLInstance::preCreate(PNLDesign* design, const PNLDesign* model) {
    super::preCreate();
}

void PNLInstance::postCreate() {
    super::postCreate();
}

void PNLInstance::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
}

}} // namespace SNL // namespace naja