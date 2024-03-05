// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_VRL_CONSTRUCTOR_H_
#define __SNL_VRL_CONSTRUCTOR_H_

#include "VerilogConstructor.h"

#include <map>

#include "SNLInstance.h"
#include "SNLNet.h"
#include "SNLTerm.h"

namespace naja { namespace SNL {

class SNLLibrary;
class SNLDesign;
class SNLScalarNet;

class SNLVRLConstructor: public naja::verilog::VerilogConstructor {
  public:
    SNLVRLConstructor() = delete;
    SNLVRLConstructor(const SNLVRLConstructor&) = delete;
    SNLVRLConstructor(SNLLibrary* library);

    static SNLNet::Type VRLTypeToSNLType(const naja::verilog::Net::Type& type);
    static SNLTerm::Direction VRLDirectionToSNLDirection(const naja::verilog::Port::Direction& direction);

    void construct(const std::filesystem::path& filePath);

    void setVerbose(bool verbose) { verbose_ = verbose; }
    bool getVerbose() const { return verbose_; }

    bool inFirstPass() const { return firstPass_; }
    void setFirstPass(bool mode) { firstPass_ = mode; }
    void startModule(const std::string& name) override;
    void moduleInterfaceSimplePort(const std::string& name) override;
    void moduleImplementationPort(const naja::verilog::Port& port) override;
    void moduleInterfaceCompletePort(const naja::verilog::Port& port) override;
    void addNet(const naja::verilog::Net& net) override;
    void addAssign(
      const naja::verilog::Identifiers& identifiers,
      const naja::verilog::Expression& expression) override;
    void startInstantiation(const std::string& modelName) override;
    void addParameterAssignment(
      const std::string& parameterName,
      const naja::verilog::Expression& expression) override;
    void addInstance(const std::string& name) override;
    void endInstantiation() override;
    void addInstanceConnection(
      const std::string& portName,
      const naja::verilog::Expression& expression) override;
    void addOrderedInstanceConnection(
      size_t portIndex,
      const naja::verilog::Expression& expression) override;
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
      const naja::verilog::Identifier& identifier,
      SNLInstance::Nets& bitNets);
    void currentInstancePortConnection(
      SNLTerm* term,
      const naja::verilog::Expression& expression);
    
    std::string getLocationString() const;

    bool              verbose_                        {false};
    bool              firstPass_                      {true};
    SNLLibrary*       library_                        {nullptr};
    SNLDesign*        currentModule_                  {nullptr};
    std::string       currentModelName_               {};
    SNLInstance*      currentInstance_                {nullptr};
    using ParameterValues = std::map<std::string, std::string>;
    ParameterValues   currentInstanceParameterValues_ {};
    SNLScalarNet*     currentModuleAssign0_           {nullptr};
    SNLScalarNet*     currentModuleAssign1_           {nullptr};
    //Following is used when 
    using InterfacePorts = std::vector<std::unique_ptr<naja::verilog::Port>>;
    using InterfacePortsMap = std::map<std::string, size_t>;
    InterfacePorts    currentModuleInterfacePorts_    {};
    InterfacePortsMap currentModuleInterfacePortsMap_ {};
};

}} // namespace SNL // namespace naja

#endif // __NAJA_VERILOG_CONSTRUCTOR_H_
