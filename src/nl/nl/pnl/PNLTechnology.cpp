// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLTechnology.h"
#include "NLException.h"
#include "NLUniverse.h"

using namespace naja::NL;

PNLTechnology::~PNLTechnology() = default;

void PNLTechnology::preCreate(NLUniverse* universe) {
  super::preCreate();
  if (not universe) {
    throw NLException("PNLTechnology::create: null universe");
  }
  if (universe->technology_) {
    throw NLException("PNLTechnology already exists");
  }
}

void PNLTechnology::postCreate() {
  super::postCreate();
  owner_->technology_ = this;
}

void PNLTechnology::preDestroy() {
  for (auto site : sites_) {
    delete site;
  }
  sites_.clear();
  if (owner_ && owner_->technology_ == this) {
    owner_->technology_ = nullptr;
  }
  super::preDestroy();
}

void PNLTechnology::addSite(PNLSite* site) {
  site->setID((NLID::DesignObjectID)sites_.size());
  sites_.push_back(site);
}

PNLTechnology* PNLTechnology::create(NLUniverse* universe) {
  preCreate(universe);
  PNLTechnology* technology = new PNLTechnology(universe);
  technology->postCreate();
  return technology;
}

PNLSite* PNLTechnology::getSiteByName(const NLName& name) const {
  for (auto site : sites_) {
    if (site->getName() == name) {
      return site;
    }
  }
  return nullptr;
}

// LCOV_EXCL_START
const char* PNLTechnology::getTypeName() const {
  return "PNLTechnology";
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
std::string PNLTechnology::getString() const {
  return getTypeName();
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
std::string PNLTechnology::getDescription() const {
  return "<" + std::string(getTypeName()) + ">";
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START
void PNLTechnology::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  stream << std::string(indent, ' ') << getDescription() << std::endl;
  if (recursive) {
    for (auto site : sites_) {
      stream << std::string(indent + 2, ' ')
             << "<PNLSite " << site->getName().getString() << " " << site->getID() << ">"
             << std::endl;
    }
  }
}
// LCOV_EXCL_STOP

// PNLSite* PNLTechnology::getSiteByClass(const std::string& siteClass) const {
//   for (auto site : sites_) {
//     if (site->getClass() == siteClass) {
//       return site;
//     }
//   }
//   return nullptr;
// }
