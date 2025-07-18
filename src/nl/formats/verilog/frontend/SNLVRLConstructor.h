// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_VRL_CONSTRUCTOR_H_
#define __SNL_VRL_CONSTRUCTOR_H_

#include "VerilogConstructor.h"


#include "NLDB0.h"
#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLTerm.h"

namespace naja { namespace NL {

class NLLibrary;
class SNLDesign;
class SNLScalarNet;

class SNLVRLConstructor: public naja::verilog::VerilogConstructor {
  public:
    struct Config {
      bool verbose_             {false};  ///< If true, print debug information.
      bool blackboxDetection_   {true};   ///< If true, detect blackbox designs.
      bool allowUnknownDesigns_ {false};  ///< If true, allow unknown designs to be created as blackbox.
    };

    Config  config_ {};

    //As we don't know how many ports are instanciated before reaching
    //the last one, so we need to store all gate instance characteristics
    //before creating it in endInstantiation; 
    struct GateInstance {
      using Connections = std::vector<naja::verilog::Expression>;
      bool isValid() const;
      void reset();
      std::string getString() const;

      NLDB0::GateType gateType_     { NLDB0::GateType::Unknown };
      std::string     instanceName_ {};
      Connections     connections_  {};
    };

    SNLVRLConstructor() = delete;
    SNLVRLConstructor(const SNLVRLConstructor&) = delete;
    SNLVRLConstructor(NLLibrary* library);

    static SNLNet::Type VRLTypeToSNLType(const naja::verilog::Net::Type& type);
    static SNLTerm::Direction VRLDirectionToSNLDirection(const naja::verilog::Port::Direction& direction);

    using Paths = std::vector<std::filesystem::path>;
    void construct(const Paths& paths);
    void construct(const std::filesystem::path& path);

    void setParseAttributes(bool parseAttributes) { parseAttributes_ = parseAttributes; }
    bool getParseAttributes() const { return parseAttributes_; }

    bool inFirstPass() const { return firstPass_; }
    void setFirstPass(bool mode) { firstPass_ = mode; }
    void startModule(const naja::verilog::Identifier& module) override;
    void moduleInterfaceSimplePort(const naja::verilog::Identifier& port) override;
    void moduleImplementationPort(const naja::verilog::Port& port) override;
    void moduleInterfaceCompletePort(const naja::verilog::Port& port) override;
    void addNet(const naja::verilog::Net& net) override;
    void addAssign(
      const naja::verilog::RangeIdentifiers& identifiers,
      const naja::verilog::Expression& expression) override;
    void startInstantiation(const naja::verilog::Identifier& model) override;
    void addParameterAssignment(
      const naja::verilog::Identifier& parameter,
      const naja::verilog::Expression& expression) override;
    void addInstance(const naja::verilog::Identifier& instance) override;
    void endInstantiation() override;
    void addInstanceConnection(
      const naja::verilog::Identifier& port,
      const naja::verilog::Expression& expression) override;
    void addOrderedInstanceConnection(
      size_t portIndex,
      const naja::verilog::Expression& expression) override;
    void startGateInstantiation(
      const naja::verilog::GateType& gateType) override;
    void addGateInstance(const naja::verilog::Identifier& id) override;
    void addGateOutputInstanceConnection(
      size_t portIndex,
      const naja::verilog::RangeIdentifiers identifiers) override;
    void addGateInputInstanceConnection(
      size_t portIndex,
      const naja::verilog::Expression& expression) override;
    void endGateInstantiation() override;
    void addDefParameterAssignment(
      const naja::verilog::Identifiers& hierarchicalParameter,
      const naja::verilog::ConstantExpression& expression) override;

    using Attributes = std::vector<naja::verilog::Attribute>;
    void addAttribute(
      const naja::verilog::Identifier& attributeName,
      const naja::verilog::ConstantExpression& expression) override;
    void endModule() override;

  private:
    void createCurrentModuleAssignNets();
    void createConstantNets(
      const naja::verilog::Number& number,
      SNLInstance::Nets& nets);
    void collectConcatenationBitNets(
      const naja::verilog::Concatenation& concatenation,
      SNLInstance::Nets& bitNets);
    void collectIdentifierNets(
      const naja::verilog::RangeIdentifier& identifier,
      SNLInstance::Nets& bitNets);
    void currentInstancePortConnection(
      SNLTerm* term,
      const naja::verilog::Expression& expression);
    
    std::string getLocationString() const;

    bool              firstPass_                      {true};
    bool              parseAttributes_                {true};
    NLLibrary*        library_                        {nullptr};
    Attributes        nextObjectAttributes_           {};
    SNLDesign*        currentModule_                  {nullptr};
    std::string       currentModelName_               {};
    GateInstance      currentGateInstance_            {};
    SNLInstance*      currentInstance_                {nullptr};
    using ParameterValues = std::map<std::string, std::string>;
    ParameterValues   currentInstanceParameterValues_ {};
    SNLScalarNet*     currentModuleAssign0_           {nullptr};
    SNLScalarNet*     currentModuleAssign1_           {nullptr};
    using InterfacePorts = std::vector<std::unique_ptr<naja::verilog::Port>>;
    using InterfacePortsMap = std::map<std::string, size_t>;
    InterfacePorts    currentModuleInterfacePorts_    {};
    InterfacePortsMap currentModuleInterfacePortsMap_ {};
};

}} // namespace NL // namespace naja

#endif // __NAJA_VERILOG_CONSTRUCTOR_H_
