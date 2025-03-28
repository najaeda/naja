// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLDesign.h"
#include "PNLInstance.h"
#include "PNLTerm.h"
#include "PNLNet.h"
#include "PNLScalarTerm.h"
#include "PNLScalarNet.h"

namespace naja { namespace NL {

class NLDB;
class NLLibrary;

class PNLDesign final: public NLObject {
  public:
    friend class NLLibrary;
    using super = NLObject;
    using PNLDesignInstancesHook =
      boost::intrusive::member_hook<PNLInstance, boost::intrusive::set_member_hook<>, &PNLInstance::designInstancesHook_>;
    using PNLDesignInstances = boost::intrusive::set<PNLInstance, PNLDesignInstancesHook, boost::intrusive::compare<NLDesign::CompareByID<PNLInstance>>>;
    using PNLDesignObjectNameIDMap = std::map<NLName, NLID::DesignObjectID>;
    using PNLDesignSlaveInstancesHook =
      boost::intrusive::member_hook<PNLInstance, boost::intrusive::set_member_hook<>, &PNLInstance::designSlaveInstancesHook_>;
    using PNLDesignSlaveInstances = boost::intrusive::set<PNLInstance, PNLDesignSlaveInstancesHook>;
    using PNLDesignTermsHook =
      boost::intrusive::member_hook<PNLTerm, boost::intrusive::set_member_hook<>, &PNLTerm::designTermsHook_>;
    using PNLDesignTerms = boost::intrusive::set<PNLTerm, PNLDesignTermsHook, boost::intrusive::compare<NLDesign::CompareByID<PNLTerm>>>;
    using PNLDesignNetsHook =
      boost::intrusive::member_hook<PNLNet, boost::intrusive::set_member_hook<>, &PNLNet::designNetsHook_>;
    using PNLDesignNets = boost::intrusive::set<PNLNet, PNLDesignNetsHook, boost::intrusive::compare<NLDesign::CompareByID<PNLNet>>>;

    class Type {
      public:
        enum TypeEnum {
          Standard, Blackbox, Primitive
        };
        Type(const TypeEnum& typeEnum);
        Type(const Type&) = default;
        Type& operator=(const Type&) = default;
        operator const TypeEnum&() const {return typeEnum_;}
        std::string getString() const;
        private:
          TypeEnum typeEnum_;
    };

    static PNLDesign* create(NLLibrary* library, const NLName& name=NLName(), const Type::TypeEnum& type=Type::Standard);

    NLID::DesignID getID() const { return id_; }
    NLID getNLID() const;

    ///\return owning NLDB
    NLDB* getDB() const;
    /// \return owning NLLibrary.
    NLLibrary* getLibrary() const { return library_; }

    /// \return NLName of this PNLDesign. 
    NLName getName() const { return name_; }
    /// \return true if this PNLDesign is anonymous.
    bool isAnonymous() const { return name_.empty(); }

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    bool deepCompare(
      const PNLDesign* other,
      std::string& reason,
      NLDesign::CompareType type=NLDesign::CompareType::Complete) const;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
    void addInstance(PNLInstance* instance);
    void removeInstance(PNLInstance* instance);
    void addInstanceAndSetID(PNLInstance* instance);

    PNLInstance* getInstance(const NLName& name) const;

    PNLInstance* getInstance(NLID::DesignObjectID id) const;

    void addSlaveInstance(PNLInstance* instance);
    void removeSlaveInstance(PNLInstance* instance);

    bool isStandard() const { return type_ == Type::Standard; }
    ///\return true if this PNLDesign is a blackbox.
    bool isBlackBox() const { return type_ == Type::Blackbox; }
    ///\return true if this PNLDesign is a primitive.
    bool isPrimitive() const { return type_ == Type::Primitive; }

    NLID::DesignReference getReference() const;

    NajaCollection<PNLTerm*> getTerms() const;
    PNLTerm* getTerm(const NLName& name) const;
    PNLTerm* getTerm(NLID::DesignObjectID id) const;

    //const PNLDesignNets& getNets() const { return nets_; }
    NajaCollection<PNLNet*> getNets() const;
    PNLNet* getNet(const NLName& name) const;
    PNLNet* getNet(NLID::DesignObjectID id) const;

    void addNet(PNLNet* net);
    void addNetAndSetID(PNLNet* net);
    void removeNet(PNLNet* net);

    PNLScalarTerm* getScalarTerm(NLID::DesignObjectID id) const;
    PNLScalarTerm* getScalarTerm(const NLName& termName) const;

    void rename(PNLTerm* term, const NLName& previousName);
    void rename(PNLNet* net, const NLName& previousName);
    void rename(PNLInstance* instance, const NLName& previousName);

    void addTerm(PNLTerm* term);
    void addTermAndSetID(PNLTerm* term);
    void removeTerm(PNLTerm* term);

    NajaCollection<PNLInstance*> getSlaveInstances() const;

    PNLBitTerm* getBitTerm(NLID::DesignObjectID id) const;
    PNLBitTerm* getBitTerm(const NLName& termName) const;
    
    NajaCollection<PNLBitNet*> getBitNets() const;
    NajaCollection<PNLScalarNet*> getScalarNets() const;

    PNLScalarNet* getScalarNet(NLID::DesignObjectID id) const;
    PNLScalarNet* getScalarNet(const NLName& netName) const;

    bool isLeaf() const { return isBlackBox() or isPrimitive(); }

  private:
    PNLDesign(NLLibrary* library, const NLName& name, const Type::TypeEnum& type = Type::Standard);
    static void preCreate(const NLLibrary* library, const NLName& name);
    void postCreateAndSetID();
    void commonPreDestroy();
    void destroyFromLibrary();

    friend bool operator< (const PNLDesign& ld, const PNLDesign& rd) {
      return ld.getNLID() < rd.getNLID();
    }

    NLID::DesignID                      id_;
    NLName                              name_               {};
    NLLibrary*                          library_            {};
    boost::intrusive::set_member_hook<> libraryDesignsHook_ {};
    PNLDesignInstances                  instances_          {};
    PNLDesignObjectNameIDMap            instanceNameIDMap_  {};
    PNLDesignSlaveInstances             slaveInstances_     {};
    Type                                type_               { Type::Standard };
    PNLDesignTerms                      terms_              {};
    PNLDesignObjectNameIDMap            termNameIDMap_      {};
    PNLDesignNets                       nets_               {};
    PNLDesignObjectNameIDMap            netNameIDMap_       {};
};

}} // namespace NL // namespace naja