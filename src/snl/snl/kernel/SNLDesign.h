// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DESIGN_H_
#define __SNL_DESIGN_H_

#include <map>

#include "NajaCollection.h"
#include "SNLTerm.h"
#include "SNLNet.h"
#include "SNLInstance.h"
#include "SNLParameter.h"

namespace naja { namespace SNL {

class SNLLibrary;
class SNLBitNet;
class SNLScalarNet;
class SNLBusNet;
class SNLBusNetBit;
class SNLBitTerm;
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

    /**
     * \brief Create a SNLDesign.
     * \param library owning SNLLibrary.
     * \param name SNLName of the SNLDesign. If empty, the SNLDesign is anonymous.
     * \return the created SNLDesign.
     */
    static SNLDesign* create(SNLLibrary* library, const SNLName& name=SNLName());

    /**
     * \brief Create a SNLDesign with a specific Type.
     * \param library owning SNLLibrary.
     * \param type the Type of the SNLDesign to create.
     * \param name SNLName of the SNLDesign. If empty, the SNLDesign is anonymous.
     * \return the created SNLDesign.
     */
    static SNLDesign* create(SNLLibrary* library, Type type, const SNLName& name=SNLName());
    
    /**
     * \brief Create a SNLDesign with a specific SNLID::DesignID and a specific Type.
     * \param library owning SNLLibrary.
     * \param id the SNLID::DesignID of the SNLDesign to create.
     * \param type the Type of the SNLDesign to create.
     * \param name SNLName of the SNLDesign. If empty, the SNLDesign is anonymous.
     * \return the created SNLDesign.
     */
    static SNLDesign* create(SNLLibrary* library, SNLID::DesignID id, Type type, const SNLName& name=SNLName());

    /// \return owning SNLDB.
    SNLDB* getDB() const;
    /// \return owning SNLLibrary.
    SNLLibrary* getLibrary() const { return library_; }

    /// \return SNLTerm with SNLID::DesignObjectID id or nullptr if it does not exist.
    SNLTerm* getTerm(SNLID::DesignObjectID id) const;
    /// \return SNLTerm with SNLName name or nullptr if it does not exist
    SNLTerm* getTerm(const SNLName& name) const;
    /// \return SNLScalarTerm with SNLID::DesignObjectID id or nullptr if it does not exist
    SNLScalarTerm* getScalarTerm(SNLID::DesignObjectID id) const;
    /// \return SNLScalarTerm with SNLName termName or nullptr if it does not exist
    SNLScalarTerm* getScalarTerm(const SNLName& termName) const;
    /// \return SNLBusTerm with SNLID::DesignObjectID id or nullptr if it does not exist
    SNLBusTerm* getBusTerm(SNLID::DesignObjectID id) const;
    /// \return SNLBusTerm with SNLName termName or nullptr if it does not exist
    SNLBusTerm* getBusTerm(const SNLName& termName) const;
    /// \return the collection of SNLTerm of this SNLDesign
    NajaCollection<SNLTerm*> getTerms() const;

    /**
     * \return the collection of SNLBusTerm of this SNLDesign (SNLBusTerm subset of getTerms())
     * \sa getTerms()
     * \sa getScalarTerms()
     */
    NajaCollection<SNLBusTerm*> getBusTerms() const;

    /**
     * \return the collection of SNLScalarTerm of this SNLDesign (SNLScalarTerm subset of getTerms()).
     * \sa getTerms()
     * \sa getBusTerms()
     */
    NajaCollection<SNLScalarTerm*> getScalarTerms() const;

    /// \return the collection of SNLBitTerm of this SNLDesign (SNLScalarTerm and flattened SNLBusTerm to SNLBusTermBit).
    NajaCollection<SNLBitTerm*> getBitTerms() const;
    /// \return SNLInstance with SNLID::DesignObjectID id or nullptr if it does not exist.
    SNLInstance* getInstance(SNLID::DesignObjectID id) const;
    /// \return SNLInstance with SNLName name if it does not exist.
    SNLInstance* getInstance(const SNLName& instanceName) const;
    /// \return the collection of SNLInstance instantiated IN this SNLDesign (instance/parent relationship).
    NajaCollection<SNLInstance*> getInstances() const;

    /**
     * \return the collection of SNLInstance instantiated BY this SNLDesign (instance/model relationship).
     * \remark SNLInstance/SNLDesign model relationship is not constructed for Primitives.
     * \sa isPrimitive
     */
    NajaCollection<SNLInstance*> getSlaveInstances() const;

    /**
     * \return the collection of SNLInstance instantiated IN this SNLDesign (instance/parent relationship)
     * and instanciating a Primitive model. 
     * \sa isPrimitive getInstances getNonPrimitiveInstances
     */
    NajaCollection<SNLInstance*> getPrimitiveInstances() const;

    /**
     * \return the collection of SNLInstance instantiated IN this SNLDesign (instance/parent relationship)
     * and instanciating a non Primitive model.
     * \sa isPrimitive getInstances getPrimitiveInstances
     */
    NajaCollection<SNLInstance*> getNonPrimitiveInstances() const;

