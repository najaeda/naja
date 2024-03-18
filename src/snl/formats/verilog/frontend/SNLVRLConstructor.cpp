// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLVRLConstructor.h"

#include <iostream>
#include <sstream>

#include "SNLUniverse.h"
#include "SNLDB0.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLBusTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLScalarNet.h"
#include "SNLInstParameter.h"

#include "SNLVRLConstructorUtils.h"
#include "SNLVRLConstructorException.h"

namespace {

void createPort(naja::SNL::SNLDesign* design, const naja::verilog::Port& port) {
  if (port.isBus()) {
    naja::SNL::SNLBusTerm::create(
      design,
      naja::SNL::SNLVRLConstructor::VRLDirectionToSNLDirection(port.direction_),
      port.range_.msb_,
      port.range_.lsb_,
      naja::SNL::SNLName(port.identifier_.name_));
  } else {
    naja::SNL::SNLScalarTerm::create(
      design,
      naja::SNL::SNLVRLConstructor::VRLDirectionToSNLDirection(port.direction_),
      naja::SNL::SNLName(port.identifier_.name_));
  }
}

void createPortNet(naja::SNL::SNLDesign* design, const naja::verilog::Port& port) {
  if (port.isBus()) {
    auto term = design->getBusTerm(naja::SNL::SNLName(port.identifier_.name_));
    auto net = naja::SNL::SNLBusNet::create(
      design,
      port.range_.msb_,
      port.range_.lsb_,
      naja::SNL::SNLName(port.identifier_.name_));
    term->setNet(net);
  } else {
    auto term = design->getScalarTerm(naja::SNL::SNLName(port.identifier_.name_));
    assert(term);
    auto net = naja::SNL::SNLScalarNet::create(design,
      naja::SNL::SNLName(port.identifier_.name_));
    term->setNet(net);
  }
}

}

