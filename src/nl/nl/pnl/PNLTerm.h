// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_TERM_H_
#define __PNL_TERM_H_

#include <boost/intrusive/set.hpp>

#include "NLName.h"
#include "PNLNetComponent.h"

namespace naja { namespace NL {

class PNLBitTerm;

class PNLTerm: public PNLNetComponent {
  public:
    friend class PNLDesign;
    using super = PNLNetComponent;
    
    NLID::DesignObjectReference getReference() const;
    
    /// \return the unique NLID::DesignObjectID of this PNLTerm in the containing PNLDesign.
    virtual NLID::DesignObjectID getID() const = 0;

    /**
     * \brief Get the flat id of this PNLTerm in the containing PNLDesign flat PNLBitTerms.
     * 
     * With three Terms created in following order:
     *   - PNLScalarTerm term1
     *   - PNLBusTerm term2[1:0]
     *   - PNLScalarTerm term3
     * 
     * Positions will be the following:
     * |0      | 1        | 2        | 3     |
     * |-------|----------|----------|-------|
     * |term0  | term2    |          | term3 |
     * |       | term2[1] | term2[0] |       |  
     * \return this PNLTerm flat id in the containing PNLDesign flat PNLBitTerms.
     **/
    virtual size_t getFlatID() const = 0;
    /// \return term NLName.
    virtual NLName getName() const = 0;
    /// \return term width, 1 for PNLScalarTerm and PNLBusNetBit.
    virtual NLID::Bit getWidth() const = 0;

    /// \return the Collection of PNLBitTerm composing this PNLTerm. 
    virtual NajaCollection<PNLBitTerm*> getBits() const = 0;

    //virtual bool deepCompare(const PNLTerm* other, std::string& reason) const = 0;

  protected:
    PNLTerm() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;

  private:
    //following used in BusTerm and ScalarTerm
    virtual void setID(NLID::DesignObjectID id) = 0;
    virtual void setFlatID(size_t flatID) = 0;
    boost::intrusive::set_member_hook<> designTermsHook_  {};
    virtual void destroyFromDesign() = 0;
    //virtual PNLTerm* clone(PNLDesign* design) const = 0;
};

}} // namespace NL // namespace naja

#endif // __PNL_TERM_H_