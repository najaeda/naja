// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "PNLBitNet.h"

namespace naja::NL {

class PNLScalarNet final: public PNLBitNet {
  public:
    friend class PNLDesign;
    using super = PNLBitNet;

    /**
     * \brief Create a PNLScalarNet.
     * \param design owner PNLDesign.
     * \param name optional name.
     * \return created PNLScalarNet. 
     */
    static PNLScalarNet* create(PNLDesign* design, const NLName& name=NLName());

    /**
     * \brief Create a PNLScalarNet with a given NLID::DesignObjectID.
     * \param design owner PNLDesign.
     * \param id NLID::DesignObjectID of the instance.
     * \param name optional name.
     * \return created PNLScalarNet.
     */
    static PNLScalarNet* create(PNLDesign* design, NLID::DesignObjectID id, const NLName& name=NLName());

    /// \return this PNLScalarNet owning design.
    PNLDesign* getDesign() const override { return design_; }

    NLID getNLID() const override;
    NLID::DesignObjectID getID() const override { return id_; }

    /// \return this PNLScalarNet name.
    NLName getName() const override { return name_; }
    void setName(const NLName& name) override;
    /// \return true if this PNLScalarNet is unnamed.
    bool isUnnamed() const override { return name_.empty(); }
    NajaCollection<PNLBitNet*> getBits() const override;

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;

    //bool deepCompare(const PNLNet* other, std::string& reason) const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream = std::cerr) const override;
  private:
    PNLScalarNet(PNLDesign* design, const NLName& name);
    PNLScalarNet(PNLDesign* design, NLID::DesignObjectID id, const NLName& name);
    static void preCreate(const PNLDesign* design, const NLName& name);
    static void preCreate(const PNLDesign* design, NLID::DesignObjectID id, const NLName& name);
    void postCreate() override;
    void postCreateAndSetID();
    void destroyFromDesign() override;
    void commonPreDestroy();
    void preDestroy() override;
    //PNLNet* clone(PNLDesign* design) const override;

    void setID(NLID::DesignObjectID id) override { id_ = id; }

    PNLDesign*                          design_;
    NLID::DesignObjectID                id_;
    NLName                              name_;
};

}  // namespace naja::NL