namespace naja { namespace SNL {

SNLTerm::Direction
SNLVRLConstructor::VRLDirectionToSNLDirection(const naja::verilog::Port::Direction& direction) {
  switch (direction) {
    case naja::verilog::Port::Direction::Input:
      return naja::SNL::SNLTerm::Direction::Input;
    case naja::verilog::Port::Direction::Output:
      return naja::SNL::SNLTerm::Direction::Output;
    case naja::verilog::Port::Direction::InOut:
      return naja::SNL::SNLTerm::Direction::InOut;
    case naja::verilog::Port::Direction::Unknown: {
      std::ostringstream reason;
      reason << "Unsupported verilog direction";
      throw naja::SNL::SNLVRLConstructorException(reason.str());
    }
  }
  return naja::SNL::SNLTerm::Direction::Input; //LCOV_EXCL_LINE
}

SNLNet::Type
SNLVRLConstructor::VRLTypeToSNLType(const naja::verilog::Net::Type& type) {
  switch(type) {
    case naja::verilog::Net::Type::Wire:
      return naja::SNL::SNLNet::Type::Standard;
    case naja::verilog::Net::Type::Supply0:
      return naja::SNL::SNLNet::Type::Supply0;
    case naja::verilog::Net::Type::Supply1:
      return naja::SNL::SNLNet::Type::Supply1;
    case naja::verilog::Net::Type::Unknown: {
      std::ostringstream reason;
      reason << "Unsupported verilog net type";
      throw naja::SNL::SNLVRLConstructorException(reason.str());
    }
  }
  return naja::SNL::SNLNet::Type::Standard; //LCOV_EXCL_LINE
}

void SNLVRLConstructor::createCurrentModuleAssignNets() {
  currentModuleAssign0_ = naja::SNL::SNLScalarNet::create(currentModule_);
  currentModuleAssign0_->setType(naja::SNL::SNLNet::Type::Assign0);
  currentModuleAssign1_ = naja::SNL::SNLScalarNet::create(currentModule_);
  currentModuleAssign1_->setType(naja::SNL::SNLNet::Type::Assign1);
} 

void SNLVRLConstructor::createConstantNets(
  const naja::verilog::Number& number,
  naja::SNL::SNLInstance::Nets& nets) {
  if (number.value_.index() != naja::verilog::Number::BASED) {
    std::ostringstream reason;
    reason << getLocationString();
    reason << ": Only base numbers are supported"; 
    throw naja::SNL::SNLVRLConstructorException(reason.str());
  }
  auto basedNumber = std::get<naja::verilog::Number::BASED>(number.value_);
  auto bits = SNLVRLConstructorUtils::numberToBits(basedNumber);
  if (bits.size() != basedNumber.size_) {
    std::ostringstream reason;
    reason << getLocationString();
    reason << ": " << "Size";
    throw naja::SNL::SNLVRLConstructorException(reason.str());
  }
  for (int i=bits.size()-1; i>=0; i--) {
    if (bits[i]) {
      nets.push_back(currentModuleAssign1_);
    } else {
      nets.push_back(currentModuleAssign0_);
    }
  }
}

std::string SNLVRLConstructor::getLocationString() const {
  std::ostringstream stream;
  auto location = getCurrentLocation();
  stream << "In "
    << location.currentPath_.string()
    << " at line " << location.line_
    << ", column " << location.column_;
  return stream.str();
}

SNLVRLConstructor::SNLVRLConstructor(SNLLibrary* library):
  library_(library)
{}

void SNLVRLConstructor::construct(const Paths& paths) {
  setFirstPass(true);
  parse(paths);
  setFirstPass(false);
  parse(paths);
}

void SNLVRLConstructor::construct(const std::filesystem::path& path) {
  construct(Paths{path});
}

void SNLVRLConstructor::startModule(const naja::verilog::Identifier& module) {
  if (inFirstPass()) {
    currentModule_ = SNLDesign::create(library_, SNLName(module.name_));
    if (verbose_) {
      std::cerr << "Construct Module: " << module.getString() << std::endl; //LCOV_EXCL_LINE
    }
  } else {
    currentModule_ = library_->getDesign(SNLName(module.name_));
    if (not currentModule_) {
      std::ostringstream reason;
      reason << "In SNLVRLConstructor second pass, ";
      reason << module.getString() << " module cannot be found in library: ";
      reason << library_->getDescription();
      throw SNLVRLConstructorException(reason.str());
    }
    if (not currentModule_->getNets().empty()) {
      std::ostringstream reason;
      reason << "In SNLVRLConstructor second pass, ";
      reason << module.getString() << " module should no contain any net";
      throw SNLVRLConstructorException(reason.str());
    }
    createCurrentModuleAssignNets();
  } 
}

void SNLVRLConstructor::moduleInterfaceSimplePort(const naja::verilog::Identifier& port) {
  if (inFirstPass()) {
    if (verbose_) {
      std::cerr << "Add port: " << port.getString() << std::endl; //LCOV_EXCL_LINE
    }
    if (currentModuleInterfacePortsMap_.find(port.name_) != currentModuleInterfacePortsMap_.end()) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": port collision in module " << currentModule_->getName().getString();
      reason << ", " << port.getString() << " has already been declared.";
      throw SNLVRLConstructorException(reason.str());
    }
    auto index = currentModuleInterfacePortsMap_.size();
    currentModuleInterfacePorts_.push_back(nullptr);
    currentModuleInterfacePortsMap_[port.name_] = index;
  }
}

void SNLVRLConstructor::moduleImplementationPort(const naja::verilog::Port& port) {
  if (inFirstPass()) {
    if (verbose_) {
      //LCOV_EXCL_START
      std::cerr << "Add implementation port: "
        << port.getString() << std::endl;
      //LCOV_EXCL_STOP
    }
    auto it = currentModuleInterfacePortsMap_.find(port.identifier_.name_);
    if (it == currentModuleInterfacePortsMap_.end()) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": undeclared port in module " << currentModule_->getName().getString();
      reason << ", " << port.getString() << " is unknown in module interface.";
      throw SNLVRLConstructorException(reason.str());
    }
    auto index = it->second;
    currentModuleInterfacePorts_[index] = std::make_unique<naja::verilog::Port>(port);
  } else {
    createPortNet(currentModule_, port);
  }
}

void SNLVRLConstructor::moduleInterfaceCompletePort(const naja::verilog::Port& port) {
  if (inFirstPass()) {
    if (verbose_) {
      std::cerr << "Add port: " << port.getString() << std::endl; //LCOV_EXCL_LINE
    }
    createPort(currentModule_, port);
  } else {
    createPortNet(currentModule_, port);
  }
}

