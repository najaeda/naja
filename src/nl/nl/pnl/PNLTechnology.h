// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_TECHNOLOGY_H_
#define __PNL_TECHNOLOGY_H_

#include <vector>
#include "PNLSite.h"
#include "PNLBox.h"

namespace naja {
namespace NL {

class NLName;
class NLUniverse;

class PNLTechnology {
 public:
  static PNLTechnology* create(NLUniverse* universe);

  const std::vector<PNLSite*>& getSites() const { return sites_; }

  PNLSite* getSiteByName(const NLName& name) const;

//   PNLSite* getSiteByClass(const std::string& siteClass) const;

  PNLBox::Unit getManufacturingGrid() const { return manufacturingGrid_; }
  void setManufacturingGrid(PNLBox::Unit grid) { manufacturingGrid_ = grid; }

 private:
  friend class NLUniverse;
  friend class PNLSite;
  explicit PNLTechnology(NLUniverse* owner): owner_(owner) {}
  ~PNLTechnology();

  // Delete copy constructor and assignment operator
  PNLTechnology(const PNLTechnology&) = delete;
  PNLTechnology& operator=(const PNLTechnology&) = delete;

  std::vector<PNLSite*> sites_;

  PNLBox::Unit manufacturingGrid_ = 0;
  NLUniverse* owner_ {nullptr};

  void addSite(PNLSite* site);
};

}  // namespace NL
}  // namespace naja

#endif  // __PNL_TECHNOLOGY_H_
