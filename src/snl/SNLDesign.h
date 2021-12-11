#ifndef __SNL_DESIGN_H_
#define __SNL_DESIGN_H_

#include <map>

#include "SNLID.h"
#include "SNLTerm.h"
#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLParameter.h"
#include "SNLCollection.h"

namespace SNL {

class SNLLibrary;
class SNLScalarTerm;
class SNLBusTerm;
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

    SNLDB* getDB() const;
    SNLLibrary* getLibrary() const { return library_; }

    ///\return SNLTerm with SNLID::DesignObjectID id or nullptr if it does not exist
    SNLTerm* getTerm(SNLID::DesignObjectID id);
    ///\return SNLTerm with SNLName name or nullptr if it does not exist
    SNLTerm* getTerm(const SNLName& name);
    ///\return SNLScalarTerm with SNLName name or nullptr if it does not exist
    SNLScalarTerm* getScalarTerm(const SNLName& netName);
    ///\return SNLBusTerm with SNLName name or nullptr if it does not exist
    SNLBusTerm* getBusTerm(const SNLName& netName);
    SNLCollection<SNLTerm*> getTerms() const;

    ///\return SNLInstance with SNLID::DesignObjectID id or nullptr if it does not exist
    SNLInstance* getInstance(SNLID::DesignObjectID id);
    ///\return SNLInstance with SNLName name if it does not exist
    SNLInstance* getInstance(const SNLName& instanceName);
    SNLCollection<SNLInstance*> getInstances() const;

    ///\return SNLNet with SNLID::DesignObjectID id or nullptr if it does not exist
    SNLNet* getNet(SNLID::DesignObjectID id);
    ///\return SNLNet with SNLName name or nullptr if it does not exist
    SNLNet* getNet(const SNLName& netName);
    ///\return SNLScalarNet with SNLName name or nullptr if it does not exist
    SNLScalarNet* getScalarNet(const SNLName& netName);
    ///\return SNLBusNet with SNLName name or nullptr if it does not exist
    SNLBusNet* getBusNet(const SNLName& netName);
    SNLCollection<SNLNet*> getNets() const;

    ///\return SNLParameter with SNLName name or nullptr if it does not exist
    SNLParameter* getParameter(const SNLName& name);
    SNLCollection<SNLParameter*> getParameters() const;

    SNLID::DesignID getID() const { return id_; }
    SNLID getSNLID() const;

    SNLName getName() const { return name_; }
    bool isAnonymous() const { return name_.empty(); }
    
    Type getType() const { return type_; }
    bool isStandard() const { return type_ == Type::Standard; }
    bool isBlackBox() const { return type_ == Type::Blackbox; }
    bool isPrimitive() const { return type_ == Type::Primitive; }

    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    Card* getCard() const override;
  private:
    SNLDesign(SNLLibrary* library, const Type& type, const SNLName& name);
    static void preCreate(const SNLLibrary* library, const Type& type, const std::string& name);
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