    /// \return SNLNet with SNLID::DesignObjectID id or nullptr if it does not exist.
    SNLNet* getNet(SNLID::DesignObjectID id) const;
    /// \return SNLNet with SNLName name or nullptr if it does not exist.
    SNLNet* getNet(const SNLName& netName) const;
    /// \return SNLScalarNet with SNLID::DesignObjectID id or nullptr if it does not exist.
    SNLScalarNet* getScalarNet(SNLID::DesignObjectID id) const;
    /// \return SNLBusNetBit with SNLID::DesignObjectID id and SNLID::Bit bit or nullptr if it does not exist.
    SNLBusNetBit* getBusNetBit(SNLID::DesignObjectID id, SNLID::Bit bit) const;
    /// \return SNLScalarNet with SNLName name or nullptr if it does not exist.
    SNLScalarNet* getScalarNet(const SNLName& netName) const;
    /// \return SNLBusNet with SNLIS::DesignObjectID id or nullptr if it does not exist.
    SNLBusNet* getBusNet(SNLID::DesignObjectID id) const;
    /// \return SNLBusNet with SNLName name or nullptr if it does not exist.
    SNLBusNet* getBusNet(const SNLName& netName) const;
    /// \return the collection of SNLNet of this SNLDesign.
    NajaCollection<SNLNet*> getNets() const;

    /**
     * \return the collection of SNLBusNet of this SNLDesign (SNLBusNet subset of getNets()).
     * \see getNets()
     * \see getScalarNets()
     */
    NajaCollection<SNLBusNet*> getBusNets() const;
    
    /**
     * \return the collection of SNLScalarNet of this SNLDesign (SNLScalarNet subset of getNets()).
     * \see getNets()
     * \see getBusNets()
     */
    NajaCollection<SNLScalarNet*> getScalarNets() const;
    
    /// \return the collection of SNLBitNet of this SNLDesign (SNLScalarNet and flattened SNLBusNet to SNLBusNetBit).
    NajaCollection<SNLBitNet*> getBitNets() const;

    /// \return SNLParameter with SNLName name or nullptr if it does not exist.
    SNLParameter* getParameter(const SNLName& name) const;
    /// \return the collection of SNLParameter of this SNLDesign.
    NajaCollection<SNLParameter*> getParameters() const;

    SNLID::DesignID getID() const { return id_; }
    SNLID getSNLID() const;
    SNLID::DesignReference getReference() const;
    
    /// \return SNLName of this SNLDesign. 
    SNLName getName() const { return name_; }
    /// \return true if this SNLDesign is anonymous.
    bool isAnonymous() const { return name_.empty(); }
    
    /// \return this SNLDesign Type.
    Type getType() const { return type_; }
    bool isStandard() const { return type_ == Type::Standard; }
    ///\return true if this SNLDesign is a blackbox.
    bool isBlackBox() const { return type_ == Type::Blackbox; }
    ///\return true if this SNLDesign is a primitive.
    bool isPrimitive() const { return type_ == Type::Primitive; }
    ///\return true if this SNLDesign is a hierarchy leaf (blackbox or primitive).
    bool isLeaf() const { return isBlackBox() or isPrimitive(); }
    ///\return true if this SNLDesign is a top design.
    bool isTopDesign() const;

    bool deepCompare(const SNLDesign* design, std::string& reason) const;
    void mergeAssigns();

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
  private:
    SNLDesign(SNLLibrary* library, Type type, const SNLName& name);
    SNLDesign(SNLLibrary* library, SNLID::DesignID id, Type type, const SNLName& name);
    static void preCreate(const SNLLibrary* library, Type type, const SNLName& name);
    static void preCreate(const SNLLibrary* library, SNLID::DesignID id, Type type, const SNLName& name);
    void destroyFromLibrary();
    void postCreateAndSetID();
    void postCreate();
    void commonPreDestroy();
    void preDestroy() override;
    void addTerm(SNLTerm* term);
    void addTermAndSetID(SNLTerm* term);
    void removeTerm(SNLTerm* term);
    void addInstance(SNLInstance* instance);
    void addInstanceAndSetID(SNLInstance* instance);
    void removeInstance(SNLInstance* instance);
    void addSlaveInstance(SNLInstance* instance);
    void removeSlaveInstance(SNLInstance* instance);
    void addNet(SNLNet* net);
    void addNetAndSetID(SNLNet* net);
    void removeNet(SNLNet* net);
    void rename(SNLTerm* term, const SNLName& previousName);
    void rename(SNLNet* net, const SNLName& previousName);
    void rename(SNLInstance* instance, const SNLName& previousName);
    void addParameter(SNLParameter* parameter);
    void removeParameter(SNLParameter* parameter);
    static bool isBetween(int n, int MSB, int LSB);

    friend bool operator< (const SNLDesign& ld, const SNLDesign& rd) {
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
    using SNLInstanceNameIDMap = std::map<SNLName, SNLID::DesignObjectID>;
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

}} // namespace SNL // namespace naja

#endif // __SNL_DESIGN_H_
