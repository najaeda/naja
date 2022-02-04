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

#include "SNLVRLDumper.h"

#include <cassert>

#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLBusTerm.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"

namespace {

void dumpDirection(const SNL::SNLTerm* term, std::ostream& o) {
  switch (term->getDirection()) {
    case SNL::SNLTerm::Direction::Input:
      o << "input";
      break;
    case SNL::SNLTerm::Direction::Output:
      o << "output";
      break;
    case SNL::SNLTerm::Direction::InOut:
      o << "inout";
      break;
  }
}

}

namespace SNL {

std::string SNLVRLDumper::createDesignName(const SNLDesign* design) {
  auto library = design->getLibrary();
  auto designID = design->getID();
  std::string designName = "module" + std::to_string(designID);
  int conflict = 0;
  while (library->getDesign(designName)) {
    designName += "_" + std::to_string(conflict++); 
  }
  return designName;
}

std::string SNLVRLDumper::createInstanceName(const SNLInstance* instance) {
  auto design = instance->getDesign();
  auto instanceID = instance->getID();
  std::string instanceName = "inst" + std::to_string(instanceID);
  int conflict = 0;
  while (design->getInstance(instanceName)) {
    instanceName += "_" + std::to_string(conflict++); 
  }
  return instanceName;
}

void SNLVRLDumper::dumpInterface(const SNLDesign* design, std::ostream& o) {
  o << "(";
  bool first = true;
  for (auto term: design->getTerms()) {
    if (not first) {
      o << ", ";
    } else {
      first = false;
    }
    dumpDirection(term, o);
    o << " ";
    if (auto bus = dynamic_cast<SNLBusTerm*>(term)) {
      o << "[" << bus->getMSB() << ":" << bus->getLSB() << "] ";
    }
    o << term->getName().getString();
  }
  o << ");";
}

void SNLVRLDumper::dumpNets(const SNLDesign* design, std::ostream& o) {
  for (auto net: design->getNets()) {
    o << "wire ";
    if (auto bus = dynamic_cast<SNLBusNet*>(net)) {
      o << "[" << bus->getMSB() << ":" << bus->getLSB() << "] ";
    }
    o << net->getName().getString();
    o << ";" << std::endl;
  }
  o << std::endl;
}

void SNLVRLDumper::dumpRange(const ContiguousNetBits& bits, std::ostream& o) {

}

void SNLVRLDumper::dumpInsTermConnectivity(const SNLTerm* term, BitNetVector& termNets, std::ostream& o) {
  if (std::all_of(termNets.begin(), termNets.end(), [](const SNLBitNet* n){ return n != nullptr; })) {
   assert(not termNets.empty());
   o << "." << term->getName().getString() << "(";
   ContiguousNetBits contiguousBits;
   for (auto net: termNets) {
     if (net) {
       if (dynamic_cast<SNLScalarNet*>(net)) {
         dumpRange(contiguousBits, o);
         contiguousBits.clear();
         o << net->getName().getString();
       } else {
         auto busNetBit = static_cast<SNLBusNetBit*>(net);
         auto busNet = busNetBit->getBus();
         if (not contiguousBits.empty()) {
           SNLBusNetBit* previousBit = contiguousBits.back();
           if (busNet == previousBit->getBus()
           and ((previousBit->getBit() == busNetBit->getBit()+1)
           or (previousBit->getBit() == busNetBit->getBit()-1))) {
             contiguousBits.push_back(busNetBit);
           } else {

           }
         }
       }
     }
   }
   o << ")";
  }
}

void SNLVRLDumper::dumpInstanceInterface(const SNLInstance* instance, std::ostream& o) {
  o << "(";
  BitNetVector termNets;
  SNLTerm* previousTerm = nullptr;
  for (auto instTerm: instance->getInstTerms()) {
    SNLTerm* currentTerm = nullptr;
    auto bitTerm = instTerm->getTerm();
    if (dynamic_cast<SNLScalarTerm*>(bitTerm)) {
      currentTerm = bitTerm;
    } else {
      auto busTermBit = static_cast<SNLBusTermBit*>(bitTerm);
      currentTerm = busTermBit->getBus();
    }
    if (currentTerm == previousTerm) {
      termNets.push_back(instTerm->getNet());
    } else {
      if (previousTerm) {
        dumpInsTermConnectivity(previousTerm, termNets, o);
      }
      termNets = { instTerm->getNet() };
    }
    previousTerm = currentTerm;
  }
  if (previousTerm) {
    dumpInsTermConnectivity(previousTerm, termNets, o);
  }
  o << ")";
}

void SNLVRLDumper::dumpInstance(const SNLInstance* instance, std::ostream& o) {
  SNLName instanceName;
  if (instance->isAnonymous()) {
    instanceName = createInstanceName(instance);
  } else {
    instanceName = instance->getName();
  }
  auto model = instance->getModel();
  if (model->isAnonymous()) {
    o << instanceName.getString() << std::endl;
  } else {
    o << model->getName().getString() << " " << instanceName.getString();
  }
  dumpInstanceInterface(instance, o);
  o << ";" << std::endl;
}

void SNLVRLDumper::dumpInstances(const SNLDesign* design, std::ostream& o) {
  for (auto instance: design->getInstances()) {
    dumpInstance(instance, o);
  }
}

void SNLVRLDumper::dumpDesign(const SNLDesign* design, std::ostream& o) {
  if (design->isAnonymous()) {
    createDesignName(design);
  }
  o << "module " << design->getName().getString();

  dumpInterface(design, o);

  o << std::endl;

  dumpNets(design, o);

  dumpInstances(design, o);

  o << "endmodule //" << design->getName().getString();
  o << std::endl;
}

}
