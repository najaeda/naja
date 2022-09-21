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
#include <iostream>
#include <sstream>
#include <boost/asio.hpp>

#include <cassert>

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
#include "SNLException.h"

using boost::asio::ip::tcp;

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
  modelReferenceBuilder.setLibraryID(modelReference.getDBDesignReference().libraryID_);
  modelReferenceBuilder.setDesignID(modelReference.getDBDesignReference().designID_);
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
    SNLID::UniverseDesignReference(
      modelReference.getDbID(),
      modelReference.getLibraryID(),
      modelReference.getDesignID());
  auto model = SNLUniverse::get()->getDesign(snlModelReference);
  if (not model) {
    std::ostringstream reason;
    reason << "cannot deserialize instance: no model found with provided reference";
    throw SNLException(reason.str());
  }
  SNLInstance::create(design, model, SNLID::DesignObjectID(instanceID), snlName);
}

void loadTermReference(
  SNLBitNet* net,
  const DBImplementation::LibraryImplementation::DesignImplementation::TermReference::Reader& termReference) {
  auto design = net->getDesign();
  auto term = design->getTerm(SNLID::DesignObjectID(termReference.getTermID()));
  if (not term) {
    //LCOV_EXCL_START
    std::ostringstream reason;
    reason << "cannot deserialize term reference: no term found with provided reference";
    throw SNLException(reason.str());
    //LCOV_EXCL_STOP
  }
  if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
    scalarTerm->setNet(net);
  } else {
    auto busTerm = static_cast<SNLBusTerm*>(term);
    assert(busTerm);
    auto busTermBit = busTerm->getBit(termReference.getBit());
    if (not busTermBit) {
      //LCOV_EXCL_START
      std::ostringstream reason;
      reason << "cannot deserialize term reference: no bus term bit found with provided reference";
      throw SNLException(reason.str());
      //LCOV_EXCL_STOP
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
    //LCOV_EXCL_START
    std::ostringstream reason;
    reason << "cannot deserialize instance term reference: no instance found with provided reference";
    throw SNLException(reason.str());
    //LCOV_EXCL_STOP
  }
  auto model = instance->getModel();
  auto termID = instTermReference.getTermID();
  auto term = model->getTerm(SNLID::DesignObjectID(termID));
  if (not term) {
    //LCOV_EXCL_START
    std::ostringstream reason;
    reason << "cannot deserialize instance term reference: no instance found with provided reference";
    throw SNLException(reason.str());
    //LCOV_EXCL_STOP
  }
  SNLBitTerm* bitTerm = dynamic_cast<SNLScalarTerm*>(term);
  if (not bitTerm) {
    auto busTerm = static_cast<SNLBusTerm*>(term);
    assert(busTerm);
    bitTerm = busTerm->getBit(instTermReference.getBit());
    if (not bitTerm) {
      //LCOV_EXCL_START
      std::ostringstream reason;
      reason << "cannot deserialize instance term reference: no bit found in bus term with provided reference";
      throw SNLException(reason.str());
      //LCOV_EXCL_STOP
    }
  }
  auto instTerm = instance->getInstTerm(bitTerm);
  assert(instTerm);
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
          //LCOV_EXCL_START
          std::ostringstream reason;
          reason << "cannot deserialize bus net bit: no bit found in bus term with provided reference";
          throw SNLException(reason.str());
          //LCOV_EXCL_STOP
        }
        for (auto componentReference: bitNet.getComponents()) {
          if (componentReference.isInstTermReference()) {
            loadInstTermReference(busNetBit, componentReference.getInstTermReference());
          } else if (componentReference.isTermReference()) {
            loadTermReference(busNetBit, componentReference.getTermReference());
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
    std::ostringstream reason;
    reason << "cannot deserialize design: no design found in library with provided id";
    throw SNLException(reason.str());
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
      } 
    }
  }
}

void loadLibraryImplementation(SNLDB* db, const DBImplementation::LibraryImplementation::Reader& libraryImplementation) {
  auto libraryID = libraryImplementation.getId();
  SNLLibrary* snlLibrary = db->getLibrary(SNLID::LibraryID(libraryID));
  if (not snlLibrary) {
    std::ostringstream reason;
    reason << "cannot deserialize library: no library found in db with provided reference";
    throw SNLException(reason.str());
  }
  if (libraryImplementation.hasDesignImplementations()) {
    for (auto designImplementation: libraryImplementation.getDesignImplementations()) {
      loadDesignImplementation(snlLibrary, designImplementation);
    } 
  }
}

}

namespace naja { namespace SNL {

void SNLCapnP::dumpImplementation(const SNLDB* snlDB, int fileDescriptor) {
  ::capnp::MallocMessageBuilder message;

  DBImplementation::Builder db = message.initRoot<DBImplementation>();
  db.setId(snlDB->getID());
  auto libraries = db.initLibraryImplementations(snlDB->getLibraries().size());
  
  size_t id = 0;
  for (auto snlLibrary: snlDB->getLibraries()) {
    auto libraryImplementationBuilder = libraries[id++];
    dumpLibraryImplementation(libraryImplementationBuilder, snlLibrary);
  }
  writePackedMessageToFd(fileDescriptor, message);
}

void SNLCapnP::dumpImplementation(const SNLDB* snlDB, const std::filesystem::path& implementationPath) {
  int fd = open(
    implementationPath.c_str(),
    O_CREAT | O_WRONLY,
    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  dumpImplementation(snlDB, fd);
  close(fd);
}

void SNLCapnP::sendImplementation(const SNLDB* db, tcp::socket& socket) {
  dumpImplementation(db, socket.native_handle());
}

void SNLCapnP::sendImplementation(
  const SNLDB* db,
  const std::string& ipAddress,
  uint16_t port) {
  boost::asio::io_service io_service;
  //socket creation
  tcp::socket socket(io_service);
  socket.connect(tcp::endpoint(boost::asio::ip::address::from_string(ipAddress), port));
  sendImplementation(db, socket);
}

SNLDB* SNLCapnP::loadImplementation(int fileDescriptor) {
  ::capnp::PackedFdMessageReader message(fileDescriptor);

  DBImplementation::Reader dbImplementation = message.getRoot<DBImplementation>();
  auto dbID = dbImplementation.getId();
  auto universe = SNLUniverse::get();
  if (not universe) {
    std::ostringstream reason;
    reason << "cannot deserialize DB implementation: no existing universe";
    throw SNLException(reason.str());
  }
  auto snldb = universe->getDB(dbID);
  if (dbImplementation.hasLibraryImplementations()) {
    for (auto libraryImplementation: dbImplementation.getLibraryImplementations()) {
      loadLibraryImplementation(snldb, libraryImplementation);
    }
  }
  return snldb;
}

SNLDB* SNLCapnP::loadImplementation(const std::filesystem::path& implementationPath) {
  //FIXME: verify if file can be opened
  int fd = open(implementationPath.c_str(), O_RDONLY);
  return loadImplementation(fd);
}

SNLDB* SNLCapnP::receiveImplementation(tcp::socket& socket) {
  return loadImplementation(socket.native_handle());
}

SNLDB* SNLCapnP::receiveImplementation(uint16_t port) {
  boost::asio::io_service io_service;
  //listen for new connection
  tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port));
  //socket creation 
  tcp::socket socket(io_service);
  //waiting for connection
  acceptor_.accept(socket);
  SNLDB* db = receiveImplementation(socket);
  return db;
}

}} // namespace SNL // namespace naja
