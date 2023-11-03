// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLNetComponentOccurrence.h"

#include "SNLNetComponent.h"
#include "SNLPath.h"
#include "SNLBitNetOccurrence.h"

namespace naja { namespace SNL {

SNLNetComponentOccurrence::SNLNetComponentOccurrence(SNLNetComponent* component):
  super(component)
{}

SNLNetComponentOccurrence::SNLNetComponentOccurrence(const SNLPath& path, SNLNetComponent* component):
  super(path, component)
{}

SNLNetComponent* SNLNetComponentOccurrence::getComponent() const {
  return static_cast<SNLNetComponent*>(getObject());
}

SNLBitNetOccurrence SNLNetComponentOccurrence::getNetOccurrence() const {
  auto net = getNet();
  if (net) {
    return SNLBitNetOccurrence(getPath(), net);
  }
  return SNLBitNetOccurrence();
}

SNLBitNet* SNLNetComponentOccurrence::getNet() const {
  auto component = getComponent();
  if (component) {
    return component->getNet();
  }
  return nullptr;
}

}} // namespace SNL // namespace naja