void SNLVRLConstructor::addNet(const naja::verilog::Net& net) {
  if (not inFirstPass()) {
    if (verbose_) {
      std::cerr << "Add net: " << net.getString() << std::endl; //LCOV_EXCL_LINE
    }
    auto netName = SNLName(net.identifier_.name_);
    auto existingNet = currentModule_->getNet(netName);
    if (existingNet) {
      //net might be existing because of a port declaration
      auto term = currentModule_->getTerm(netName);
      if (term) {
        return;
      } else {
        std::ostringstream reason;
        reason << getLocationString();
        reason << ": wire collision for net " << net.identifier_.name_;
        throw SNLVRLConstructorException(reason.str());
      }
    }
    try {
      SNLNet* snlNet = nullptr;
      if (net.isBus()) {
        snlNet = SNLBusNet::create(currentModule_, net.range_.msb_, net.range_.lsb_, SNLName(net.identifier_.name_));
      } else {
        snlNet = SNLScalarNet::create(currentModule_, SNLName(net.identifier_.name_));
      }
      snlNet->setType(VRLTypeToSNLType(net.type_));
    //LCOV_EXCL_START
    } catch (const SNLException& exception) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": " << exception.getReason();
      throw SNLVRLConstructorException(reason.str());
    }
    //LCOV_EXCL_STOP
  }
}

void SNLVRLConstructor::addAssign(
  const naja::verilog::RangeIdentifiers& identifiers,
  const naja::verilog::Expression& expression) {
  if (inFirstPass()) {
    return;
  }
  using BitNets = std::vector<SNLBitNet*>;
  BitNets leftNets;
  BitNets rightNets;
  if (verbose_) {
    std::cerr << "Assign: " << std::endl; //LCOV_EXCL_LINE
  }
  for (auto id: identifiers) {
    collectIdentifierNets(id, leftNets);
    if (verbose_) {
      std::cerr << "ID: " << id.getString() << std::endl; //LCOV_EXCL_LINE
    }
  }
  if (verbose_) {
    std::cerr << expression.getString() << std::endl; //LCOV_EXCL_LINE
  }
  if (expression.valid_) {
    if (not expression.supported_) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": " << expression.getString() << " is not currently supported";
      throw SNLVRLConstructorException(reason.str());
    }
    switch (expression.value_.index()) {
      case naja::verilog::Expression::NUMBER: {
        auto number =
          std::get<naja::verilog::Expression::Type::NUMBER>(expression.value_);
        createConstantNets(number, rightNets);
        break;
      }
      case naja::verilog::Expression::Type::RANGEIDENTIFIER: {
        auto identifier = std::get<naja::verilog::RangeIdentifier>(expression.value_);
        std::string name = identifier.identifier_.name_;
        SNLNet* net = currentModule_->getNet(SNLName(name));
        if (not net) {
          std::ostringstream reason;
          reason << getLocationString();
          reason << ": net \"" <<  name
            << "\" cannot be found in "
            << currentModule_->getName().getString();
          throw SNLVRLConstructorException(reason.str());
        }
        if (identifier.range_.valid_) {
          SNLBusNet* busNet = dynamic_cast<SNLBusNet*>(net);
          if (not busNet) {
            std::ostringstream reason;
            reason << getLocationString() << " NOT BUSNET"; 
            throw SNLVRLConstructorException(reason.str());
          }
          int netMSB = identifier.range_.msb_;
          int netLSB = netMSB;
          if (not identifier.range_.singleValue_) {
            netLSB = identifier.range_.lsb_;
          }
          busNet->insertBits(rightNets, rightNets.begin(), netMSB, netLSB);
        } else {
          rightNets.insert(rightNets.end(),
            net->getBits().begin(), net->getBits().end());
        }
        break;
      }
      case naja::verilog::Expression::Type::CONCATENATION: {
        SNLInstance::Nets bitNets;
        auto concatenation = std::get<naja::verilog::Concatenation>(expression.value_);
        collectConcatenationBitNets(concatenation, rightNets);
        break;
      }
      default: {
        std::ostringstream reason;
        reason << expression.getString() << " type is not supported in instance connection";
        throw SNLVRLConstructorException(reason.str());
      }
    }
  }
  assert(leftNets.size() == rightNets.size());
  for (size_t i=0; i<leftNets.size(); ++i) {
    auto leftNet = leftNets[i];
    auto rightNet = rightNets[i];
    auto assign = SNLInstance::create(currentModule_, SNLDB0::getAssign());
    assign->setTermNet(SNLDB0::getAssignOutput(), leftNet);
    assign->setTermNet(SNLDB0::getAssignInput(), rightNet);
  }
}

