// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PNLSite.h"
#include "PNLTechnology.h"

using namespace naja::NL;

PNLSite* PNLSite::create(const NLName& name, ClassType siteClass,
                  const PNLUnit::Unit& width,
                  const PNLUnit::Unit& height) {
    PNLSite* site = new PNLSite();
    site->name_ = name;
    site->width_ = width;
    site->height_ = height;
    site->class_ = siteClass;
    PNLTechnology::getOrCreate()->addSite(site);
    return site;
  }