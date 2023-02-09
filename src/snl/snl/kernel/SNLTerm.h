/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SNL_TERM_H_
#define __SNL_TERM_H_

#include <boost/intrusive/set.hpp>

#include "SNLName.h"
#include "SNLNetComponent.h"

namespace naja { namespace SNL {

class SNLTerm: public SNLNetComponent {
  public:
    friend class SNLDesign;
    using super = SNLNetComponent;
    
    SNLID::DesignObjectReference getReference() const;
    virtual SNLID::DesignObjectID getID() const = 0;
    /**
     * \return this SNLTerm flat id in the containing SNLDesign flat SNLBitTerms.
     * \n
     * For instance, with three Terms created in following order:
     *   - SNLScalarTerm term1
     *   - SNLBusTerm term2[1:0]
     *   - SNLScalarTerm term3
     * 
     * Positions will be the following:
     * |0      | 1        | 2        | 3     |
     * |-------|----------|----------|-------|
     * |term0  | term2    |          | term3 |
     * |       | term2[1] | term2[0] |       |  
     **/
    virtual size_t getFlatID() const = 0;
    ///\return term SNLName.
    virtual SNLName getName() const = 0;
    ///\return term size, 1 for SNLScalarTerm and SNLBusNetBit.
    virtual size_t getSize() const = 0;

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
