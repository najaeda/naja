// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLVRLConstructor.h"

#include <iostream>
#include <sstream>

#include "NLUniverse.h"
#include "NLDB0.h"
#include "NLLibrary.h"

#include "SNLDesign.h"
#include "SNLBusTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLScalarNet.h"
#include "SNLInstTerm.h"
#include "SNLInstParameter.h"
#include "SNLAttributes.h"

#include "SNLVRLConstructorUtils.h"
#include "SNLVRLConstructorException.h"

namespace {

void collectAttributes(
  naja::NL::NLObject* object,
  const naja::NL::SNLVRLConstructor::Attributes& attributes) {
  if (auto design = dynamic_cast<naja::NL::SNLDesign*>(object)) {
    for (const auto attribute: attributes) {
      naja::NL::NLName attributeName(attribute.name_.getString());
      std::string expression;
      naja::NL::SNLAttributeValue::Type valueType;
      switch (attribute.expression_.getType()) {
        case naja::verilog::ConstantExpression::Type::STRING:
          valueType = naja::NL::SNLAttributeValue::Type::STRING;
          break;
        case naja::verilog::ConstantExpression::Type::NUMBER:
          valueType = naja::NL::SNLAttributeValue::Type::NUMBER;
          break;
      }
      if (attribute.expression_.valid_) {
        expression = attribute.expression_.getString();
      }
      naja::NL::SNLAttributes::addAttribute(
        design,
        naja::NL::SNLAttribute(
          attributeName,
          naja::NL::SNLAttributeValue(valueType, expression)));
    }
  } else if (auto designObject = dynamic_cast<naja::NL::SNLDesignObject*>(object)) {
    for (const auto attribute: attributes) {
      naja::NL::NLName attributeName(attribute.name_.getString());
      std::string expression;
      naja::NL::SNLAttributeValue::Type valueType;
      switch (attribute.expression_.getType()) {
        case naja::verilog::ConstantExpression::Type::STRING:
          valueType = naja::NL::SNLAttributeValue::Type::STRING;
          break;
        case naja::verilog::ConstantExpression::Type::NUMBER:
          valueType = naja::NL::SNLAttributeValue::Type::NUMBER;
          break;
      }
      if (attribute.expression_.valid_) {
        expression = attribute.expression_.getString();
      }
      naja::NL::SNLAttributes::addAttribute(
        designObject,
        naja::NL::SNLAttribute(
          attributeName,
          naja::NL::SNLAttributeValue(valueType, expression)));
    }
  }
}

void createPort(
  naja::NL::SNLDesign* design,
  const naja::verilog::Port& port,
  const naja::NL::SNLVRLConstructor::Attributes& attributes) {
  //spdlog::trace("Module {} create port: {}", design->getDescription(), port.getString());
  naja::NL::SNLTerm* term = nullptr;
  if (port.isBus()) {
    term = naja::NL::SNLBusTerm::create(
      design,
      naja::NL::SNLVRLConstructor::VRLDirectionToSNLDirection(port.direction_),
      port.range_.msb_,
      port.range_.lsb_,
      naja::NL::NLName(port.identifier_.name_));
  } else {
    term = naja::NL::SNLScalarTerm::create(
      design,
      naja::NL::SNLVRLConstructor::VRLDirectionToSNLDirection(port.direction_),
      naja::NL::NLName(port.identifier_.name_));
  }
  collectAttributes(term, attributes);
}

void createPortNet(naja::NL::SNLDesign* design, const naja::verilog::Port& port) {
  //spdlog::trace("Module {} create port net: {}", design->getDescription(), port.getString());
  if (port.isBus()) {
    auto term = design->getBusTerm(naja::NL::NLName(port.identifier_.name_));
    auto net = naja::NL::SNLBusNet::create(
      design,
      port.range_.msb_,
      port.range_.lsb_,
      naja::NL::NLName(port.identifier_.name_));
    term->setNet(net);
  } else {
    auto term = design->getScalarTerm(naja::NL::NLName(port.identifier_.name_));
    assert(term);
    auto net = naja::NL::SNLScalarNet::create(design,
      naja::NL::NLName(port.identifier_.name_));
    term->setNet(net);
  }
}

bool allNetsArePortNets(naja::NL::SNLDesign* design) {
  for (auto net: design->getBitNets()) {
    if (net->isAssignConstant()) {
      continue;
    }
    if (net->getBitTerms().empty()) {
      //LCOV_EXCL_START
      //spdlog::info("Net {} in {} is not a port net", 
        //net->getName().getString(),
        //design->getName().getString());
      return false;
      //LCOV_EXCL_STOP
    }
  }
  return true;
}

naja::NL::NLDB0::GateType najaVerilogToNLDB0GateType(const naja::verilog::GateType& gateType) {
  switch (gateType) {
    case naja::verilog::GateType::And:
      return naja::NL::NLDB0::GateType::And;
    case naja::verilog::GateType::Nand:
      return naja::NL::NLDB0::GateType::Nand;
    case naja::verilog::GateType::Or:
      return naja::NL::NLDB0::GateType::Or;
    case naja::verilog::GateType::Nor:
      return naja::NL::NLDB0::GateType::Nor;
    case naja::verilog::GateType::Xor:
      return naja::NL::NLDB0::GateType::Xor;
    case naja::verilog::GateType::Xnor:
      return naja::NL::NLDB0::GateType::Xnor;
    case naja::verilog::GateType::Buf:
      return naja::NL::NLDB0::GateType::Buf;
    case naja::verilog::GateType::Not:
      return naja::NL::NLDB0::GateType::Not;
    //LCOV_EXCL_START
    default: {
      std::ostringstream reason;
      reason << "Unsupported verilog gate type: ";
      reason << gateType.getString();
      throw naja::NL::SNLVRLConstructorException(reason.str());
    }
    //LCOV_EXCL_STOP
  }
  return naja::NL::NLDB0::GateType::Unknown; //LCOV_EXCL_LINE
}

naja::NL::NLLibrary* getOrCreateAutoBlackBoxLibrary(naja::NL::NLLibrary* library) {
  const std::string AUTO_BLACKBOX = "AUTO_BLACKBOX";
  auto autoBlackBoxLibrary = library->getLibrary(naja::NL::NLName(AUTO_BLACKBOX));
  if (not autoBlackBoxLibrary) {
    autoBlackBoxLibrary = naja::NL::NLLibrary::create(library, naja::NL::NLName(AUTO_BLACKBOX));
  }
  return autoBlackBoxLibrary;
}

naja::NL::SNLDesign* getOrCreateAutoBlackBox(
  naja::NL::NLLibrary* autoBlackBoxLibrary,
  const naja::NL::NLName& modelName) {
  auto model = autoBlackBoxLibrary->getSNLDesign(modelName);
  if (not model) {
    model = naja::NL::SNLDesign::create(
      autoBlackBoxLibrary,
      naja::NL::SNLDesign::Type::AutoBlackBox,
      modelName
    );
  }
  return model;
}

}

