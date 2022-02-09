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

#ifndef __SNL_INSTANCE_H_
#define __SNL_INSTANCE_H_

#include <vector>
#include <map>
#include <boost/intrusive/set.hpp>

#include "SNLDesignObject.h"
#include "SNLID.h"
#include "SNLName.h"
#include "SNLCollection.h"

namespace naja { namespace SNL {

class SNLTerm;
class SNLBitTerm;
class SNLInstTerm;
class SNLSharedPath;

class SNLInstance final: public SNLDesignObject {
  public:
    friend class SNLDesign;
    friend class SNLSharedPath;
    friend class SNLPath;
    using super = SNLDesignObject;
    using SNLInstanceInstTerms = std::vector<SNLInstTerm*>; 
    using SNLSharedPaths = std::map<const SNLSharedPath*, SNLSharedPath*>;

    /**
     * @brief SNLInstance creator 
     * 
     * @param design owner SNLDesign
     * @param model instanciated SNLDesign
     * @param name optional name
     * @return SNLInstance* 
     */
    static SNLInstance* create(SNLDesign* design, SNLDesign* model, const SNLName& name=SNLName());

    SNLDesign* getDesign() const override { return design_; }
    SNLDesign* getModel() const { return model_; }

    SNLID::DesignObjectID getID() const { return id_; }
    SNLID getSNLID() const override;
    SNLName getName() const { return name_; }
    bool isAnonymous() const override { return name_.empty(); }
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    Card* getCard() const override;

    SNLInstTerm* getInstTerm(const SNLBitTerm* term);
    SNLCollection<SNLInstTerm*> getInstTerms() const;
    SNLCollection<SNLInstTerm*> getInstScalarTerms() const;
    SNLCollection<SNLInstTerm*> getInstBusTermBits() const;
  private:
    static void preCreate(SNLDesign* design, const SNLName& name);
    void postCreate();
    void commonPreDestroy();
    void destroyFromDesign();
    void destroyFromModel();
    void preDestroy() override;
    void createInstTerm(SNLBitTerm* term);
    void removeInstTerm(SNLBitTerm* term);

    SNLInstance(SNLDesign* design, SNLDesign* model, const SNLName& name);

    SNLSharedPath* getSharedPath(const SNLSharedPath* tailSharedPath) const;
    void addSharedPath(const SNLSharedPath* tailSharedPath);

    SNLDesign*                          design_                   {nullptr};
    SNLDesign*                          model_                    {nullptr};
    SNLID::InstanceID                   id_;
    SNLName                             name_                     {};
    SNLInstanceInstTerms                instTerms_                {};
    boost::intrusive::set_member_hook<> designInstancesHook_      {};
    boost::intrusive::set_member_hook<> designSlaveInstancesHook_ {};
    SNLSharedPaths                      sharedPaths_              {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_INSTANCE_H_ 