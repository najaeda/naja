// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PNLDesignObject.h"
#include "NLID.h"
#include "NLName.h"

namespace naja { namespace NL {

class PNLInstance final: public PNLDesignObject {
  public:
    friend class PNLDesign;
    using super = PNLDesignObject;
    PNLDesign* getDesign() const override { return design_; }
    /// \return the instanciated SNLDesign (model).
    PNLDesign* getModel() const { return model_; }

    /// \return this PNLInstance id. Positional id in parent PNLDesign.
    NLID::DesignObjectID getID() const { return id_; }
    /// \return this PNLInstance name.
    NLName getName() const { return name_; }

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
  private:
    PNLDesign*                          design_                 {nullptr};
    PNLDesign*                          model_                  {nullptr};
    NLID::DesignObjectID                id_;
    NLName                              name_                   {};
    boost::intrusive::set_member_hook<> designInstancesHook_    {};
};

}} // namespace NL // namespace naja