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

#include <boost/dynamic_bitset.hpp>

#include "SNLUniverse.h"
#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLBusTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusTermBit.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLScalarNet.h"
#include "SNLVRLConstructorException.h"

namespace {

naja::SNL::SNLTerm::Direction VRLDirectionToSNLDirection(const naja::verilog::Port::Direction& direction) {
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

naja::SNL::SNLNet::Type VRLTypeToSNLType(const naja::verilog::Net::Type& type) {
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

void createPort(naja::SNL::SNLDesign* design, const naja::verilog::Port& port) {
  if (port.isBus()) {
    naja::SNL::SNLBusTerm::create(
      design,
      VRLDirectionToSNLDirection(port.direction_),
      port.range_.msb_,
      port.range_.lsb_,
      naja::SNL::SNLName(port.name_));
  } else {
    naja::SNL::SNLScalarTerm::create(design,
      VRLDirectionToSNLDirection(port.direction_),
      naja::SNL::SNLName(port.name_));
  }
}

void setBit(boost::dynamic_bitset<>& bits, size_t i) {
  if (i<bits.size()) {
    bits.set(i);
  }
}

void reverse(boost::dynamic_bitset<> &bs) {
  for (size_t begin = 0, end = bs.size() - 1; begin < end; begin++, end--) {
    bool b = bs[end];
    bs[end] = bs[begin];
    bs[begin] = b;
  }
}

boost::dynamic_bitset<> numberToBits(const naja::verilog::BasedNumber& number) {
  switch (number.base_) {
    case naja::verilog::BasedNumber::DECIMAL: {
      unsigned long value = std::stoul(number.digits_);
      auto bits = boost::dynamic_bitset<>(number.size_, value);
      ::reverse(bits);
      return bits;
    }
    case naja::verilog::BasedNumber::HEX: {
      boost::dynamic_bitset<> bits(number.size_, 0ul);
      size_t i = bits.size()-1; 
      for (int j = number.digits_.size()-1; j>=0; j--) {
        const char& c = number.digits_[j];
        switch (toupper(c)) {
          case '0': break;
          case '1': setBit(bits, i); break;
          case '2': setBit(bits, i-1); break;
          case '3': setBit(bits, i); setBit(bits, i-1); break;
          case '4': setBit(bits, i-2); break;
          case '5': setBit(bits, i-2); setBit(bits, i); break;
          case '6': setBit(bits, i-2); setBit(bits, i-1); break;
          case '7': setBit(bits, i-2); setBit(bits, i-1); setBit(bits, i); break;
          case '8': setBit(bits, i-3); break;
          case '9': setBit(bits, i-3); setBit(bits, i); break;
          case 'A': setBit(bits, i-3); setBit(bits, i-1); break;
          case 'B': setBit(bits, i-3); setBit(bits, i-1); setBit(bits, i); break;
          case 'C': setBit(bits, i-3); setBit(bits, i-2); break;
          case 'D': setBit(bits, i-3); setBit(bits, i-2); setBit(bits, i); break;
          case 'E': setBit(bits, i-3); setBit(bits, i-2); setBit(bits, i-1); break;
          case 'F': setBit(bits, i-3); setBit(bits, i-2); setBit(bits, i-1); setBit(bits, i); break;
        }
        if (i>0) {
          i -= 4;
        }
      }
      return bits;
    }
    default:
      throw naja::SNL::SNLVRLConstructorException("Error");
  }
  return boost::dynamic_bitset<>();
}

}

namespace naja { namespace SNL {

SNLScalarNet* SNLVRLConstructor::getOrCreateCurrentModelAssignNet(naja::SNL::SNLNet::Type type) {
  auto design = currentInstance_->getDesign();
  switch (type) {
    case naja::SNL::SNLNet::Type::Assign0:
      if (not currentModelAssign0_) {
        currentModelAssign0_ =
          naja::SNL::SNLScalarNet::create(design);
        currentModelAssign0_->setType(naja::SNL::SNLNet::Type::Assign0);
      }
      return currentModelAssign0_;
      break;
    case naja::SNL::SNLNet::Type::Assign1:
      if (not currentModelAssign1_) {
        currentModelAssign1_ =
          naja::SNL::SNLScalarNet::create(design);
        currentModelAssign1_->setType(naja::SNL::SNLNet::Type::Assign1);
      }
      return currentModelAssign1_;
      break;
    default:
      throw SNLVRLConstructorException("ASSIGN issue");
  }
  return nullptr;
}

void SNLVRLConstructor::createConstantNets(
  const naja::verilog::Number& number,
  naja::SNL::SNLInstance::Nets& nets) {
  if (number.value_.index() != naja::verilog::Number::BASED) {
    std::ostringstream reason;
    reason << "Only base numbers are supported"; 
    throw naja::SNL::SNLVRLConstructorException(reason.str());
  }
  auto basedNumber = std::get<naja::verilog::Number::BASED>(number.value_);
  auto bits = numberToBits(basedNumber);
  if (bits.size() != basedNumber.size_) {
    throw naja::SNL::SNLVRLConstructorException("Size");
  }
  for (int i=0; i<bits.size(); i++) {
    SNLScalarNet* assignNet = nullptr;
    if (bits[i]) {
      assignNet = getOrCreateCurrentModelAssignNet(naja::SNL::SNLNet::Type::Assign1);
    } else {
      assignNet = getOrCreateCurrentModelAssignNet(naja::SNL::SNLNet::Type::Assign0);
    }
    nets.push_back(assignNet);
  }
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
      std::cerr << "Cannot find Module: " << name << std::endl;
      exit(5);
    }
  } 
}

void SNLVRLConstructor::moduleInterfaceSimplePort(const std::string& name) {
  if (inFirstPass()) {
    if (verbose_) {
      std::cerr << "Add port: " << name << std::endl;
    }
    //FIXME test port name collision
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
      //error
    }
    createPort(currentModule_, port);
  }
}

void SNLVRLConstructor::moduleInterfaceCompletePort(const naja::verilog::Port& port) {
  if (inFirstPass()) {
    if (verbose_) {
      std::cerr << "Add port: " << port.getString() << std::endl;
    }
    createPort(currentModule_, port);
  }
}

void SNLVRLConstructor::addNet(const naja::verilog::Net& net) {
  if (not inFirstPass()) {
    if (verbose_) {
      std::cerr << "Add net: " << net.getString() << std::endl;
    }
    SNLNet* snlNet = nullptr;
    if (net.isBus()) {
      snlNet = SNLBusNet::create(currentModule_, net.range_.msb_, net.range_.lsb_, SNLName(net.name_));
    } else {
      snlNet = SNLScalarNet::create(currentModule_, SNLName(net.name_));
    }
    snlNet->setType(VRLTypeToSNLType(net.type_));
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
      reason << currentModelName_
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
      currentInstance_->addParameterValue(SNLName(parameterValue.first), parameterValue.second);
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
      reason << portName
        << " port cannot be found in " << model->getName().getString()
        << " model";
      throw SNLVRLConstructorException(reason.str());
    }
    if (expression.valid_) {
      if (not expression.supported_) {
        std::ostringstream reason;
        reason << expression.getString() << " is not currently supported";
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
            reason << name
              << " net cannot be found in " << model->getName().getString()
              << " model";
            throw SNLVRLConstructorException(reason.str());
          }
          if (identifier.range_.valid_) {
            SNLBusNet* busNet = dynamic_cast<SNLBusNet*>(net);
            if (not busNet) {
              throw SNLVRLConstructorException("NOT BUSTERM");
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
          for (auto expression: concatenation.expressions_) {
            if (not expression.supported_ or not expression.valid_) {
              std::ostringstream reason;
              reason << expression.getString() << " is not supported";
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
                SNLNet* net = currentInstance_->getDesign()->getNet(SNLName(name));
                if (not net) {
                  std::ostringstream reason;
                  reason << name
                    << " net cannot be found in " << model->getName().getString()
                    << " model";
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
          using BitTerms = std::vector<SNLBitTerm*>;
          auto busTerm = dynamic_cast<SNLBusTerm*>(term);
          if (not busTerm) {
            throw SNLVRLConstructorException("NOT BUSTERM");
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
  currentModelAssign0_ = nullptr;
  currentModelAssign1_ = nullptr;
}

}} // namespace SNL // namespace naja