void SNLVRLConstructor::startInstantiation(const naja::verilog::Identifier& model) {
  if (not inFirstPass()) {
    currentModelName_ = model.name_;
    if (verbose_) {
      std::cerr << "Start Instantiation: " << model.getString() << std::endl; //LCOV_EXCL_LINE
    }
  }
}

void SNLVRLConstructor::addInstance(const naja::verilog::Identifier& instance) {
  if (not inFirstPass()) {
    assert(not currentModelName_.empty());
    SNLName modelName(currentModelName_);
    SNLDesign* model = library_->getDesign(modelName);
    if (not model) {
      model = library_->getDB()->getDesign(modelName);
    }
    if (not model) {
      model = SNLUniverse::get()->getDesign(modelName);
    }
    if (not model) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": " << currentModelName_
        << " cannot be found in SNL while constructing instance "
        << instance.getString();
      throw SNLVRLConstructorException(reason.str());
    }
    //FIXME
    //might be a good idea to create a cache <Name, SNLDesign*> here
    //in particular for primitives
    currentInstance_ = SNLInstance::create(currentModule_, model, SNLName(instance.name_));
  }
}

void SNLVRLConstructor::addParameterAssignment(
  const naja::verilog::Identifier& parameter,
  const naja::verilog::Expression& expression) {
  if (not inFirstPass()) {
    currentInstanceParameterValues_[parameter.name_] = expression.getString();
  }
}

void SNLVRLConstructor::endInstantiation() {
  if (not inFirstPass()) {
    assert(currentInstance_);
    if (verbose_) {
      std::cerr << "End " << currentInstance_->getString() 
        << " instantiation" << std::endl; //LCOV_EXCL_LINE
    }
    //Save current Parameter assignments
    for (auto parameterValue: currentInstanceParameterValues_) {
      auto parameterName = SNLName(parameterValue.first);
      auto parameter = currentInstance_->getModel()->getParameter(parameterName);
      if (not parameter) {
        std::ostringstream reason;
        reason << getLocationString();
        reason << ": " << currentModelName_
          << " does not contain any Parameter named " << parameterValue.first;
        throw SNLVRLConstructorException(reason.str());
      }
      SNLInstParameter::create(currentInstance_, parameter, parameterValue.second);
    }
    currentInstanceParameterValues_.clear();
    currentInstance_ = nullptr;
    currentModelName_ = std::string();
  }
}

