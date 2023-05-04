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
      naja::SNL::SNLName(port.name_));
  } else {
    naja::SNL::SNLScalarTerm::create(
      design,
      naja::SNL::SNLVRLConstructor::VRLDirectionToSNLDirection(port.direction_),
      naja::SNL::SNLName(port.name_));
  }
}

void createPortNet(naja::SNL::SNLDesign* design, const naja::verilog::Port& port) {
  if (port.isBus()) {
    auto term = design->getBusTerm(naja::SNL::SNLName(port.name_));
    auto net = naja::SNL::SNLBusNet::create(
      design,
      port.range_.msb_,
      port.range_.lsb_,
      naja::SNL::SNLName(port.name_));
    term->setNet(net);
  } else {
    auto term = design->getScalarTerm(naja::SNL::SNLName(port.name_));
    assert(term);
    auto net = naja::SNL::SNLScalarNet::create(design,
      naja::SNL::SNLName(port.name_));
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

void SNLVRLConstructor::construct(const std::filesystem::path& filePath) {
  setFirstPass(true);
  parse(filePath);
  setFirstPass(false);
  parse(filePath);
}

void SNLVRLConstructor::startModule(const std::string& name) {
  if (inFirstPass()) {
    currentModule_ = SNLDesign::create(library_, SNLName(name));
    if (verbose_) {
      std::cerr << "Construct Module: " << name << std::endl;
    }
  } else {
    currentModule_ = library_->getDesign(SNLName(name));
    if (not currentModule_) {
      std::ostringstream reason;
      reason << "In SNLVRLConstructor second pass, ";
      reason << name << " module cannot be found in library: ";
      reason << library_->getDescription();
      throw SNLVRLConstructorException(reason.str());
    }
    if (not currentModule_->getNets().empty()) {
      std::ostringstream reason;
      reason << "In SNLVRLConstructor second pass, ";
      reason << name << " module should no contain any net";
      throw SNLVRLConstructorException(reason.str());
    }
    createCurrentModuleAssignNets();
  } 
}

void SNLVRLConstructor::moduleInterfaceSimplePort(const std::string& name) {
  if (inFirstPass()) {
    if (verbose_) {
      std::cerr << "Add port: " << name << std::endl;
    }
    if (currentModuleInterfacePorts_.find(name) != currentModuleInterfacePorts_.end()) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": port collision in module " << currentModule_->getName().getString();
      reason << ", " << name << " has already been declared.";
      throw SNLVRLConstructorException(reason.str());
    }
    currentModuleInterfacePorts_.insert(name);
  }
}

void SNLVRLConstructor::moduleImplementationPort(const naja::verilog::Port& port) {
  if (inFirstPass()) {
    if (verbose_) {
      std::cerr << "Add implementation port: " << port.getString() << std::endl;
    }
    if (auto it = currentModuleInterfacePorts_.find(port.name_);
      it == currentModuleInterfacePorts_.end()) {
        std::ostringstream reason;
        reason << getLocationString();
        reason << ": undeclared port in module " << currentModule_->getName().getString();
        reason << ", " << port.getString() << " is unknown in module interface.";
        throw SNLVRLConstructorException(reason.str());
    }
    createPort(currentModule_, port);
  } else {
    createPortNet(currentModule_, port);
  }
}

void SNLVRLConstructor::moduleInterfaceCompletePort(const naja::verilog::Port& port) {
  if (inFirstPass()) {
    if (verbose_) {
      std::cerr << "Add port: " << port.getString() << std::endl;
    }
    createPort(currentModule_, port);
  } else {
    createPortNet(currentModule_, port);
  }
}

void SNLVRLConstructor::addNet(const naja::verilog::Net& net) {
  if (not inFirstPass()) {
    if (verbose_) {
      std::cerr << "Add net: " << net.getString() << std::endl;
    }
    auto netName = SNLName(net.name_);
    auto existingNet = currentModule_->getNet(netName);
    if (existingNet) {
      //net might be existing because of a port declaration
      auto term = currentModule_->getTerm(netName);
      if (term) {
        return;
      } else {
        std::ostringstream reason;
        reason << getLocationString();
        reason << ": wire collision for net " << net.name_;
        throw SNLVRLConstructorException(reason.str());
      }
    }
    try {
      SNLNet* snlNet = nullptr;
      if (net.isBus()) {
        snlNet = SNLBusNet::create(currentModule_, net.range_.msb_, net.range_.lsb_, SNLName(net.name_));
      } else {
        snlNet = SNLScalarNet::create(currentModule_, SNLName(net.name_));
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
  const naja::verilog::Identifiers& identifiers,
  const naja::verilog::Expression& expression) {
  if (inFirstPass()) {
    return;
  }
  using BitNets = std::vector<SNLBitNet*>;
  BitNets leftNets;
  BitNets rightNets;
  std::cerr << "Assign: " << std::endl;
  for (auto id: identifiers) {
    collectIdentifierNets(id, leftNets);
    std::cerr << "ID: " << id.getString() << std::endl;
  }
  std::cerr << expression.getString() << std::endl;
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
      case naja::verilog::Expression::Type::IDENTIFIER: {
        auto identifier = std::get<naja::verilog::Identifier>(expression.value_);
        std::string name = identifier.name_;
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
  std::cerr << leftNets.size();
  std::cerr << rightNets.size();
  assert(leftNets.size() == rightNets.size());
  for (size_t i=0; i<leftNets.size(); ++i) {
    auto leftNet = leftNets[i];
    auto rightNet = rightNets[i];
    auto assign = SNLInstance::create(currentModule_, SNLDB0::getAssign());
    assign->setTermNet(SNLDB0::getAssignOutput(), leftNet);
    assign->setTermNet(SNLDB0::getAssignInput(), rightNet);
  }
}

void SNLVRLConstructor::startInstantiation(const std::string& modelName) {
  if (not inFirstPass()) {
    currentModelName_ = modelName;
    if (verbose_) {
      std::cerr << "Start Instantiation: " << modelName << std::endl;
    }
  }
}

void SNLVRLConstructor::addInstance(const std::string& name) {
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
        << name;
      throw SNLVRLConstructorException(reason.str());
    }
    //FIXME
    //might be a good idea to create a cache <Name, SNLDesign*> here
    //in particular for primitives
    currentInstance_ = SNLInstance::create(currentModule_, model, SNLName(name));
  }
}

void SNLVRLConstructor::addParameterAssignment(
  const std::string& parameterName,
  const naja::verilog::Expression& expression) {
  if (not inFirstPass()) {
    currentInstanceParameterValues_[parameterName] = expression.getString();
  }
}

void SNLVRLConstructor::endInstantiation() {
  if (not inFirstPass()) {
    assert(currentInstance_);
    if (verbose_) {
      std::cerr << "End " << currentInstance_->getString() << " instantiation" << std::endl;
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

void SNLVRLConstructor::addInstanceConnection(
  const std::string& portName,
  const naja::verilog::Expression& expression) {
  if (not inFirstPass()) {
    assert(currentInstance_);
    SNLDesign* model = currentInstance_->getModel();
    SNLTerm* term = model->getTerm(SNLName(portName));
    if (not term) {
      std::ostringstream reason;
      reason << getLocationString();
      reason << ": " << portName
        << " port cannot be found in " << model->getName().getString()
        << " model";
      throw SNLVRLConstructorException(reason.str());
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
        case naja::verilog::Expression::Type::IDENTIFIER: {
          auto identifier = std::get<naja::verilog::Identifier>(expression.value_);
          std::string name = identifier.name_;
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
    if (verbose_) {
      std::cerr << "Instance connection: "
        << currentInstance_->getString()
        << " - " << term->getString() << " connection"
        << std::endl;
    }
  }
}

void SNLVRLConstructor::endModule() {
  if (verbose_) {
    std::cerr << "End module: " << currentModule_->getString() << std::endl;
  }
  currentModule_ = nullptr;
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
      case naja::verilog::Expression::IDENTIFIER: {
        auto identifier =
          std::get<naja::verilog::Expression::Type::IDENTIFIER>(expression.value_);
        std::string name = identifier.name_;
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
  const naja::verilog::Identifier& identifier,
  SNLInstance::Nets& bitNets) {
  std::string name = identifier.name_;
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
