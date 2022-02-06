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

#ifndef __SNL_DESIGN_H_
#define __SNL_DESIGN_H_

#include <map>

#include "SNLCollection.h"
#include "SNLBusTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLParameter.h"

namespace SNL {

class SNLLibrary;
class SNLScalarNet;
class SNLBusNet;

class SNLDesign final: public SNLObject {
  public:
    friend class SNLLibrary;
    friend class SNLScalarTerm;
    friend class SNLBusTerm;
    friend class SNLInstance;
    friend class SNLScalarNet;
    friend class SNLBusNet;
    friend class SNLParameter;
    using super = SNLObject;

    class Type {
      public:
        enum TypeEnum {
          Standard, Blackbox, Primitive
        };
        Type(const TypeEnum& typeEnum);
        Type(const Type& type) = default;
        operator const TypeEnum&() const {return typeEnum_;}
        std::string getString() const;
        private:
          TypeEnum typeEnum_;
    };

    SNLDesign() = delete;
    SNLDesign(const SNLDesign& design) = delete;

    static SNLDesign* create(SNLLibrary* library, const SNLName& name=SNLName());
    static SNLDesign* create(SNLLibrary* library, const Type& type, const SNLName& name=SNLName());

    ///\return owning SNLDB
    SNLDB* getDB() const;
    ///\return owning SNLLibrary
    SNLLibrary* getLibrary() const { return library_; }

    ///\return SNLTerm with SNLID::DesignObjectID id or nullptr if it does not exist
    SNLTerm* getTerm(SNLID::DesignObjectID id) const;
    ///\return SNLTerm with SNLName name or nullptr if it does not exist
    SNLTerm* getTerm(const SNLName& name) const;
    ///\return SNLScalarTerm with SNLName name or nullptr if it does not exist
    SNLScalarTerm* getScalarTerm(const SNLName& netName) const;
    ///\return SNLBusTerm with SNLName name or nullptr if it does not exist
    SNLBusTerm* getBusTerm(const SNLName& netName) const;
    ///\return the collection of SNLTerm of this SNLDesign
    SNLCollection<SNLTerm*> getTerms() const;
    ///\return the collection of SNLBusTerm of this SNLDesign (SNLBusTerm subset of getTerms())
    ///\see getTerms()
    ///\see getScalarTerms()
    SNLCollection<SNLBusTerm*> getBusTerms() const;
    ///\return the collection of SNLScalarTerm of this SNLDesign (SNLScalarTerm subset of getTerms())
    ///\see getTerms()
    ///\see getBusTerms()
    SNLCollection<SNLScalarTerm*> getScalarTerms() const;
    SNLCollection<SNLBitTerm*> getBitTerms() const;

    ///\return SNLInstance with SNLID::DesignObjectID id or nullptr if it does not exist
    SNLInstance* getInstance(SNLID::DesignObjectID id) const;
    ///\return SNLInstance with SNLName name if it does not exist
    SNLInstance* getInstance(const SNLName& instanceName) const;
    ///\return the collection of SNLInstance instantiated IN this SNLDesign (instance/master relationship) 
    SNLCollection<SNLInstance*> getInstances() const;
    ///\return the collection of SNLInstance instantiated BY this SNLDesign (instance/model relationship)
    ///\remark SNLInstance/SNLDesign model relationship is not constructed for Primitives.
    ///\sa isPrimitive
    SNLCollection<SNLInstance*> getSlaveInstances() const;

    ///\return SNLNet with SNLID::DesignObjectID id or nullptr if it does not exist
    SNLNet* getNet(SNLID::DesignObjectID id) const;
    ///\return SNLNet with SNLName name or nullptr if it does not exist
    SNLNet* getNet(const SNLName& netName) const;
    ///\return SNLScalarNet with SNLName name or nullptr if it does not exist
    SNLScalarNet* getScalarNet(const SNLName& netName) const;
    ///\return SNLBusNet with SNLName name or nullptr if it does not exist
    SNLBusNet* getBusNet(const SNLName& netName) const;
    SNLCollection<SNLNet*> getNets() const;

