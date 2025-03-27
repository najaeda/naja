// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PNLDesignObject.h"
#include "NLID.h"
#include "NLName.h"
#include "PNLInstTerm.h"

namespace naja { namespace NL {

class PNLBitTerm;

class PNLInstance final: public PNLDesignObject {
  public:
    friend class PNLDesign;
    using super = PNLDesignObject;
    using PNLInstanceInstTerms = std::vector<PNLInstTerm*>;

    static PNLInstance* create(PNLDesign* design, PNLDesign* model, const NLName& name=NLName());

    PNLDesign* getDesign() const override { return design_; }
    /// \return the instanciated PNLDesign (model).
    PNLDesign* getModel() const { return model_; }

    /// \return this PNLInstance id. Positional id in parent PNLDesign.
    NLID::DesignObjectID getID() const { return id_; }
    /// \return this PNLInstance name.
    NLName getName() const { return name_; }

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

    naja::NL::NLID getNLID() const override;

    void createInstTerm(PNLBitTerm* term);
    void removeInstTerm(PNLBitTerm* term);

  private:
    PNLInstance(PNLDesign* design, PNLDesign* model, const NLName& name);
    static void preCreate(PNLDesign* design, const PNLDesign* model, const NLName& name);
    void commonPostCreate();
    void postCreateAndSetID();
    void postCreate();
    void commonPreDestroy();
    void preDestroy() override;

    PNLDesign*                          design_                 {nullptr};
    PNLDesign*                          model_                  {nullptr};
    NLID::DesignObjectID                id_;
    NLName                              name_                   {};
    boost::intrusive::set_member_hook<> designInstancesHook_    {};
    boost::intrusive::set_member_hook<> designSlaveInstancesHook_ {};
    PNLInstanceInstTerms               instTerms_              {};
};

}} // namespace NL // namespace naja