void SNLVRLConstructor::currentInstancePortConnection(
  naja::SNL::SNLTerm* term,
  const naja::verilog::Expression& expression) {
  if (expression.valid_) {
    if (not expression.supported_) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": " << expression.getString() << " is not currently supported";
      throw SNLVRLConstructorException(reason.str());
    }
    switch (expression.value_.index()) {
      case naja::verilog::Expression::NUMBER: {
        auto number =
          std::get<naja::verilog::Expression::Type::NUMBER>(expression.value_);
        SNLInstance::Terms bitTerms;
        if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
          bitTerms.push_back(scalarTerm);
        } else {
          auto busTerm = static_cast<SNLBusTerm*>(term);
          bitTerms = SNLInstance::Terms(busTerm->getBits().begin(), busTerm->getBits().end());
        }
        SNLInstance::Nets bitNets;
        createConstantNets(number, bitNets);
        currentInstance_->setTermsNets(bitTerms, bitNets);
        break;
      }
      case naja::verilog::Expression::Type::RANGEIDENTIFIER: {
        auto identifier = std::get<naja::verilog::RangeIdentifier>(expression.value_);
        std::string name = identifier.identifier_.name_;
        SNLNet* net = currentInstance_->getDesign()->getNet(SNLName(name));
        if (not net) {
          std::ostringstream reason;
          reason << getLocationString();
          reason << ": net \"" <<  name
            << "\" cannot be found in "
            << currentInstance_->getDesign()->getName().getString();
          throw SNLVRLConstructorException(reason.str());
        }
        if (identifier.range_.valid_) {
          SNLBusNet* busNet = dynamic_cast<SNLBusNet*>(net);
          if (not busNet) {
            std::ostringstream reason;
            reason << getLocationString() << " NOT BUSTERM"; 
            throw SNLVRLConstructorException(reason.str());
          }
          int netMSB = identifier.range_.msb_;
          int netLSB = netMSB;
          if (not identifier.range_.singleValue_) {
            netLSB = identifier.range_.lsb_;
          }
          currentInstance_->setTermNet(term, net, netMSB, netLSB);
        } else {
          currentInstance_->setTermNet(term, net);
        }
        break;
      }
      case naja::verilog::Expression::Type::CONCATENATION: {
        SNLInstance::Nets bitNets;
        auto concatenation = std::get<naja::verilog::Concatenation>(expression.value_);
        collectConcatenationBitNets(concatenation, bitNets);
        using BitTerms = std::vector<SNLBitTerm*>;
        auto busTerm = dynamic_cast<SNLBusTerm*>(term);
        if (not busTerm) {
          std::ostringstream reason;
          reason << getLocationString() << ": NOT BUSTERM";
          throw SNLVRLConstructorException(reason.str());
        }
        BitTerms bitTerms(busTerm->getBits().begin(), busTerm->getBits().end());
        assert(bitTerms.size() == bitNets.size());
        currentInstance_->setTermsNets(bitTerms, bitNets);
        break;
      }
      default: {
        std::ostringstream reason;
        reason << expression.getString() << " type is not supported in instance connection";
        throw SNLVRLConstructorException(reason.str());
      }
    }
  }
}

void SNLVRLConstructor::addInstanceConnection(
  const naja::verilog::Identifier& port,
  const naja::verilog::Expression& expression) {
  if (not inFirstPass()) {
    assert(currentInstance_);
    SNLDesign* model = currentInstance_->getModel();
    SNLTerm* term = model->getTerm(SNLName(port.name_));
    if (not term) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": " << port.getString()
        << " port cannot be found in " << model->getName().getString()
        << " model";
      throw SNLVRLConstructorException(reason.str());
    }
    currentInstancePortConnection(term, expression);
    if (verbose_) {
      //LCOV_EXCL_START
      std::cerr << "Instance connection: "
        << currentInstance_->getString()
        << " - " << term->getString() << " connection"
        << std::endl;
      //LCOV_EXCL_STOP
    }
  }
}

void SNLVRLConstructor::addOrderedInstanceConnection(
  size_t portIndex,
  const naja::verilog::Expression& expression) {
  if (not inFirstPass()) {
    assert(currentInstance_);
    SNLDesign* model = currentInstance_->getModel();
    SNLTerm* term = model->getTerm(SNLID::DesignObjectID(portIndex));
    currentInstancePortConnection(term, expression);
    if (verbose_) {
      //LCOV_EXCL_START
      std::cerr << "Instance connection: "
        << currentInstance_->getString()
        << " - " << term->getString() << " connection"
        << std::endl;
      //LCOV_EXCL_STOP
    }
  }
}

void SNLVRLConstructor::endModule() {
  if (verbose_) {
    //LCOV_EXCL_START
    std::cerr << "End module: "
      << currentModule_->getString() << std::endl;
    //LCOV_EXCL_STOP
  }
  if (inFirstPass()) {
    //construct interface declared ports if existing
    //verify that all ports declared interface
    //have their declaration in implementation part: no nullptr in vector
    auto findIt = find(currentModuleInterfacePorts_.begin(), currentModuleInterfacePorts_.end(), nullptr);
    if (findIt != currentModuleInterfacePorts_.end()) {
      auto position = findIt - currentModuleInterfacePorts_.begin();
      //undeclared port
      //search in all map for same index, very long
      //but this is just to produce message error
      std::string portName;
      for (const auto& [key, value]: currentModuleInterfacePortsMap_) {
        if (value == position) {
          portName = key;
        }
      }
      std::ostringstream reason;
      reason << portName << " declared in interface has no declaration in module implementation.";
      throw SNLVRLConstructorException(reason.str());
    }
    for (auto& port: currentModuleInterfacePorts_) {
      createPort(currentModule_, *port);
    }
  }
  currentModule_ = nullptr;
  currentModuleInterfacePortsMap_.clear();
  currentModuleInterfacePorts_.clear();
  currentModuleAssign0_ = nullptr;
  currentModuleAssign1_ = nullptr;
}

