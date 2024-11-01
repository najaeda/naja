// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_INSTANCE_H_
#define __SNL_INSTANCE_H_

#include <vector>
#include <map>
#include <boost/intrusive/set.hpp>

#include "NajaCollection.h"

#include "SNLDesignObject.h"
#include "SNLID.h"
#include "SNLSharedPath.h"
#include "SNLInstParameter.h"

namespace naja { namespace SNL {

class SNLTerm;
class SNLNet;
class SNLBitTerm;
class SNLBitNet;
class SNLInstTerm;

class SNLInstance final: public SNLDesignObject {
  public:
    friend class SNLDesign;
    friend class SNLInstParameter;
    friend class SNLSharedPath;
    friend class SNLPath;
    using super = SNLDesignObject;
    using SNLInstanceInstTerms = std::vector<SNLInstTerm*>;
    using SNLInstParametersHook =
      boost::intrusive::member_hook<SNLInstParameter, boost::intrusive::set_member_hook<>, &SNLInstParameter::instParametersHook_>;
    using SNLInstParameters = boost::intrusive::set<SNLInstParameter, SNLInstParametersHook>;
      

    /**
     * \brief Create a SNLInstance.
     * \param design owner SNLDesign.
     * \param model instanciated SNLDesign (model).
     * \param name optional name.
     * \return created SNLInstance. 
     */
    static SNLInstance* create(SNLDesign* design, SNLDesign* model, const SNLName& name=SNLName());

    /**
     * \brief Create a SNLInstance with a given SNLID::DesignObjectID.
     * \param design owner SNLDesign.
     * \param model instanciated SNLDesign (model).
     * \param id SNLID::DesignObjectID of the instance.
     * \param name optional name.
     * \return created SNLInstance. 
     */
    static SNLInstance* create(SNLDesign* design, SNLDesign* model, SNLID::DesignObjectID id, const SNLName& name=SNLName());

    SNLDesign* getDesign() const override { return design_; }
    /// \return the instanciated SNLDesign (model).
    SNLDesign* getModel() const { return model_; }

    /// \return this SNLInstance id. Positional id in parent SNLDesign.
    SNLID::DesignObjectID getID() const { return id_; }
    SNLID getSNLID() const override;
    SNLID::DesignObjectReference getReference() const;
    bool deepCompare(const SNLInstance* other, std::string& reason) const;

    /// \return this SNLInstance name.
    SNLName getName() const { return name_; }
    /**
     * \brief Set this SNLInstance name.
     * \param name new SNLInstance name. 
     */
    void setName(const SNLName& name) override;

    bool isAnonymous() const override { return name_.empty(); }
    ///\return true if this SNLInstance is a blackbox.
    bool isBlackBox() const;
    ///\return true if this SNLInstance is a primitive.
    bool isPrimitive() const;
    ///\return true if this SNLInstance is a hierarchy leaf (blackbox or primitive).
    bool isLeaf() const;

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

    SNLInstParameter* getInstParameter(const SNLName& name) const;
    NajaCollection<SNLInstParameter*> getInstParameters() const;

    ///\return SNLInstTerm corresponding to the SNLBitTerm representative in this instance. 
    SNLInstTerm* getInstTerm(const SNLBitTerm* bitTerm) const;
    SNLInstTerm* getInstTerm(const SNLID::DesignObjectID termID) const;
    ///\return the NajaCollection of all SNLInstTerm of this SNLInstance.
    NajaCollection<SNLInstTerm*> getInstTerms() const;
    /**
     * \return the NajaCollection subset of connected SNLInstTerm of this SNLInstance.
     * \remark A SNLInstTerm is connected if instTerm->getNet() is not NULL.
     **/
    NajaCollection<SNLInstTerm*> getConnectedInstTerms() const;
    ///\return the NajaCollection subset of SNLInstTerm (only SNLScalarTerm type) of this SNLInstance.
    NajaCollection<SNLInstTerm*> getInstScalarTerms() const;
    ///\return the NajaCollection subset of SNLInstTerm (only SNLBusTermBit type) of this SNLInstance.
    NajaCollection<SNLInstTerm*> getInstBusTermBits() const;

