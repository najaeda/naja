// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLTechnology.h"
#include "NLException.h"
#include "NLUniverse.h"

using namespace naja::NL;

PNLTechnology::~PNLTechnology() {
  for (auto site : sites_) {
    delete site;
  }
  sites_.clear();
}

void PNLTechnology::addSite(PNLSite* site) {
  site->setID((NLID::DesignObjectID)sites_.size());
  sites_.push_back(site);
}

PNLTechnology* PNLTechnology::create(NLUniverse* universe) {
  if (not universe) {
    throw NLException("PNLTechnology::create: null universe");
  }
  if (universe->technology_) {
    throw NLException("PNLTechnology already exists");
  }
  universe->technology_ = new PNLTechnology(universe);
  return universe->technology_;
}

PNLSite* PNLTechnology::getSiteByName(const NLName& name) const {
  for (auto site : sites_) {
    if (site->getName() == name) {
      return site;
    }
  }
  return nullptr;
}

// PNLSite* PNLTechnology::getSiteByClass(const std::string& siteClass) const {
//   for (auto site : sites_) {
//     if (site->getClass() == siteClass) {
//       return site;
//     }
//   }
//   return nullptr;
// }
