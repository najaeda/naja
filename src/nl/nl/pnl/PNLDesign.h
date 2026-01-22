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
#include "PNLBox.h"

namespace naja::NL {

class NLDB;
class NLLibrary;
class PNLSite;

class PNLDesign final: public NLObject {
  public:
    enum Symmetry {
      NONE = 0,
      X,
      Y,
      X_Y,
      R90
    };
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

     class ClassType {
      public:
        enum ClassTypeEnum {
          NONE, 
          CORE, CORE_FEEDTHRU, CORE_TIEHIGH, CORE_TIELOW, CORE_SPACER, CORE_ANTENNACELL, CORE_WELLTAP,
          PAD, PAD_INPUT, PAD_OUTPUT, PAD_INOUT, PAD_POWER, PAD_SPACER, PAD_AREAIO, 
          BLOCK, BLACKBOX, SOFT_MACRO, 
          ENDCAP_PRE, ENDCAP_POST, ENDCAP_TOPLEFT, ENDCAP_TOPRIGHT, ENDCAP_BOTTOMLEFT, ENDCAP_BOTTOMRIGHT, 
          COVER, COVER_BUMP, RING
        };
        ClassType(const ClassTypeEnum& typeEnum);
        ClassType(const ClassType&) = default;
        ClassType& operator=(const ClassType&) = default;
        operator const ClassTypeEnum&() const {return typeEnum_;}
        std::string getString() const;
        private:
          ClassTypeEnum typeEnum_;
    };
    
    /**
     * \brief Create a PNLDesign.
     * \param library owning NLLibrary.
     * \param name NLName of the PNLDesign. If empty, the PNLDesign is anonymous.
     * \return the created PNLDesign.
     */
    static PNLDesign* create(NLLibrary* library, const NLName& name=NLName());

    static PNLDesign* create(NLLibrary* library, const Type& type, const NLName& name=NLName());
    /**
     * \brief Create a PNLDesign with a specific NLID::DesignID and a specific Type.
     * \param library owning NLLibrary.
     * \param id the NLID::DesignID of the PNLDesign to create.
     * \param type the Type of the PNLDesign to create.
     * \param name NLName of the PNLDesign. If empty, the PNLDesign is anonymous.
     * \return the created PNLDesign.
     */
    static PNLDesign* create(NLLibrary* library, NLID::DesignID id, Type type, const NLName& name=NLName());

    NLID::DesignID getID() const { return id_; }
    NLID getNLID() const;

    ///\return owning NLDB
    NLDB* getDB() const;
    /// \return owning NLLibrary.
    NLLibrary* getLibrary() const { return library_; }

    /// \return NLName of this PNLDesign. 
    NLName getName() const { return name_; }
    void setName(const NLName& name);
    /// \return true if this PNLDesign is unnamed.
    bool isUnnamed() const { return name_.empty(); }

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

    PNLNet* addNet(const NLName& name);
    void addNet(PNLNet* net);
    void addNetAndSetID(PNLNet* net);
    void removeNet(PNLNet* net);

    PNLScalarTerm* getScalarTerm(NLID::DesignObjectID id) const;
    PNLScalarTerm* getScalarTerm(const NLName& termName) const;

    void rename(PNLTerm* term, const NLName& previousName);
    void rename(PNLNet* net, const NLName& previousName);
    void rename(PNLInstance* instance, const NLName& previousName);

    PNLTerm* addTerm(const NLName& name);
    void addTerm(PNLTerm* term);
    void addTermAndSetID(PNLTerm* term);
    void removeTerm(PNLTerm* term);

    NajaCollection<PNLInstance*> getSlaveInstances() const;

    PNLBitTerm* getBitTerm(NLID::DesignObjectID id) const;
    PNLBitTerm* getBitTerm(const NLName& termName) const;
    NajaCollection<PNLBitTerm*> getBitTerms() const;
    PNLBitTerm* getBitTerm(NLID::DesignObjectID id, NLID::Bit bit) const;
    
    NajaCollection<PNLBitNet*> getBitNets() const;
    NajaCollection<PNLScalarNet*> getScalarNets() const;

    PNLScalarNet* getScalarNet(NLID::DesignObjectID id) const;
    PNLScalarNet* getScalarNet(const NLName& netName) const;
    NajaCollection<PNLScalarTerm*> getScalarTerms() const;

    bool isLeaf() const { return isBlackBox() or isPrimitive(); }

    ///\warning setType cannot be called to set a design as a primitive.
    void setType(Type type);

    void setClassType(ClassType type) { classType_ = type; }
    ClassType getClassType() const { return classType_; }

    NajaCollection<PNLInstance*> getInstances() const;
    NajaCollection<PNLInstance*> getPrimitiveInstances() const;
    NajaCollection<PNLInstance*> getNonPrimitiveInstances() const;

    void setAbutmentBox(const PNLBox& box) { abutmentBox_ = box; }
    const PNLBox& getAbutmentBox() const { return abutmentBox_; }
    //const PNLBox& getBoundingBox() const { return boundingBox_; }

    void setTerminalNetlist(bool terminalNetlist) { terminalNetlist_ = terminalNetlist; }
    bool isTerminalNetlist() const { return terminalNetlist_; }
    void setSite(PNLSite* site) { site_ = site; }
    PNLSite* getSite() const { return site_; }

    void setSymmetry(Symmetry symmetry) { Symmetry_ = symmetry; }
    Symmetry getSymmetry() const { return Symmetry_; }

  private:
    PNLDesign(NLLibrary* library, const Type& type = Type::Standard, const NLName& name = NLName());
    PNLDesign(NLLibrary* library, NLID::DesignID id, Type type, const NLName& name);
    static void preCreate(const NLLibrary* library, Type type, const NLName& name);
    static void preCreate(const NLLibrary* library, NLID::DesignID id, Type type, const NLName& name);
    void postCreateAndSetID();
    void commonPreDestroy();
    void destroyFromLibrary();
    void preDestroy() override;

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
    ClassType                           classType_          { ClassType::NONE };
    PNLDesignTerms                      terms_              {};
    PNLDesignObjectNameIDMap            termNameIDMap_      {};
    PNLDesignNets                       nets_               {};
    PNLDesignObjectNameIDMap            netNameIDMap_       {};
    PNLBox                              abutmentBox_;
    PNLBox                              boundingBox_;
    bool                                terminalNetlist_ = false;
    PNLSite*                            site_ = nullptr;
    Symmetry                            Symmetry_ = NONE;
};

}  // namespace naja::NL