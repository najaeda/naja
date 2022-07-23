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

#include "SNLCapnP.h"

#include <fcntl.h>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>

#include "snl_implementation.capnp.h"

#include "SNLUniverse.h"
#include "SNLScalarNet.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstTerm.h"

namespace {

using namespace naja::SNL;

void dumpInstance(
  DBImplementation::LibraryImplementation::DesignImplementation::Instance::Builder& instance,
  const SNLInstance* snlInstance) {
  instance.setId(snlInstance->getID());
  if (not snlInstance->isAnonymous()) {
    instance.setName(snlInstance->getName().getString());
  }
  auto model = snlInstance->getModel();
  auto modelReference = model->getReference();
  auto modelReferenceBuilder = instance.initModelReference();
  modelReferenceBuilder.setDbID(modelReference.dbID_);
  modelReferenceBuilder.setLibraryID(modelReference.libraryID_);
  modelReferenceBuilder.setDesignID(modelReference.designID_);
  instance.setModelReference(modelReferenceBuilder);
}

void dumpBitTermReference(
  DBImplementation::LibraryImplementation::DesignImplementation::NetComponentReference::Builder& componentReference,
  const SNLBitTerm* term) {
  auto termRefenceBuilder = componentReference.initTermReference();
  termRefenceBuilder.setTermID(term->getID());
  if (auto busTermBit = dynamic_cast<const SNLBusTermBit*>(term)) {
    termRefenceBuilder.setBit(busTermBit->getBit());
  }
}

void dumpInstTermReference(
  DBImplementation::LibraryImplementation::DesignImplementation::NetComponentReference::Builder& componentReference,
  const SNLInstTerm* instTerm) {
  auto instTermRefenceBuilder = componentReference.initInstTermReference();
  instTermRefenceBuilder.setInstanceID(instTerm->getInstance()->getID());
  auto term = instTerm->getTerm();
  instTermRefenceBuilder.setTermID(term->getID());
  if (auto busTermBit = dynamic_cast<const SNLBusTermBit*>(term)) {
    instTermRefenceBuilder.setBit(busTermBit->getBit());
  }
}

void dumpNetComponentReference(
  DBImplementation::LibraryImplementation::DesignImplementation::NetComponentReference::Builder& componentReference,
  const SNLNetComponent* component) {
  if (auto instTerm = dynamic_cast<const SNLInstTerm*>(component)) {
    dumpInstTermReference(componentReference, instTerm);
  } else {
    auto term = dynamic_cast<const SNLBitTerm*>(component);
    dumpBitTermReference(componentReference, term);
  }
}

void dumpScalarNet(
  DBImplementation::LibraryImplementation::DesignImplementation::Net::Builder& net,
  const SNLScalarNet* scalarNet) {
  auto scalarNetBuilder = net.initScalarNet();
  scalarNetBuilder.setId(scalarNet->getID());
  if (not scalarNet->isAnonymous()) {
    scalarNetBuilder.setName(scalarNet->getName().getString());
  }
  size_t componentsSize = scalarNet->getComponents().size();
  if (componentsSize > 0) {
    auto components = scalarNetBuilder.initComponents(componentsSize);
    size_t id = 0;
    for (auto component: scalarNet->getComponents()) {
      auto componentRefBuilder = components[id++];
      dumpNetComponentReference(componentRefBuilder, component);
    }
  }
}

void dumpBusNetBit(
  DBImplementation::LibraryImplementation::DesignImplementation::BusNetBit::Builder& bit,
  const SNLBusNetBit* busNetBit) {
  bit.setBit(busNetBit->getBit());
  size_t componentsSize = busNetBit->getComponents().size();
  if (componentsSize > 0) {
    auto components = bit.initComponents(componentsSize);
    size_t id = 0;
    for (auto component: busNetBit->getComponents()) {
      auto componentRefBuilder = components[id++];
      dumpNetComponentReference(componentRefBuilder, component);
    }
  }
}

void dumpBusNet(
  DBImplementation::LibraryImplementation::DesignImplementation::Net::Builder& net,
  const SNLBusNet* busNet) {
  auto busNetBuilder = net.initBusNet();
  busNetBuilder.setId(busNet->getID());
  if (not busNet->isAnonymous()) {
    busNetBuilder.setName(busNet->getName().getString());
  }
  busNetBuilder.setMsb(busNet->getMSB());
  busNetBuilder.setLsb(busNet->getLSB());
  auto bits = busNetBuilder.initBits(busNet->getBits().size());
  size_t id = 0;
  for (auto bit: busNet->getBits()) {
    auto bitBuilder = bits[id++];
    dumpBusNetBit(bitBuilder, bit);
  }
}

void dumpDesignImplementation(
  DBImplementation::LibraryImplementation::DesignImplementation::Builder& designImplementation,
  const SNLDesign* snlDesign) {
  designImplementation.setId(snlDesign->getID());

  size_t id = 0;
  auto instances = designImplementation.initInstances(snlDesign->getInstances().size());
  for (auto instance: snlDesign->getInstances()) {
    auto instanceBuilder = instances[id++];
    dumpInstance(instanceBuilder, instance);
  }

  id = 0;
  auto nets = designImplementation.initNets(snlDesign->getNets().size());
  for (auto net: snlDesign->getNets()) {
    auto netBuilder = nets[id++];
    if (auto scalarNet = dynamic_cast<const SNLScalarNet*>(net)) {
      dumpScalarNet(netBuilder, scalarNet);
    } else {
      auto busNet = static_cast<SNLBusNet*>(net);
      dumpBusNet(netBuilder, busNet);
    }
  }
}

void dumpLibraryImplementation(
  DBImplementation::LibraryImplementation::Builder& libraryImplementation,
  const SNLLibrary* snlLibrary) {
  libraryImplementation.setId(snlLibrary->getID());
  auto subLibraries = libraryImplementation.initLibraryImplementations(snlLibrary->getLibraries().size());
  size_t id = 0;
  for (auto subLib: snlLibrary->getLibraries()) {
    auto subLibraryBuilder = subLibraries[id++];
    dumpLibraryImplementation(subLibraryBuilder, subLib);
  }

  auto designs = libraryImplementation.initDesignImplementations(snlLibrary->getDesigns().size());
  id = 0;
  for (auto snlDesign: snlLibrary->getDesigns()) {
    auto designImplementationBuilder = designs[id++]; 
    dumpDesignImplementation(designImplementationBuilder, snlDesign);
  }
}

void loadInstance(
  SNLDesign* design,
  const DBImplementation::LibraryImplementation::DesignImplementation::Instance::Reader& instance) {
  auto instanceID = instance.getId();
  SNLName snlName;
  if (instance.hasName()) {
    snlName = SNLName(instance.getName());
  }
  auto modelReference = instance.getModelReference();
  auto snlModelReference =
    SNLID::DesignReference(modelReference.getDbID(), modelReference.getLibraryID(), modelReference.getDesignID());
  auto model = SNLUniverse::get()->getDesign(snlModelReference);
  if (not model) {
    //throw error
  }
  SNLInstance::create(design, model, SNLID::DesignObjectID(instanceID), snlName);
}

void loadTermReference(
  SNLBitNet* net,
  const DBImplementation::LibraryImplementation::DesignImplementation::TermReference::Reader& termReference) {
  auto design = net->getDesign();
  auto term = design->getTerm(SNLID::DesignObjectID(termReference.getTermID()));
  if (not term) {
    //throw error
  }
  if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
    scalarTerm->setNet(net);
  } else {
    auto busTerm = static_cast<SNLBusTerm*>(term);
    if (not busTerm) {
      //throw error
    }
    auto busTermBit = busTerm->getBit(termReference.getBit());
    if (not busTermBit) {
      //throw error
    }
    busTermBit->setNet(net);
  }
}