    ///\return SNLParameter with SNLName name or nullptr if it does not exist
    SNLParameter* getParameter(const SNLName& name) const;
    SNLCollection<SNLParameter*> getParameters() const;

    SNLID::DesignID getID() const { return id_; }
    SNLID getSNLID() const;

    SNLName getName() const { return name_; }
    bool isAnonymous() const { return name_.empty(); }
    
    Type getType() const { return type_; }
    bool isStandard() const { return type_ == Type::Standard; }
    bool isBlackBox() const { return type_ == Type::Blackbox; }
    ///\return true if this SNLDesign is a primitive.
    bool isPrimitive() const { return type_ == Type::Primitive; }

    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    Card* getCard() const override;
  private:
    SNLDesign(SNLLibrary* library, const Type& type, const SNLName& name);
    static void preCreate(const SNLLibrary* library, const Type& type, const SNLName& name);
    void destroyFromLibrary();
    void postCreate();
    void commonPreDestroy();
    void preDestroy() override;
    void addTerm(SNLTerm* term);
    void removeTerm(SNLTerm* term);
    void addInstance(SNLInstance* instance);
    void removeInstance(SNLInstance* instance);
    void addSlaveInstance(SNLInstance* instance);
    void removeSlaveInstance(SNLInstance* instance);
    void addNet(SNLNet* net);
    void removeNet(SNLNet* net);
    void addParameter(SNLParameter* parameter);
    void removeParameter(SNLParameter* parameter);

    friend bool operator< (const SNLDesign &ld, const SNLDesign &rd) {
      return ld.getSNLID() < rd.getSNLID();
    }

    using SNLDesignTermsHook =
      boost::intrusive::member_hook<SNLTerm, boost::intrusive::set_member_hook<>, &SNLTerm::designTermsHook_>;
    using SNLDesignTerms = boost::intrusive::set<SNLTerm, SNLDesignTermsHook>;
    using SNLDesignObjectNameIDMap = std::map<SNLName, SNLID::DesignObjectID>;
    using SNLDesignInstancesHook =
      boost::intrusive::member_hook<SNLInstance, boost::intrusive::set_member_hook<>, &SNLInstance::designInstancesHook_>;
    using SNLDesignInstances = boost::intrusive::set<SNLInstance, SNLDesignInstancesHook>;
    using SNLDesignSlaveInstancesHook =
      boost::intrusive::member_hook<SNLInstance, boost::intrusive::set_member_hook<>, &SNLInstance::designSlaveInstancesHook_>;
    using SNLDesignSlaveInstances = boost::intrusive::set<SNLInstance, SNLDesignSlaveInstancesHook>;
    using SNLInstanceNameIDMap = std::map<SNLName, SNLID::InstanceID>;
    using SNLDesignNetsHook =
      boost::intrusive::member_hook<SNLNet, boost::intrusive::set_member_hook<>, &SNLNet::designNetsHook_>;
    using SNLDesignNets = boost::intrusive::set<SNLNet, SNLDesignNetsHook>;
    using SNLDesignParametersHook =
      boost::intrusive::member_hook<SNLParameter, boost::intrusive::set_member_hook<>, &SNLParameter::designParametersHook_>;
    using SNLDesignParameters = boost::intrusive::set<SNLParameter, SNLDesignParametersHook>;

    SNLID::DesignID                     id_;
    SNLName                             name_               {};
    Type                                type_               { Type::Standard };
    SNLLibrary*                         library_;
    boost::intrusive::set_member_hook<> libraryDesignsHook_ {};
    SNLDesignTerms                      terms_              {};
    SNLDesignObjectNameIDMap            termNameIDMap_      {};
    SNLDesignInstances                  instances_          {};
    SNLInstanceNameIDMap                instanceNameIDMap_  {};
    SNLDesignSlaveInstances             slaveInstances_     {};
    SNLDesignNets                       nets_               {};
    SNLDesignObjectNameIDMap            netNameIDMap_       {};
    SNLDesignParameters                 parameters_         {};
};

}

#endif /* __SNL_DESIGN_H_ */ 
