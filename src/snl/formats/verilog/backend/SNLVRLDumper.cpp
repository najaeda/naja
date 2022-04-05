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
#include <algorithm>
#include <sstream>
#include <fstream>

#include "SNLLibrary.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLInstTerm.h"
#include "SNLUtils.h"

namespace {

void dumpDirection(const naja::SNL::SNLTerm* term, std::ostream& o) {
  switch (term->getDirection()) {
    case naja::SNL::SNLTerm::Direction::Input:
      o << "input";
      break;
    case naja::SNL::SNLTerm::Direction::Output:
      o << "output";
      break;
    case naja::SNL::SNLTerm::Direction::InOut:
      o << "inout";
      break;
  }
}

}

namespace naja { namespace SNL {

void SNLVRLDumper::setSingleFile(bool mode) {
  configuration_.setSingleFile(mode);
}

void SNLVRLDumper::setTopFileName(const std::string& name) {
  configuration_.setTopFileName(name);
}

void SNLVRLDumper::setDumpHierarchy(bool mode) {
  configuration_.setDumpHierarchy(mode);
}

std::string SNLVRLDumper::createDesignName(const SNLDesign* design) {
  auto library = design->getLibrary();
  auto designID = design->getID();
  std::string designName = "module" + std::to_string(designID);
  int conflict = 0;
  while (library->getDesign(SNLName(designName))) {
    designName += "_" + std::to_string(conflict++); 
  }
  return designName;
}

std::string SNLVRLDumper::createInstanceName(const SNLInstance* instance, DesignInsideAnonymousNaming& naming) {
  auto design = instance->getDesign();
  auto instanceID = instance->getID();
  std::string instanceName = "instance_" + std::to_string(instanceID);
  int conflict = 0;
  std::string uniqueInstanceName(instanceName);
  while (design->getInstance(SNLName(uniqueInstanceName))
      && naming.instanceNameSet_.find(uniqueInstanceName) != naming.instanceNameSet_.end()) {
    uniqueInstanceName = instanceName + "_" + std::to_string(conflict++); 
  }
  naming.instanceNameSet_.insert(uniqueInstanceName);
  naming.instanceNames_[instance->getID()] = uniqueInstanceName;
  return uniqueInstanceName;
}

SNLName SNLVRLDumper::createNetName(const SNLNet* net, DesignInsideAnonymousNaming& naming) {
  auto design = net->getDesign();
  auto netID = net->getID();
  std::string netName = "net_" + std::to_string(netID);
  int conflict = 0;
  SNLName uniqueNetName(netName);
  while (naming.netTermNameSet_.find(SNLName(uniqueNetName)) != naming.netTermNameSet_.end()) {
    uniqueNetName = SNLName(netName + "_" + std::to_string(conflict++)); 
  }
  naming.netTermNameSet_.insert(uniqueNetName);
  naming.netNames_[net->getID()] = uniqueNetName;
  return uniqueNetName;
}

SNLName SNLVRLDumper::getNetName(const SNLNet* net, const DesignInsideAnonymousNaming& naming) {
  if (net->isAnonymous()) {
    auto it = naming.netNames_.find(net->getID());
    assert(it != naming.netNames_.end());
    return it->second;
  } else {
    return net->getName();
  }
}

void SNLVRLDumper::dumpInterface(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming) {
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

void SNLVRLDumper::dumpNet(const SNLNet* net, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  SNLName netName;
  if (net->isAnonymous()) {
    netName = createNetName(net, naming);
  } else {
    netName = net->getName();
  }
  o << "wire ";
  if (auto bus = dynamic_cast<const SNLBusNet*>(net)) {
    o << "[" << bus->getMSB() << ":" << bus->getLSB() << "] ";
  }
  o << netName.getString();
  o << ";" << std::endl;
}

void SNLVRLDumper::dumpNets(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  for (auto net: design->getNets()) {
    dumpNet(net, o, naming);
  }
  o << std::endl;
}

void SNLVRLDumper::dumpRange(const ContiguousNetBits& bits, std::ostream& o) {

}

void SNLVRLDumper::dumpInsTermConnectivity(
  const SNLTerm* term,
  BitNetVector& termNets,
  std::ostream& o,
  const DesignInsideAnonymousNaming& naming) {
  if (std::all_of(termNets.begin(), termNets.end(), [](const SNLBitNet* n){ return n != nullptr; })) {
    assert(not termNets.empty());
    o << "." << term->getName().getString() << "(";
    ContiguousNetBits contiguousBits;
    for (auto net: termNets) {
      if (net) {
        if (dynamic_cast<SNLScalarNet*>(net)) {
          dumpRange(contiguousBits, o);
          contiguousBits.clear();
          SNLName netName = getNetName(net, naming);
          o << netName.getString();
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
  } else {
    //should we not dump anything for non connected inst terms ?
    o << "." << term->getName().getString() << "()"; 
  }
}

void SNLVRLDumper::dumpInstanceInterface(
  const SNLInstance* instance,
  std::ostream& o,
  const DesignInsideAnonymousNaming& naming) {
  o << "(";
  BitNetVector termNets;
  SNLTerm* previousTerm = nullptr;
  bool first = true;
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
        if (first) {
          first = false;
        } else {
          o << ", ";
        }
        dumpInsTermConnectivity(previousTerm, termNets, o, naming);
      }
      termNets = { instTerm->getNet() };
    }
    previousTerm = currentTerm;
  }
  if (previousTerm) {
    if (first) {
      first = false;
    } else {
      o << ", ";
    }
    dumpInsTermConnectivity(previousTerm, termNets, o, naming);
  }
  o << ")";
}

void SNLVRLDumper::dumpInstance(const SNLInstance* instance, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  std::string instanceName;
  if (instance->isAnonymous()) {
    instanceName = createInstanceName(instance, naming);
  } else {
    instanceName = instance->getName().getString();
  }
  auto model = instance->getModel();
  if (model->isAnonymous()) {
    o << instanceName << std::endl;
  } else {
    o << model->getName().getString() << " " << instanceName;
  }
  dumpInstanceInterface(instance, o, naming);
  o << ";" << std::endl;
}

void SNLVRLDumper::dumpInstances(const SNLDesign* design, std::ostream& o, DesignInsideAnonymousNaming& naming) {
  bool first = true;
  for (auto instance: design->getInstances()) {
    if (not first) {
      o << std::endl;
    }
    first = false;
    dumpInstance(instance, o, naming);
  }
}

void SNLVRLDumper::dumpOneDesign(const SNLDesign* design, std::ostream& o) {
  DesignInsideAnonymousNaming naming;
  for (auto term: design->getTerms()) {
    if (not term->isAnonymous()) {
      naming.netTermNameSet_.insert(term->getName());
    }
  }
  for (auto net: design->getNets()) {
    if (not net->isAnonymous()) {
      naming.netTermNameSet_.insert(net->getName());
    }
  }
  if (design->isAnonymous()) {
    createDesignName(design);
  }
  o << "module " << design->getName().getString();

  dumpInterface(design, o, naming);

  o << std::endl;

  dumpNets(design, o, naming);

  dumpInstances(design, o, naming);

  o << "endmodule //" << design->getName().getString();
  o << std::endl;
}

void SNLVRLDumper::dumpDesign(const SNLDesign* design, std::ostream& o) {
  if (configuration_.isDumpHierarchy()) {
    SNLUtils::SortedDesigns designs;
    SNLUtils::getDesignsSortedByHierarchicalLevel(design, designs);
    bool first = true;
    for (auto designLevel: designs) {
      const SNLDesign* design = designLevel.first;
      if (not first) {
        o << std::endl;
      }
      first = false;
      dumpOneDesign(design, o);
    }
  } else {
    dumpOneDesign(design, o);
  }
}

std::string SNLVRLDumper::getTopFileName(const SNLDesign* top) const {
  if (configuration_.hasTopFileName()) {
    return configuration_.getTopFileName() + ".v";
  }
  if (not top->isAnonymous()) {
    return top->getName().getString() + ".v";
  }
  return "top.v";
} 

void SNLVRLDumper::dumpDesign(const SNLDesign* design, const std::filesystem::path& path) {
  if (not std::filesystem::exists(path)) {
    std::ostringstream reason;
    if (not design->isAnonymous()) {
      reason << design->getName().getString();
    } else {
      reason << "anonymous design";
    }
    reason << " cannot be dumped: ";
    reason << path.string() << " " << " does not exist";
    throw SNLVRLDumperException(reason.str());
  }
  if (configuration_.isSingleFile()) {
    //create file
    std::filesystem::path filePath = path/getTopFileName(design);
    std::ofstream outFile;
    outFile.open(filePath);
    dumpDesign(design, outFile);
  } else {
    SNLVRLDumper streamDumper;
    SNLVRLDumper::Configuration configuration(configuration_);
    configuration.setDumpHierarchy(false);
    streamDumper.setConfiguration(configuration);
    SNLUtils::SortedDesigns designs;
    SNLUtils::getDesignsSortedByHierarchicalLevel(design, designs);
    for (auto designLevel: designs) {
      const SNLDesign* design = designLevel.first;
      std::filesystem::path filePath = path/getTopFileName(design);
      std::ofstream outFile;
      outFile.open(filePath);
      streamDumper.dumpDesign(design, outFile);
    }
  }
}

}} // namespace SNL // namespace naja
