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

#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLBusTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusNet.h"
#include "SNLScalarNet.h"

namespace {

naja::SNL::SNLTerm::Direction VRLDirectionToSNLDirection(const naja::verilog::Port::Direction& direction) {
  switch (direction) {
    case naja::verilog::Port::Direction::Input:
      return naja::SNL::SNLTerm::Direction::Input;
    case naja::verilog::Port::Direction::Output:
      return naja::SNL::SNLTerm::Direction::Output;
    case naja::verilog::Port::Direction::InOut:
      return naja::SNL::SNLTerm::Direction::InOut;
    default:
      std::exit(-43);
  }
}

}

namespace naja { namespace SNL {

SNLVRLConstructor::SNLVRLConstructor(SNLLibrary* library):
  library_(library)
{}

void SNLVRLConstructor::startModule(std::string&& name) {
  if (inFirstPass()) {
    currentModule_ = SNLDesign::create(library_, SNLName(name));
    std::cerr << "Construct Module: " << name << std::endl;
  } else {
    currentModule_ = library_->getDesign(SNLName(name));
    if (not currentModule_) {
      std::cerr << "Cannot find Module: " << name << std::endl;
      exit(5);
    }
  } 
}

void SNLVRLConstructor::moduleInterfaceCompletePort(naja::verilog::Port&& port) {
  if (inFirstPass()) {
    if (port.isBus()) {
      SNLBusTerm::create(
        currentModule_,
        VRLDirectionToSNLDirection(port.direction_),
        port.range_.msb_,
        port.range_.lsb_,
        SNLName(port.name_));
    } else {
      SNLScalarTerm::create(currentModule_,
        VRLDirectionToSNLDirection(port.direction_),
        SNLName(port.name_));
    }
  }
}

void SNLVRLConstructor::addNet(naja::verilog::Net&& net) {
  if (not inFirstPass()) {
    std::cerr << "Add net: " << net.getString() << std::endl;
    if (net.isBus()) {
      SNLBusNet::create(currentModule_, net.range_.msb_, net.range_.lsb_, SNLName(net.name_));
    } else {
      SNLScalarNet::create(currentModule_, SNLName(net.name_));
    }
  }
}

void SNLVRLConstructor::addInstance(std::string&& name) {

  
}

}} // namespace SNL // namespace naja
