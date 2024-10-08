// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_SCALAR_TERM_H_
#define __SNL_SCALAR_TERM_H_

#include "SNLBitTerm.h"

namespace naja { namespace SNL {

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
    static SNLScalarTerm* create(SNLDesign* design, Direction direction, const SNLName& name=SNLName());

    /**
     * \brief Create a SNLScalarTerm with a given SNLID::DesignObjectID.
     * \param design owner SNLDesign.
     * \param id SNLID::DesignObjectID of the instance.
     * \param direction direction of the term.
     * \param name optional name.
     * \return created SNLScalarTerm.
     */
    static SNLScalarTerm* create(SNLDesign* design, SNLID::DesignObjectID id, Direction direction, const SNLName& name=SNLName());

    SNLDesign* getDesign() const override { return design_; }
    SNLID getSNLID() const override;
    SNLID::DesignObjectID getID() const override { return id_; }
    SNLID::Bit getBit() const override { return 0; }
    size_t getFlatID() const override { return flatID_; }
    SNLName getName() const override { return name_; }
    bool isAnonymous() const override { return name_.empty(); }

    /// \brief Change the name of this SNLScalarTerm.
    void setName(const SNLName& name) override;

    NajaCollection<SNLBitTerm*> getBits() const override;
    SNLTerm::Direction getDirection() const override { return direction_; }
    void setDirection(const SNLTerm::Direction& direction) { direction_ = direction; }

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
    bool deepCompare(const SNLTerm* other, std::string& reason) const override;
  private:
    SNLScalarTerm(SNLDesign* design, Direction direction, const SNLName& name);
    SNLScalarTerm(SNLDesign* design, SNLID::DesignObjectID, Direction direction, const SNLName& name);
    static void preCreate(SNLDesign* design, const SNLName& name);
    static void preCreate(SNLDesign* design, SNLID::DesignObjectID id, const SNLName& name);
    void postCreateAndSetID();
    void postCreate();
    void destroyFromDesign() override;
    SNLTerm* clone(SNLDesign* design) const override;
    void commonPreDestroy();
    void preDestroy() override;

    void setID(SNLID::DesignObjectID id) override { id_ = id; }
    void setFlatID(size_t flatID) override {flatID_ = flatID; }

    SNLDesign*            design_;
    SNLID::DesignObjectID id_         {};
    size_t                flatID_     {0};
    SNLName               name_       {};
    SNLTerm::Direction    direction_;  
};

}} // namespace SNL // namespace naja

#endif // __SNL_SCALAR_TERM_H_
