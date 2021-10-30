#ifndef __SNL_DESIGN_H_
#define __SNL_DESIGN_H_

#include <map>

#include "SNLID.h"
#include "SNLTerm.h"
#include "SNLInstance.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLCollection.h"

namespace SNL {

class SNLLibrary;
class SNLScalarTerm;
class SNLBusTerm;

class SNLDesign final: public SNLObject {
  public:
    friend class SNLLibrary;
    friend class SNLScalarTerm;
    friend class SNLBusTerm;
    friend class SNLInstance;
    friend class SNLScalarNet;
    friend class SNLBusNet;

    using super = SNLObject;

    SNLDesign() = delete;
    SNLDesign(const SNLDesign& design) = delete;

    static SNLDesign* create(SNLLibrary* library);
    static SNLDesign* create(SNLLibrary* library, const std::string& name);

    SNLLibrary* getLibrary() const { return library_; }
    SNLTerm* getTerm(const SNLName& netName);
    SNLScalarTerm* getScalarTerm(const SNLName& netName);
    SNLBusTerm* getBusTerm(const SNLName& netName);
    SNLInstance* getInstance(const SNLName& instanceName);
    SNLScalarNet* getScalarNet(const SNLName& netName);
    SNLBusNet* getBusNet(const SNLName& netName);

    SNLCollection<SNLInstance> getInstances() const;

    SNLID::DesignID getID() const { return id_; }
    SNLID getSNLID() const;
    bool isAnonymous() const { return name_.empty(); }
    SNLName getName() const { return name_; }
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    Card* getCard() const override;
  private:
    SNLDesign(SNLLibrary* library);
    SNLDesign(SNLLibrary* library, const SNLName& name);
    static void preCreate(const SNLLibrary* library);
    static void preCreate(const SNLLibrary* library, const std::string& name);
    void destroyFromLibrary();
    void postCreate();
    void commonPreDestroy();
    void preDestroy() override;
    void addTerm(SNLTerm* term);
    void removeTerm(SNLTerm* term);
    void addInstance(SNLInstance* instance);
    void removeInstance(SNLInstance* instance);
    void addScalarNet(SNLScalarNet* scalarNet);
    void removeScalarNet(SNLScalarNet* scalarNet);
    void addBusNet(SNLBusNet* busNet);
    void removeBusNet(SNLBusNet* busNet);

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
    using SNLInstanceNameIDMap = std::map<SNLName, SNLID::InstanceID>;
    using SNLDesignScalarNetsHook =
      boost::intrusive::member_hook<SNLScalarNet, boost::intrusive::set_member_hook<>, &SNLScalarNet::designScalarNetsHook_>;
    using SNLDesignScalarNets = boost::intrusive::set<SNLScalarNet, SNLDesignScalarNetsHook>;
    using SNLDesignBusNetsHook =
      boost::intrusive::member_hook<SNLBusNet, boost::intrusive::set_member_hook<>, &SNLBusNet::designBusNetsHook_>;
    using SNLDesignBusNets = boost::intrusive::set<SNLBusNet, SNLDesignBusNetsHook>;

    SNLID::DesignID                     id_;
    SNLName                             name_               {};
    SNLLibrary*                         library_;
    boost::intrusive::set_member_hook<> libraryDesignsHook_ {};
    SNLDesignTerms                      terms_              {};
    SNLDesignObjectNameIDMap            termNameIDMap_      {};
    SNLDesignInstances                  instances_          {};
    SNLInstanceNameIDMap                instanceNameIDMap_  {};
    SNLDesignScalarNets                 scalarNets_         {};
    SNLDesignBusNets                    busNets_            {};
};

}

#endif /* __SNL_DESIGN_H_ */ 
