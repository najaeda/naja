// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PNLDesignObject.h"
#include "NLID.h"
#include "NLName.h"
#include "PNLInstTerm.h"
#include "PNLTransform.h"

namespace naja { namespace NL {

class PNLBitTerm;

class PNLInstance final: public PNLDesignObject {
  public:
    friend class PNLDesign;
    using super = PNLDesignObject;
    using PNLInstanceInstTerms = std::vector<PNLInstTerm*>;

    enum PlacementStatus {UNPLACED=0, PLACED=1, FIXED=2};

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
    PNLInstTerm* getInstTerm(const PNLBitTerm* term) const;
    PNLInstTerm* getInstTerm(const NLID::DesignObjectID id) const;

    bool isAnonymous() const override { return name_.empty(); }

    bool isBlackBox() const;
    bool isPrimitive() const;
    bool isLeaf() const;

    void destroyFromDesign();
    void destroyFromModel();

    void setTransform(const PNLTransform& transform) { transform_ = transform; }
    const PNLTransform& getTransform() const { return transform_; }

    void setPlacementStatus(PlacementStatus status) { placementStatus_ = status; }
    PlacementStatus getPlacementStatus() const { return placementStatus_; }

    void setOrigin(const PNLPoint& origin) { origin_ = origin; }
    const PNLPoint& getOrigin() const { return origin_; }

    void setType(Type type) { type_ = type; }
    Type getType() const { return type_; }

  private:
  
    PNLInstance(PNLDesign* design, PNLDesign* model, const NLName& name);
    static void preCreate(PNLDesign* design, const PNLDesign* model, const NLName& name);
    void commonPostCreate();
    void postCreateAndSetID();
    void postCreate();
    void createInstTerm(PNLBitTerm* term);
    void removeInstTerm(PNLBitTerm* term);
    void commonPreDestroy();
    void preDestroy() override;

    PNLDesign*                          design_                 {nullptr};
    PNLDesign*                          model_                  {nullptr};
    NLID::DesignObjectID                id_;
    PNLInstanceInstTerms                instTerms_              {};
    NLName                              name_                   {};
    PNLPoint                            origin_                 {0, 0};
    PNLTransform                        transform_;
    PlacementStatus                     placementStatus_        {UNPLACED};
    boost::intrusive::set_member_hook<> designInstancesHook_    {};
    boost::intrusive::set_member_hook<> designSlaveInstancesHook_ {};
    
};

}} // namespace NL // namespace naja