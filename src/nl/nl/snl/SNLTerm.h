// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_TERM_H_
#define __SNL_TERM_H_

#include <boost/intrusive/set.hpp>

#include "NLName.h"
#include "SNLNetComponent.h"

namespace naja { namespace NL {

class SNLBitTerm;

class SNLTerm: public SNLNetComponent {
  public:
    friend class SNLDesign;
    using super = SNLNetComponent;
    
    NLID::DesignObjectReference getReference() const;
    
    /// \return the unique NLID::DesignObjectID of this SNLTerm in the containing SNLDesign.
    virtual NLID::DesignObjectID getID() const = 0;

    /**
     * \brief Get the flat id of this SNLTerm in the containing SNLDesign flat SNLBitTerms.
     * 
     * With three Terms created in following order:
     *   - SNLScalarTerm term1
     *   - SNLBusTerm term2[1:0]
     *   - SNLScalarTerm term3
     * 
     * Positions will be the following:
     * |0      | 1        | 2        | 3     |
     * |-------|----------|----------|-------|
     * |term0  | term2    |          | term3 |
     * |       | term2[1] | term2[0] |       |  
     * \return this SNLTerm flat id in the containing SNLDesign flat SNLBitTerms.
     **/
    virtual size_t getFlatID() const = 0;
    /// \return term NLName.
    virtual NLName getName() const = 0;
    /// \return term width, 1 for SNLScalarTerm and SNLBusNetBit.
    virtual NLID::Bit getWidth() const = 0;

    /// \return the Collection of SNLBitTerm composing this SNLTerm. 
    virtual NajaCollection<SNLBitTerm*> getBits() const = 0;

    virtual bool deepCompare(const SNLTerm* other, std::string& reason) const = 0;

  protected:
    SNLTerm() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;

  private:
    //following used in BusTerm and ScalarTerm
    virtual void setID(NLID::DesignObjectID id) = 0;
    virtual void setFlatID(size_t flatID) = 0;
    boost::intrusive::set_member_hook<> designTermsHook_  {};
    virtual void destroyFromDesign() = 0;
    virtual SNLTerm* clone(SNLDesign* design) const = 0;
};

}} // namespace NL // namespace naja

#endif // __SNL_TERM_H_