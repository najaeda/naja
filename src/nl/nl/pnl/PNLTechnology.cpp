// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLTechnology.h"

#include "PNLSite.h"

using namespace naja::NL;

PNLTechnology* PNLTechnology::tech_ = nullptr;

void PNLTechnology::addSite(PNLSite* site) {
  site->setID((NLID::DesignObjectID)sites_.size());
  sites_.push_back(site);
}

PNLTechnology::~PNLTechnology() {
  for (auto site : sites_) {
    delete site;
  }
  sites_.clear();
}

PNLSite* PNLTechnology::getSiteByName(const NLName& name) const {
   printf("getSiteByName name %s\n", name.getString().c_str());
  for (auto site : sites_) {
    printf("getSiteByName site %s\n", site->getName().getString().c_str());
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