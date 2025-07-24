// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_SCALAR_NET_H_
#define __SNL_SCALAR_NET_H_

#include "SNLBitNet.h"

namespace naja { namespace NL {

class SNLScalarNet final: public SNLBitNet {
  public:
    friend class SNLDesign;
    using super = SNLBitNet;

    /**
     * \brief Create a SNLScalarNet.
     * \param design owner SNLDesign.
     * \param name optional name.
     * \return created SNLScalarNet. 
     */
    static SNLScalarNet* create(SNLDesign* design, const NLName& name=NLName());

    /**
     * \brief Create a SNLScalarNet with a given NLID::DesignObjectID.
     * \param design owner SNLDesign.
     * \param id NLID::DesignObjectID of the instance.
     * \param name optional name.
     * \return created SNLScalarNet.
     */
    static SNLScalarNet* create(SNLDesign* design, NLID::DesignObjectID id, const NLName& name=NLName());

    /// \return this SNLScalarNet owning design.
    SNLDesign* getDesign() const override { return design_; }

    NLID getNLID() const override;
    NLID::DesignObjectID getID() const override { return id_; }

    /// \return this SNLScalarNet name.
    NLName getName() const override { return name_; }
    /// \return true if this SNLScalarNet is unnamed.
    bool isUnnamed() const override { return name_.empty(); }
    void setName(const NLName& name) override;
    NajaCollection<SNLBitNet*> getBits() const override;

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;

    bool deepCompare(const SNLNet* other, std::string& reason) const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream = std::cerr) const override;
  private:
    SNLScalarNet(SNLDesign* design, const NLName& name);
    SNLScalarNet(SNLDesign* design, NLID::DesignObjectID id, const NLName& name);
    static void preCreate(const SNLDesign* design, const NLName& name);
    static void preCreate(const SNLDesign* design, NLID::DesignObjectID id, const NLName& name);
    void postCreate();
    void postCreateAndSetID();
    void destroyFromDesign() override;
    void commonPreDestroy();
    void preDestroy() override;
    SNLNet* clone(SNLDesign* design) const override;

    void setID(NLID::DesignObjectID id) override { id_ = id; }

    SNLDesign*                          design_;
    NLID::DesignObjectID                id_;
    NLName                              name_;
};

}} // namespace NL // namespace naja

#endif // __SNL_SCALAR_NET_H_
