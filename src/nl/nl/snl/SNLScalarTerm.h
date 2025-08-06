// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_SCALAR_TERM_H_
#define __SNL_SCALAR_TERM_H_

#include "SNLBitTerm.h"

namespace naja { namespace NL {

class SNLScalarTerm final: public SNLBitTerm {
  public:
    friend class SNLDesign;
    using super = SNLBitTerm;

    /**
     * \brief Create a SNLScalarTerm.
     * \param design owner SNLDesign.
     * \param direction direction of the term.
     * \param name optional name.
     * \return created SNLScalarTerm. 
     */
    static SNLScalarTerm* create(SNLDesign* design, Direction direction, const NLName& name=NLName());

    /**
     * \brief Create a SNLScalarTerm with a given NLID::DesignObjectID.
     * \param design owner SNLDesign.
     * \param id NLID::DesignObjectID of the instance.
     * \param direction direction of the term.
     * \param name optional name.
     * \return created SNLScalarTerm.
     */
    static SNLScalarTerm* create(SNLDesign* design, NLID::DesignObjectID id, Direction direction, const NLName& name=NLName());

    SNLDesign* getDesign() const override { return design_; }
    NLID getNLID() const override;
    NLID::DesignObjectID getID() const override { return id_; }
    NLID::Bit getBit() const override { return 0; }
    size_t getFlatID() const override { return flatID_; }
    NLName getName() const override { return name_; }
    bool isUnnamed() const override { return name_.empty(); }

    /// \brief Change the name of this SNLScalarTerm.
    void setName(const NLName& name) override;

    NajaCollection<SNLBitTerm*> getBits() const override;
    SNLTerm::Direction getDirection() const override { return direction_; }
    void setDirection(const SNLTerm::Direction& direction) { direction_ = direction; }

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
    bool deepCompare(const SNLNetComponent* other, std::string& reason) const override;
  private:
    SNLScalarTerm(SNLDesign* design, Direction direction, const NLName& name);
    SNLScalarTerm(SNLDesign* design, NLID::DesignObjectID, Direction direction, const NLName& name);
    static void preCreate(SNLDesign* design, const NLName& name);
    static void preCreate(SNLDesign* design, NLID::DesignObjectID id, const NLName& name);
    void postCreateAndSetID();
    void postCreate();
    void destroyFromDesign() override;
    SNLTerm* clone(SNLDesign* design) const override;
    void commonPreDestroy();
    void preDestroy() override;

    void setID(NLID::DesignObjectID id) override { id_ = id; }
    void setFlatID(size_t flatID) override {flatID_ = flatID; }

    SNLDesign*            design_;
    NLID::DesignObjectID  id_         {0};
    size_t                flatID_     {0};
    NLName                name_       {};
    SNLTerm::Direction    direction_;  
};

}} // namespace NL // namespace naja

#endif // __SNL_SCALAR_TERM_H_