void loadInstTermReference(
  SNLBitNet* net,
  const DBImplementation::LibraryImplementation::DesignImplementation::InstTermReference::Reader& instTermReference) {
  auto instanceID = instTermReference.getInstanceID();
  auto design = net->getDesign();
  auto instance = design->getInstance(SNLID::InstanceID(instanceID));
  if (not instance) {
    //throw error
  }
  auto model = instance->getModel();
  auto termID = instTermReference.getTermID();
  auto term = model->getTerm(SNLID::DesignObjectID(termID));
  if (not term) {
    //throw error
  }
  SNLBitTerm* bitTerm = dynamic_cast<SNLScalarTerm*>(term);
  if (not bitTerm) {
    auto busTerm = static_cast<SNLBusTerm*>(term);
    if (not busTerm) {
      //throw error
    }
    bitTerm = busTerm->getBit(instTermReference.getBit());
    if (not bitTerm) {
      //throw error
    }
  }
  auto instTerm = instance->getInstTerm(bitTerm);
  if (not instTerm) {
    //throw error
  }
  instTerm->setNet(net);
}

void loadBusNet(
  SNLDesign* design,
  const DBImplementation::LibraryImplementation::DesignImplementation::BusNet::Reader& net) {
  SNLName snlName;
  if (net.hasName()) {
    snlName = SNLName(net.getName());
  }
  auto busNet = SNLBusNet::create(design, SNLID::DesignObjectID(net.getId()), net.getMsb(), net.getLsb(), snlName);
  if (net.hasBits()) {
    for (auto bitNet: net.getBits()) {
      if (bitNet.hasComponents()) {
        auto bit = bitNet.getBit();
        auto busNetBit = busNet->getBit(bit);
        if (not busNetBit) {
          //throw error
        }
        for (auto componentReference: bitNet.getComponents()) {
          if (componentReference.isInstTermReference()) {
            loadInstTermReference(busNetBit, componentReference.getInstTermReference());
          } else if (componentReference.isTermReference()) {
            loadTermReference(busNetBit, componentReference.getTermReference());
          } else {
            //throw error
          }
        }
      }
    }
  }
}

