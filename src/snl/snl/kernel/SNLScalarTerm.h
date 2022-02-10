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

#ifndef __SNL_SCALAR_TERM_H_
#define __SNL_SCALAR_TERM_H_

#include "SNLBitTerm.h"
#include "SNLName.h"

namespace naja { namespace SNL {

class SNLScalarTerm final: public SNLBitTerm {
  public:
    friend class SNLDesign;
    using super = SNLBitTerm;

    static SNLScalarTerm* create(SNLDesign* design, const Direction& direction, const SNLName& name=SNLName());

    SNLDesign* getDesign() const override { return design_; }

    SNLID getSNLID() const override;
    SNLID::DesignObjectID getID() const override { return id_; }
    SNLID::Bit getBit() const override { return 0; }
    size_t getPosition() const override { return position_; } 
    SNLName getName() const override { return name_; }
    bool isAnonymous() const override { return name_.empty(); }
    SNLTerm::Direction getDirection() const override { return direction_; }
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
  private:
    SNLScalarTerm(SNLDesign* design, const Direction& direction, const SNLName& name);
    static void preCreate(SNLDesign* design, const SNLName& name);
    void postCreate();
    void destroyFromDesign() override;
    void commonPreDestroy();
    void preDestroy() override;

    void setID(SNLID::DesignObjectID id) override { id_ = id; }
    void setPosition(size_t position) override {position_ = position; }

    SNLDesign*            design_;
    SNLID::DesignObjectID id_         {};
    size_t                position_   {0};
    SNLName               name_       {};
    SNLTerm::Direction    direction_;  
};

}} // namespace SNL // namespace naja

#endif // __SNL_SCALAR_TERM_H_