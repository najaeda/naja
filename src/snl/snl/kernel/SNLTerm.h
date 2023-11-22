// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_TERM_H_
#define __SNL_TERM_H_

#include <boost/intrusive/set.hpp>

#include "SNLName.h"
#include "SNLNetComponent.h"

namespace naja { namespace SNL {

class SNLBitTerm;

class SNLTerm: public SNLNetComponent {
  public:
    friend class SNLDesign;
    using super = SNLNetComponent;
    
    SNLID::DesignObjectReference getReference() const;
    virtual SNLID::DesignObjectID getID() const = 0;
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
    ///\return term SNLName.
    virtual SNLName getName() const = 0;
    ///\return term size, 1 for SNLScalarTerm and SNLBusNetBit.
    virtual SNLID::Bit getSize() const = 0;

    virtual NajaCollection<SNLBitTerm*> getBits() const = 0;

  protected:
    SNLTerm() = default;

    static void preCreate();
    void postCreate();
    void preDestroy() override;

  private:
    //following used in BusTerm and ScalarTerm
    virtual void setID(SNLID::DesignObjectID id) = 0;
    virtual void setFlatID(size_t flatID) = 0;
    boost::intrusive::set_member_hook<> designTermsHook_  {};
    virtual void destroyFromDesign() = 0;
};

}} // namespace SNL // namespace naja

#endif // __SNL_TERM_H_