void SNLVRLConstructor::collectConcatenationBitNets(
  const naja::verilog::Concatenation& concatenation,
  SNLInstance::Nets& bitNets) {
  for (auto expression: concatenation.expressions_) {
    if (not expression.supported_ or not expression.valid_) {
      std::ostringstream reason;
      reason << naja::SNL::SNLVRLConstructor::getLocationString();
      reason << ": " << expression.getString() << " is not supported";
      throw SNLVRLConstructorException(reason.str());
    }
    switch (expression.value_.index()) {
      case naja::verilog::Expression::NUMBER: {
        auto number =
          std::get<naja::verilog::Expression::Type::NUMBER>(expression.value_);
        createConstantNets(number, bitNets);
        break;
      }
      case naja::verilog::Expression::RANGEIDENTIFIER: {
        auto identifier =
          std::get<naja::verilog::Expression::Type::RANGEIDENTIFIER>(expression.value_);
        std::string name = identifier.identifier_.name_;
        SNLNet* net = currentModule_->getNet(SNLName(name));
        if (not net) {
          std::ostringstream reason;
          reason << getLocationString();
          reason << ": net \"" <<  name
            << "\" cannot be found in \""
            << currentModule_->getName().getString()
            << "\"";
          throw SNLVRLConstructorException(reason.str());
        }
        if (auto scalarNet = dynamic_cast<SNLScalarNet*>(net)) {
          if (identifier.range_.valid_) {
            std::ostringstream reason;
            reason << expression.getString() << " is not supported (scalar-range)";
            throw SNLVRLConstructorException(reason.str());
          }
          bitNets.push_back(scalarNet);
        } else {
          auto busNet = static_cast<SNLBusNet*>(net);
          if (not identifier.range_.valid_) {
            bitNets.insert(bitNets.end(), busNet->getBits().begin(), busNet->getBits().end());
          } else {
            int msb = identifier.range_.msb_;
            if (identifier.range_.singleValue_) {
              bitNets.push_back(busNet->getBit(msb));
            } else {
              int lsb = identifier.range_.lsb_;
              int incr = msb<lsb?+1:-1;
              for (int i=msb; i!=lsb+incr; i+=incr) {
                auto bit = busNet->getBit(i);
                bitNets.push_back(bit);
              }
            }
          }
        }
        break;
      }
      default: {
        std::ostringstream reason;
        reason << expression.getString() << " type is not supported";
        throw SNLVRLConstructorException(reason.str());
      }
    }
  }
}

void SNLVRLConstructor::collectIdentifierNets(
  const naja::verilog::RangeIdentifier& identifier,
  SNLInstance::Nets& bitNets) {
  std::string name = identifier.identifier_.name_;
  SNLNet* net = currentModule_->getNet(SNLName(name));
  if (not net) {
    std::ostringstream reason;
    reason << getLocationString();
    reason << ": net \"" <<  name
      << "\" cannot be found in "
      << currentModule_->getName().getString();
    throw SNLVRLConstructorException(reason.str());
  }
  if (identifier.range_.valid_) {
    SNLBusNet* busNet = dynamic_cast<SNLBusNet*>(net);
    if (not busNet) {
      std::ostringstream reason;
      reason << getLocationString() << " NOT BUSNET"; 
      throw SNLVRLConstructorException(reason.str());
    }
    int netMSB = identifier.range_.msb_;
    int netLSB = netMSB;
    if (not identifier.range_.singleValue_) {
      netLSB = identifier.range_.lsb_;
    }
    busNet->insertBits(bitNets, bitNets.end(), netMSB, netLSB);
  } else {
    bitNets.insert(bitNets.end(),
      net->getBits().begin(), net->getBits().end());
  }
}

}} // namespace SNL // namespace naja