void loadScalarNet(
  SNLDesign* design,
  const DBImplementation::LibraryImplementation::DesignImplementation::ScalarNet::Reader& net) {
  SNLName snlName;
  if (net.hasName()) {
    snlName = SNLName(net.getName());
  }
  auto scalarNet = SNLScalarNet::create(design, SNLID::DesignObjectID(net.getId()), snlName);
  if (net.hasComponents()) {
    for (auto componentReference: net.getComponents()) {
      if (componentReference.isInstTermReference()) {
        loadInstTermReference(scalarNet, componentReference.getInstTermReference());
      } else if (componentReference.isTermReference()) {
        loadTermReference(scalarNet, componentReference.getTermReference());
      } else {
        //throw error
      }
    }
  }
}

void loadDesignImplementation(
  SNLLibrary* library,
  const DBImplementation::LibraryImplementation::DesignImplementation::Reader& designImplementation) {
  auto designID = designImplementation.getId();
  SNLDesign* snlDesign = library->getDesign(SNLID::DesignID(designID));
  if (not snlDesign) {
    //throw error
  }
  if (designImplementation.hasInstances()) {
    for (auto instance: designImplementation.getInstances()) {
      loadInstance(snlDesign, instance);
    }
  }
  if (designImplementation.hasNets()) {
    for (auto net: designImplementation.getNets()) {
      if (net.isScalarNet()) {
        auto scalarNet = net.getScalarNet();
        loadScalarNet(snlDesign, scalarNet);
      } else if (net.isBusNet()) {
        auto busNet = net.getBusNet();
        loadBusNet(snlDesign, busNet);
      } else {
        //throw error
      }
    }
  }
}

void loadLibraryImplementation(SNLDB* db, const DBImplementation::LibraryImplementation::Reader& libraryImplementation) {
  auto libraryID = libraryImplementation.getId();
  SNLLibrary* snlLibrary = db->getLibrary(SNLID::LibraryID(libraryID));
  if (not snlLibrary) {
    //throw error
  }
  if (libraryImplementation.hasDesignImplementations()) {
    for (auto designImplementation: libraryImplementation.getDesignImplementations()) {
      loadDesignImplementation(snlLibrary, designImplementation);
    } 
  }
}

}

namespace naja { namespace SNL {

void SNLCapnP::dumpImplementation(const SNLDB* snlDB, const std::filesystem::path& implementationPath) {
  ::capnp::MallocMessageBuilder message;

  DBImplementation::Builder db = message.initRoot<DBImplementation>();
  db.setId(snlDB->getID());
  auto libraries = db.initLibraryImplementations(snlDB->getLibraries().size());
  
  size_t id = 0;
  for (auto snlLibrary: snlDB->getLibraries()) {
    auto libraryImplementationBuilder = libraries[id++];
    dumpLibraryImplementation(libraryImplementationBuilder, snlLibrary);
  }

  int fd = open(
    implementationPath.c_str(),
    O_CREAT | O_WRONLY,
    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  writePackedMessageToFd(fd, message);
  close(fd);
}

SNLDB* SNLCapnP::loadImplementation(const std::filesystem::path& implementationPath) {
  //FIXME: verify if file can be opened
  int fd = open(implementationPath.c_str(), O_RDONLY);
  ::capnp::PackedFdMessageReader message(fd);

  DBImplementation::Reader dbImplementation = message.getRoot<DBImplementation>();
  auto dbID = dbImplementation.getId();
  auto universe = SNLUniverse::get();
  if (not universe) {
    //throw error
  }
  auto snldb = universe->getDB(dbID);
  if (dbImplementation.hasLibraryImplementations()) {
    for (auto libraryImplementation: dbImplementation.getLibraryImplementations()) {
      loadLibraryImplementation(snldb, libraryImplementation);
    }
  }
  return snldb;
}

}} // namespace SNL // namespace naja