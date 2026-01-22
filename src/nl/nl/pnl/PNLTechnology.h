// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLObject.h"
#include "PNLSite.h"
#include "PNLBox.h"

namespace naja::NL {

class NLName;
class NLUniverse;

class PNLTechnology: public NLObject {
  public:
    using super = NLObject;
    static PNLTechnology* create(NLUniverse* universe);

    const std::vector<PNLSite*>& getSites() const { return sites_; }

    PNLSite* getSiteByName(const NLName& name) const;

//   PNLSite* getSiteByClass(const std::string& siteClass) const;

    PNLBox::Unit getManufacturingGrid() const { return manufacturingGrid_; }
    void setManufacturingGrid(PNLBox::Unit grid) { manufacturingGrid_ = grid; }

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

  private:
    friend class NLUniverse;
    friend class PNLSite;
    
    explicit PNLTechnology(NLUniverse* owner): owner_(owner) {}
    ~PNLTechnology();

    // Delete copy constructor and assignment operator
    PNLTechnology(const PNLTechnology&) = delete;
    PNLTechnology& operator=(const PNLTechnology&) = delete;

    static void preCreate(NLUniverse* universe);
    void postCreate() override;
    void preDestroy() override;

    std::vector<PNLSite*> sites_;

    PNLBox::Unit manufacturingGrid_ = 0;
    NLUniverse* owner_ {nullptr};

    void addSite(PNLSite* site);
};

}  // namespace naja::NL