    using Terms = std::vector<SNLBitTerm*>;
    using Nets = std::vector<SNLBitNet*>;
    /**
     * \brief function allowing to connect a vector of SNLTerm representatives (SNLInstTerm)
     * in current instance to the corresponding vector of SNLNet bits.
     * \remark terms and nets must have the same size.
     * \remark nets accepts nullptr elements. This method can be used to disconnect SNLInstance terminals (SNLInstTerm).
     */
    void setTermsNets(const Terms& terms, const Nets& nets);
    /**
     * \brief Helper function allowing to connect a SNLTerm representative in current instance to the
     * corresponding SNLNet bits.
     * \remark term and net must have the same size.
     */
    void setTermNet(SNLTerm* term, SNLNet* net);
    /**
     * \brief Helper function allowing to connect a SNLTerm representative in current instance to the
     * corresponding SNLNet bits. This version allows to connect a subpart of bits.
     */
    void setTermNet(
      SNLTerm* term,
      SNLID::Bit termMSB, SNLID::Bit termLSB,
      SNLNet* net,
      SNLID::Bit netMSB, SNLID::Bit netLSB);
    /**
     * \brief Helper function allowing to connect a SNLTerm representative in current instance to the
     * corresponding SNLNet bits. This version allows to connect a subpart of net bits.
     */
    void setTermNet(
      SNLTerm* term,
      SNLNet* net,
      SNLID::Bit netMSB, SNLID::Bit netLSB);

    /**
     * \brief Seting new model for this instance.
     * \param newModel new model for this instance.
     */
    void setModel(SNLDesign* newModel);

    SNLID::DesignObjectID getOrderID() const { return orederID_; }
    void setOrderID(SNLID::DesignObjectID orderID) { orederID_ = orderID; }

  private:
    SNLInstance(SNLDesign* design, SNLDesign* model, const SNLName& name);
    SNLInstance(SNLDesign* design, SNLDesign* model, SNLID::DesignObjectID id, const SNLName& name);
    static void preCreate(SNLDesign* design, const SNLDesign* model, const SNLName& name);
    static void preCreate(SNLDesign* design, const SNLDesign* model, SNLID::DesignObjectID id, const SNLName& name);
    void commonPostCreate();
    void postCreateAndSetID();
    void postCreate();
    void commonPreDestroy();
    void addInstParameter(SNLInstParameter* instParameter);
    void removeInstParameter(SNLInstParameter* instParameter);
    void destroyFromDesign();
    void destroyFromModel();
    void preDestroy() override;
    void createInstTerm(SNLBitTerm* term);
    void removeInstTerm(SNLBitTerm* term);
    SNLInstance* clone(SNLDesign* design) const;

    //SharedPath manipulators
    SNLSharedPath* getSharedPath(const SNLSharedPath* sharedPath) const;
    void addSharedPath(SNLSharedPath* sharedPath);
    void removeSharedPath(SNLSharedPath* sharedPath);

    using SNLInstanceSharedPaths = std::map<const SNLSharedPath*, SNLSharedPath*>;
    SNLDesign*                          design_                   {nullptr};
    SNLDesign*                          model_                    {nullptr};
    SNLID::DesignObjectID               id_;
    SNLName                             name_                     {};
    SNLInstanceInstTerms                instTerms_                {};
    SNLInstanceSharedPaths              sharedPaths_              {};
    SNLInstParameters                   instParameters_           {};
    boost::intrusive::set_member_hook<> designInstancesHook_      {};
    boost::intrusive::set_member_hook<> designSlaveInstancesHook_ {};
    SNLID::DesignObjectID orederID_ = (SNLID::DesignObjectID) -1;
};

}} // namespace SNL // namespace naja

#endif // __SNL_INSTANCE_H_ 