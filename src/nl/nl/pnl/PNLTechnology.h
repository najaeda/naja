// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_TECHNOLOGY_H_
#define __PNL_TECHNOLOGY_H_

// #include "PNLSite.h"
#include <vector>
#include "PNLBox.h"

namespace naja {
namespace NL {

class PNLSite;
class NLName;

// Singlton class
class PNLTechnology {
 public:
  static PNLTechnology* getOrCreate() {
    if (tech_ == nullptr) {
      tech_ = new PNLTechnology();
    }
    return tech_;
  }

  void addSite(PNLSite* site);

  void removeSite(PNLSite* site) {
    auto it = std::remove(sites_.begin(), sites_.end(), site);
    if (it != sites_.end()) {
      sites_.erase(it, sites_.end());
    }
  }

  const std::vector<PNLSite*>& getSites() const { return sites_; }

  PNLSite* getSiteByName(const NLName& name) const;

//   PNLSite* getSiteByClass(const std::string& siteClass) const;

  PNLBox::Unit getManufacturingGrid() const { return manufacturingGrid_; }
  void setManufacturingGrid(PNLBox::Unit grid) { manufacturingGrid_ = grid; }

 private:
  PNLTechnology() = default;  // Private constructor
  ~PNLTechnology();           // Private destructor

  static PNLTechnology* tech_;

  // Delete copy constructor and assignment operator
  PNLTechnology(const PNLTechnology&) = delete;
  PNLTechnology& operator=(const PNLTechnology&) = delete;

  std::vector<PNLSite*> sites_;

  PNLBox::Unit manufacturingGrid_ = 0;
};

}  // namespace NL
}  // namespace naja

#endif  // __PNL_TECHNOLOGY_H_