namespace naja { namespace NL {

void SNLVRLConstructor::GateInstance::reset() {
  gateType_ = NLDB0::GateType::Unknown;
  instanceName_ = std::string();
  connections_.clear();
}

bool SNLVRLConstructor::GateInstance::isValid() const {
  return gateType_ != NLDB0::GateType::Unknown;
}

//LCOV_EXCL_START
std::string SNLVRLConstructor::GateInstance::getString() const {
  std::ostringstream stream;
  stream << "GateInstance: " << gateType_.getString();
  if (not instanceName_.empty()) {
    stream  << " " << instanceName_;
  } else {
    stream << " <anonymous>";
  }
  return stream.str();
}
//LCOV_EXCL_STOP

SNLTerm::Direction
SNLVRLConstructor::VRLDirectionToSNLDirection(const naja::verilog::Port::Direction& direction) {
  switch (direction) {
    case naja::verilog::Port::Direction::Input:
      return naja::NL::SNLTerm::Direction::Input;
    case naja::verilog::Port::Direction::Output:
      return naja::NL::SNLTerm::Direction::Output;
    case naja::verilog::Port::Direction::InOut:
      return naja::NL::SNLTerm::Direction::InOut;
    case naja::verilog::Port::Direction::Unknown: {
      std::ostringstream reason;
      reason << "Unsupported verilog direction";
      throw naja::NL::SNLVRLConstructorException(reason.str());
    }
  }
  return naja::NL::SNLTerm::Direction::Input; //LCOV_EXCL_LINE
}

SNLNet::Type
SNLVRLConstructor::VRLTypeToSNLType(const naja::verilog::Net::Type& type) {
  switch(type) {
    case naja::verilog::Net::Type::Wire:
      return naja::NL::SNLNet::Type::Standard;
    case naja::verilog::Net::Type::Supply0:
      return naja::NL::SNLNet::Type::Supply0;
    case naja::verilog::Net::Type::Supply1:
      return naja::NL::SNLNet::Type::Supply1;
    case naja::verilog::Net::Type::Unknown: {
      std::ostringstream reason;
      reason << "Unsupported verilog net type";
      throw naja::NL::SNLVRLConstructorException(reason.str());
    }
  }
  return naja::NL::SNLNet::Type::Standard; //LCOV_EXCL_LINE
}

void SNLVRLConstructor::createCurrentModuleAssignNets() {
  currentModuleAssign0_ = naja::NL::SNLScalarNet::create(currentModule_);
  currentModuleAssign0_->setType(naja::NL::SNLNet::Type::Assign0);
  currentModuleAssign1_ = naja::NL::SNLScalarNet::create(currentModule_);
  currentModuleAssign1_->setType(naja::NL::SNLNet::Type::Assign1);
} 

void SNLVRLConstructor::createConstantNets(
  const naja::verilog::Number& number,
  naja::NL::SNLInstance::Nets& nets) {
  if (number.value_.index() != naja::verilog::Number::BASED) {
    std::ostringstream reason;
    reason << getLocationString();
    reason << ": Only base numbers are supported"; 
    throw naja::NL::SNLVRLConstructorException(reason.str());
  }
  auto basedNumber = std::get<naja::verilog::Number::BASED>(number.value_);
  auto bits = SNLVRLConstructorUtils::numberToBits(basedNumber);
  if (bits.size() != basedNumber.size_) {
    std::ostringstream reason;
    reason << getLocationString();
    reason << ": " << "Size";
    throw naja::NL::SNLVRLConstructorException(reason.str());
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

SNLVRLConstructor::SNLVRLConstructor(NLLibrary* library):
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
    currentModule_ = SNLDesign::create(library_, NLName(module.name_));
    collectAttributes(currentModule_, nextObjectAttributes_);
    if (config_.verbose_) {
      std::cerr << "Construct Module: " << module.getString() << std::endl; //LCOV_EXCL_LINE
    }
  } else {
    currentModule_ = library_->getSNLDesign(NLName(module.name_));
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
  nextObjectAttributes_.clear();
}

void SNLVRLConstructor::moduleInterfaceSimplePort(const naja::verilog::Identifier& port) {
  if (inFirstPass()) {
    if (config_.verbose_) {
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
    if (config_.verbose_) {
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
  nextObjectAttributes_.clear();
}

void SNLVRLConstructor::moduleInterfaceCompletePort(const naja::verilog::Port& port) {
  if (inFirstPass()) {
    createPort(currentModule_, port, nextObjectAttributes_);
  } else {
    createPortNet(currentModule_, port);
  }
  nextObjectAttributes_.clear();
}

void SNLVRLConstructor::addNet(const naja::verilog::Net& net) {
  if (not inFirstPass()) {
    if (config_.verbose_) {
      std::cerr << "Add net: " << net.getString() << std::endl; //LCOV_EXCL_LINE
    }
    auto netName = NLName(net.identifier_.name_);
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
        snlNet = SNLBusNet::create(currentModule_, net.range_.msb_, net.range_.lsb_, NLName(net.identifier_.name_));
      } else {
        snlNet = SNLScalarNet::create(currentModule_, NLName(net.identifier_.name_));
      }
      snlNet->setType(VRLTypeToSNLType(net.type_));
      collectAttributes(snlNet, nextObjectAttributes_);
    //LCOV_EXCL_START
    } catch (const NLException& exception) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": " << exception.getReason();
      throw SNLVRLConstructorException(reason.str());
    }
    //LCOV_EXCL_STOP
  }
  nextObjectAttributes_.clear();
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
  if (config_.verbose_) {
    std::cerr << "Assign: " << std::endl; //LCOV_EXCL_LINE
  }
  for (auto id: identifiers) {
    collectIdentifierNets(id, leftNets);
    if (config_.verbose_) {
      std::cerr << "ID: " << id.getString() << std::endl; //LCOV_EXCL_LINE
    }
  }
  if (config_.verbose_) {
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
        SNLNet* net = currentModule_->getNet(NLName(name));
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
    auto assign = SNLInstance::create(currentModule_, NLDB0::getAssign());
    assign->setTermNet(NLDB0::getAssignOutput(), leftNet);
    assign->setTermNet(NLDB0::getAssignInput(), rightNet);
  }
}

void SNLVRLConstructor::startInstantiation(const naja::verilog::Identifier& model) {
  if (not inFirstPass()) {
    currentModelName_ = model.name_;
    if (config_.verbose_) {
      std::cerr << "Start Instantiation: " << model.getString() << std::endl; //LCOV_EXCL_LINE
    }
  }
}

void SNLVRLConstructor::addInstance(const naja::verilog::Identifier& instance) {
  if (not inFirstPass()) {
    assert(not currentModelName_.empty());
    NLName modelName(currentModelName_);
    SNLDesign* model = library_->getSNLDesign(modelName);
    if (not model) {
      model = library_->getDB()->getSNLDesign(modelName);
    }
    if (not model) {
      model = NLUniverse::get()->getSNLDesign(modelName);
    }
    if (not model) {
      if (config_.allowUnknownDesigns_) {
        if (config_.verbose_) {
          std::cerr << "Unknown design: " << modelName.getString() << std::endl; //LCOV_EXCL_LINE
        }
        auto autoBlackBoxLibrary = getOrCreateAutoBlackBoxLibrary(library_);
        model = getOrCreateAutoBlackBox(autoBlackBoxLibrary, modelName);
      } else {
        std::ostringstream reason;
        reason << getLocationString();
        reason << ": " << currentModelName_
          << " cannot be found in SNL while constructing instance "
          << instance.getString();
        throw SNLVRLConstructorException(reason.str());
      }
    }
    //FIXME
    //might be a good idea to create a cache <Name, SNLDesign*> here
    //in particular for primitives
    currentInstance_ = SNLInstance::create(currentModule_, model, NLName(instance.name_));
    collectAttributes(currentInstance_, nextObjectAttributes_);
  }
  nextObjectAttributes_.clear();
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
    if (config_.verbose_) {
      //LCOV_EXCL_START
      std::cerr << "End " << currentInstance_->getString() 
        << " instantiation" << std::endl;
      //LCOV_EXCL_STOP
    }
    //Save current Parameter assignments
    for (auto parameterValue: currentInstanceParameterValues_) {
      auto parameterName = NLName(parameterValue.first);
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
  naja::NL::SNLTerm* term,
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
        SNLNet* net = currentInstance_->getDesign()->getNet(NLName(name));
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
            reason << getLocationString()
              << ": net \"" << name << "\" is not a bus net (expected SNLBusNet) for range access";
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
            reason << getLocationString()
               << ": term \"" << (term ? term->getString() : "<null>")
               << "\" is not a bus term (expected SNLBusTerm) for concatenation connection";
          throw SNLVRLConstructorException(reason.str());
        }
        BitTerms bitTerms(busTerm->getBits().begin(), busTerm->getBits().end());
        if (bitTerms.size() != bitNets.size()) {
          std::ostringstream reason;
          reason << getLocationString();
          reason << ": " << term->getString() << " and " << expression.getString();
          reason << " do not have the same number of bits";
          throw SNLVRLConstructorException(reason.str());
        }
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
    SNLTerm* term = model->getTerm(NLName(port.name_));
    if (not term) {
      if (model->isAutoBlackBox()) {
        if (expression.getSize() > 1) {
          term = SNLBusTerm::create(
            model,
            SNLTerm::Direction::Undefined,
            expression.getSize()-1,
            0,
            NLName(port.name_));
        } else {
        term = SNLScalarTerm::create(
          model,
          SNLTerm::Direction::Undefined,
          NLName(port.name_));
        }
      } else {
        std::ostringstream reason;
        reason << getLocationString();
        reason << ": " << port.getString()
          << " port cannot be found in " << model->getName().getString()
          << " model";
        throw SNLVRLConstructorException(reason.str());
      }
    }
    currentInstancePortConnection(term, expression);
    if (config_.verbose_) {
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
    SNLTerm* term = model->getTerm(NLID::DesignObjectID(portIndex));
    currentInstancePortConnection(term, expression);
    if (config_.verbose_) {
      //LCOV_EXCL_START
      std::cerr << "Instance connection: "
        << currentInstance_->getString()
        << " - " << term->getString() << " connection"
        << std::endl;
      //LCOV_EXCL_STOP
    }
  }
}

void SNLVRLConstructor::startGateInstantiation(const naja::verilog::GateType& gateType) {
  if (not inFirstPass()) {
    currentGateInstance_.gateType_ = najaVerilogToNLDB0GateType(gateType);
    if (config_.verbose_) {
      std::cerr << "Start Instantiation: " << gateType.getString() << std::endl; //LCOV_EXCL_LINE
    }
  }
}

void SNLVRLConstructor::addGateInstance(const naja::verilog::Identifier& id) {
  if (not inFirstPass() and not id.empty()) {
    currentGateInstance_.instanceName_ = id.name_;
  }
}

void SNLVRLConstructor::addGateOutputInstanceConnection(
  size_t portIndex,
  const naja::verilog::RangeIdentifiers identifiers) {
  if (inFirstPass()) {
    return;
  }
  if (identifiers.size() != 1) {
    //LCOV_EXCL_START
    std::ostringstream reason;
    reason << getLocationString();
    reason << ": " << naja::verilog::getRangeIdentifiersString(identifiers) << " is not supported";
    throw SNLVRLConstructorException(reason.str());
    //LCOV_EXCL_STOP
  }
  auto identifier = identifiers[0];
  naja::verilog::Expression expression;
  expression.value_ = identifier;
  expression.valid_ = true;
  expression.supported_ = true;
  currentGateInstance_.connections_.push_back(expression);
}

void SNLVRLConstructor::addGateInputInstanceConnection(
  size_t portIndex,
  const naja::verilog::Expression& expression) {
  if (inFirstPass()) {
    return;
  }
  currentGateInstance_.connections_.push_back(expression);
}

void SNLVRLConstructor::endGateInstantiation() {
  if (not inFirstPass()) {
    assert(currentGateInstance_.isValid());
    if (config_.verbose_) {
      //LCOV_EXCL_START
      std::cerr << "End " << currentGateInstance_.getString() 
        << " instantiation" << std::endl;
      //LCOV_EXCL_STOP
    }
    //create gate instance
    SNLDesign* gate = nullptr;
    size_t nbTerms = currentGateInstance_.connections_.size();
    if (currentGateInstance_.gateType_.isNInput()) {
      gate = NLDB0::getOrCreateNInputGate(currentGateInstance_.gateType_, nbTerms-1);
    } else {
      gate = NLDB0::getOrCreateNOutputGate(currentGateInstance_.gateType_, nbTerms-1);
    }
    currentInstance_ = SNLInstance::create(
      currentModule_,
      gate,
      NLName(currentGateInstance_.instanceName_));

    using Terms = std::vector<SNLBitTerm*>;
    Terms terms;
    terms.push_back(NLDB0::getGateSingleTerm(gate));
    auto nterm = NLDB0::getGateNTerms(gate);
    terms.insert(terms.end(), nterm->getBits().begin(), nterm->getBits().end());

    for (size_t i=0; i<currentGateInstance_.connections_.size(); ++i) {
      currentInstancePortConnection(
        terms[i],
        currentGateInstance_.connections_[i]);
    }
    currentInstance_ = nullptr;
    currentGateInstance_.reset();
  }
}

void SNLVRLConstructor::addAttribute(
  const naja::verilog::Identifier& attributeName,
  const naja::verilog::ConstantExpression& expression) {
  if (getParseAttributes()) {
    nextObjectAttributes_.push_back(naja::verilog::Attribute(attributeName, expression));
  }
}

void SNLVRLConstructor::endModule() {
  if (config_.verbose_) {
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
    for (const auto& port: currentModuleInterfacePorts_) {
      createPort(currentModule_, *port, nextObjectAttributes_);
    }
  } else {
    //Blackbox detection
    if (config_.blackboxDetection_ and currentModule_->getType() == SNLDesign::Type::Standard) {
      if (currentModule_->getInstances().empty()) {
        if (allNetsArePortNets(currentModule_)) {
          currentModule_->setType(SNLDesign::Type::UserBlackBox);
          //spdlog::info("Blackbox detected: {}", currentModule_->getName().getString());
        }
      }
    }
    //Allow unknown designs
    if (config_.allowUnknownDesigns_ and currentModule_->isStandard()) {
      for (auto instance: currentModule_->getInstances()) {
        if (instance->isAutoBlackBox()) {
          //spdlog::info("Auto blackbox detected: {}", instance->getDesign()->getName().getString());
          auto model = instance->getModel();
          for (auto term: model->getTerms()) {
            if (term->getDirection() == SNLTerm::Direction::Undefined) {
              if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
                auto instanceTerm = instance->getInstTerm(scalarTerm);
                if (instanceTerm) {
                  auto net = instanceTerm->getNet();
                  if (net->isConstant()) {
                    scalarTerm->setDirection(SNLTerm::Direction::Input);
                  }
                }
              }
              //else if (auto busTerm = dynamic_cast<SNLBusTerm*>(term)) {
              //  busTerm->setDirection(SNLTerm::Direction::InOut);
              //} else {
              //  std::ostringstream reason;
              //  reason << getLocationString();
              //  reason << ": " << term->getString() << " is not a bus or scalar term";
              //  throw SNLVRLConstructorException(reason.str());
              //}
            }
          }
        }
      }
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
      reason << naja::NL::SNLVRLConstructor::getLocationString();
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
        SNLNet* net = currentModule_->getNet(NLName(name));
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
  SNLNet* net = currentModule_->getNet(NLName(name));
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

void SNLVRLConstructor::addDefParameterAssignment(
  const naja::verilog::Identifiers& hierarchicalParameter,
  const naja::verilog::ConstantExpression& expression) {
  if (not inFirstPass()) {
    if (hierarchicalParameter.size() != 2) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": Only Hierarchical parameter of size 2 assignment are supported";
      throw SNLVRLConstructorException(reason.str());
    }
    auto instanceName = hierarchicalParameter[0];
    auto parameterName = hierarchicalParameter[1];
    auto instance = currentModule_->getInstance(NLName(instanceName.name_));
    if (not instance) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": instance " << instanceName.getString();
      reason << " cannot be found in " << currentModule_->getName().getString();
      throw SNLVRLConstructorException(reason.str());
    }
    auto parameter = instance->getModel()->getParameter(NLName(parameterName.name_));
    if (not parameter) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": parameter " << parameterName.getString();
      reason << " cannot be found in " << instance->getModel()->getName().getString();
      throw SNLVRLConstructorException(reason.str());
    }
    SNLInstParameter::create(instance, parameter, expression.getString());
  }
}

}} // namespace NL // namespace naja
