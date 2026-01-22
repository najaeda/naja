// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLSite.h"

#include "NLException.h"
#include "PNLTechnology.h"

namespace naja::NL {

PNLSite* PNLSite::create(PNLTechnology* owner,
                         const NLName& name,
                         ClassType siteClass,
                         const PNLBox::Unit& width,
                         const PNLBox::Unit& height) {
  if (not owner) {
    throw NLException("PNLSite::create: null technology");
  }
  auto site = new PNLSite(owner);
  site->name_ = name;
  site->width_ = width;
  site->height_ = height;
  site->class_ = siteClass;
  owner->addSite(site);
  return site;
}

}  // namespace naja::NL
