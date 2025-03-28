// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_SCALAR_TERM_H_
#define __PNL_SCALAR_TERM_H_

#include "PNLBitTerm.h"

namespace naja { namespace NL {

class PNLScalarTerm final: public PNLBitTerm {
  public:
    friend class PNLDesign;
    using super = PNLBitTerm;

    /**
     * \brief Create a PNLScalarTerm.
     * \param design owner PNLDesign.
     * \param direction direction of the term.
     * \param name optional name.
     * \return created PNLScalarTerm. 
     */
    static PNLScalarTerm* create(PNLDesign* design, Direction direction, const NLName& name=NLName());

    /**
     * \brief Create a PNLScalarTerm with a given NLID::DesignObjectID.
     * \param design owner PNLDesign.
     * \param id NLID::DesignObjectID of the instance.
     * \param direction direction of the term.
     * \param name optional name.
     * \return created PNLScalarTerm.
     */
    static PNLScalarTerm* create(PNLDesign* design, NLID::DesignObjectID id, Direction direction, const NLName& name=NLName());

    PNLDesign* getDesign() const override { return design_; }
    NLID getNLID() const override;
    NLID::DesignObjectID getID() const override { return id_; }
    NLID::Bit getBit() const override { return 0; }
    size_t getFlatID() const override { return flatID_; }
    NLName getName() const override { return name_; }

    /// \brief Change the name of this PNLScalarTerm.
    void setName(const NLName& name);

    NajaCollection<PNLBitTerm*> getBits() const override;
    PNLTerm::Direction getDirection() const override { return direction_; }
    void setDirection(const PNLTerm::Direction& direction) { direction_ = direction; }

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
    bool deepCompare(const PNLTerm* other, std::string& reason) const override;
    bool isAnonymous() const override { return name_.empty(); }
  private:
    PNLScalarTerm(PNLDesign* design, Direction direction, const NLName& name);
    PNLScalarTerm(PNLDesign* design, NLID::DesignObjectID, Direction direction, const NLName& name);
    static void preCreate(PNLDesign* design, const NLName& name);
    static void preCreate(PNLDesign* design, NLID::DesignObjectID id, const NLName& name);
    void postCreateAndSetID();
    void postCreate();
    void destroyFromDesign() override;
    PNLTerm* clone(PNLDesign* design) const override;
    void commonPreDestroy();
    void preDestroy() override;

    void setID(NLID::DesignObjectID id) override { id_ = id; }
    void setFlatID(size_t flatID) override {flatID_ = flatID; }

    PNLDesign*            design_;
    NLID::DesignObjectID  id_         {0};
    size_t                flatID_     {0};
    NLName                name_       {};
    PNLTerm::Direction    direction_;  
};

}} // namespace NL // namespace naja

#endif // __PNL_SCALAR_TERM_H_