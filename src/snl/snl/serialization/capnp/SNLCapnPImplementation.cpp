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

void dumpScalarNet(
  DBImplementation::LibraryImplementation::DesignImplementation::Net::Builder& net,
  const SNLScalarNet* scalarNet) {
  auto scalarNetBuilder = net.initScalarNet();
  scalarNetBuilder.setId(scalarNet->getID());
  if (not scalarNet->isAnonymous()) {
    scalarNetBuilder.setName(scalarNet->getName().getString());
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

void loadScalarNet(
  SNLDesign* design,
  const DBImplementation::LibraryImplementation::DesignImplementation::ScalarNet::Reader& net) {
  auto netID = net.getId();
  SNLName snlName;
  if (net.hasName()) {
    snlName = SNLName(net.getName());
  }
  SNLScalarNet::create(design, SNLID::DesignObjectID(netID), snlName);
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
        //loadBusNet(snlDesign, busNet);
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