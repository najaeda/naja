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
#include <boost/intrusive/set.hpp>

#include "NajaCollection.h"

#include "SNLDesignObject.h"
#include "SNLID.h"
#include "SNLSharedPath.h"
#include "SNLName.h"
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
      
    using SNLInstanceSharedPathsHook =
      boost::intrusive::member_hook<SNLSharedPath, boost::intrusive::set_member_hook<>, &SNLSharedPath::instanceSharedPathsHook_>;
    using SNLInstanceSharedPaths = boost::intrusive::set<SNLSharedPath, SNLInstanceSharedPathsHook>;

    /**
     * \brief SNLInstance creator.
     * 
     * \param design owner SNLDesign
     * \param model instanciated SNLDesign (model)
     * \param name optional name
     * \return created SNLInstance. 
     */
    static SNLInstance* create(SNLDesign* design, SNLDesign* model, const SNLName& name=SNLName());

    static SNLInstance* create(SNLDesign* design, SNLDesign* model, SNLID::DesignObjectID, const SNLName& name=SNLName());

    SNLDesign* getDesign() const override { return design_; }
    SNLDesign* getModel() const { return model_; }

    SNLID::DesignObjectID getID() const { return id_; }
    SNLID getSNLID() const override;
    SNLID::DesignObjectReference getReference() const;

    SNLName getName() const { return name_; }
    void setName(const SNLName& name);

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

    SNLInstParameter* getInstParameter(const SNLName& name) const;
    NajaCollection<SNLInstParameter*> getInstParameters() const;

    ///\return SNLInstTerm corresponding to the SNLBitTerm representative in this instance. 
    SNLInstTerm* getInstTerm(const SNLBitTerm* term) const;
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
     * Helper function allowing to connect a vector of SNLTerm representatives (SNLInstTerm)
     * in current instance to the corresponding vector of SNLNet bits.
     * \remark terms and nets must have the same size.
     * \remark nets accepts nullptr elements. This method can be used to disconnect SNLInstance terminals (SNLInstTerm).
     **/
    void setTermsNets(const Terms& terms, const Nets& nets);
    /**
     * Helper function allowing to connect a SNLTerm representative in current instance to the
     * corresponding SNLNet bits.
     * \remark term and net must have the same size.
     **/
    void setTermNet(SNLTerm* term, SNLNet* net);
    /**
     * Helper function allowing to connect a SNLTerm representative in current instance to the
     * corresponding SNLNet bits. This version allows to connect a subpart of bits.
     **/
    void setTermNet(
      SNLTerm* term,
      SNLID::Bit termMSB, SNLID::Bit termLSB,
      SNLNet* net,
      SNLID::Bit netMSB, SNLID::Bit netLSB);
    /**
     * Helper function allowing to connect a SNLTerm representative in current instance to the
     * corresponding SNLNet bits. This version allows to connect a subpart of net bits.
     **/
    void setTermNet(
      SNLTerm* term,
      SNLNet* net,
      SNLID::Bit netMSB, SNLID::Bit netLSB);

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

    //SharedPath manipulators
    SNLSharedPath* getSharedPath(const SNLSharedPath* sharedPath) const;
    void addSharedPath(SNLSharedPath* sharedPath);
    void removeSharedPath(SNLSharedPath* sharedPath);

    SNLDesign*                          design_                   {nullptr};
    SNLDesign*                          model_                    {nullptr};
    SNLID::DesignObjectID               id_;
    SNLName                             name_                     {};
    SNLInstanceInstTerms                instTerms_                {};
    SNLInstanceSharedPaths              sharedPaths_              {};
    SNLInstParameters                   instParameters_           {};
    boost::intrusive::set_member_hook<> designInstancesHook_      {};
    boost::intrusive::set_member_hook<> designSlaveInstancesHook_ {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_INSTANCE_H_ 
