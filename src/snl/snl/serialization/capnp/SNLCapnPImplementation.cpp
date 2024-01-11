// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

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

DBImplementation::LibraryImplementation::DesignImplementation::NetType SNLtoCapnPNetType(SNLNet::Type type) {
  switch (type) {
    case SNLNet::Type::Standard:
      return DBImplementation::LibraryImplementation::DesignImplementation::NetType::STANDARD;
    case SNLNet::Type::Assign0:
      return DBImplementation::LibraryImplementation::DesignImplementation::NetType::ASSIGN0;
    case SNLNet::Type::Assign1:
      return DBImplementation::LibraryImplementation::DesignImplementation::NetType::ASSIGN1;
    case SNLNet::Type::Supply0:
      return DBImplementation::LibraryImplementation::DesignImplementation::NetType::SUPPLY0;
    case SNLNet::Type::Supply1:
      return DBImplementation::LibraryImplementation::DesignImplementation::NetType::SUPPLY1;
  }
  return DBImplementation::LibraryImplementation::DesignImplementation::NetType::STANDARD; //LCOV_EXCL_LINE
}

void dumpInstParameter(
  DBImplementation::LibraryImplementation::DesignImplementation::Instance::InstParameter::Builder& instParameter,
  const SNLInstParameter* snlInstParameter) {
  instParameter.setName(snlInstParameter->getName().getString());
  instParameter.setValue(snlInstParameter->getValue());
}

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
  size_t id = 0;
  auto instParameters = instance.initInstParameters(snlInstance->getInstParameters().size());
  for (auto instParameter: snlInstance->getInstParameters()) {
    auto instParameterBuilder = instParameters[id++];
    dumpInstParameter(instParameterBuilder, instParameter);
  }
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
  scalarNetBuilder.setType(SNLtoCapnPNetType(scalarNet->getType()));
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
  bit.setType(SNLtoCapnPNetType(busNetBit->getType()));
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
  for (auto bit: busNet->getBusBits()) {
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

SNLNet::Type CapnPtoSNLNetType(DBImplementation::LibraryImplementation::DesignImplementation::NetType type) {
  switch (type) {
    case DBImplementation::LibraryImplementation::DesignImplementation::NetType::STANDARD:
      return SNLNet::Type::Standard;
    case DBImplementation::LibraryImplementation::DesignImplementation::NetType::ASSIGN0:
      return SNLNet::Type::Assign0;
    case DBImplementation::LibraryImplementation::DesignImplementation::NetType::ASSIGN1:
      return SNLNet::Type::Assign1;
    case DBImplementation::LibraryImplementation::DesignImplementation::NetType::SUPPLY0:
      return SNLNet::Type::Supply0;
    case DBImplementation::LibraryImplementation::DesignImplementation::NetType::SUPPLY1:
      return SNLNet::Type::Supply1;
  }
  return SNLNet::Type::Standard; //LCOV_EXCL_LINE
}

void loadInstParameter(
  SNLInstance* instance,
  const DBImplementation::LibraryImplementation::DesignImplementation::Instance::InstParameter::Reader& instParameter) {
  auto name = instParameter.getName();
  auto value = instParameter.getValue();
  auto parameter = instance->getModel()->getParameter(SNLName(name));
  //LCOV_EXCL_START
  if (not parameter) {
    std::ostringstream reason;
    reason << "cannot deserialize instance: no parameter " << std::string(name);
    reason << " exists in " << instance->getDescription() << " model";
    throw SNLException(reason.str());
  }
  //LCOV_EXCL_STOP
  SNLInstParameter::create(instance, parameter, value);
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
    SNLID::DesignReference(
      modelReference.getDbID(),
      modelReference.getLibraryID(),
      modelReference.getDesignID());
  auto model = SNLUniverse::get()->getDesign(snlModelReference);
  //LCOV_EXCL_START
  if (not model) {
    std::ostringstream reason;
    reason << "cannot deserialize instance: no model found with provided reference";
    throw SNLException(reason.str());
  }
  //LCOV_EXCL_STOP
  auto snlInstance =
    SNLInstance::create(design, model, SNLID::DesignObjectID(instanceID), snlName);
  if (instance.hasInstParameters()) {
    for (auto instParameter: instance.getInstParameters()) {
      loadInstParameter(snlInstance, instParameter);
    }
  }
}

void loadTermReference(
  SNLBitNet* net,
  const DBImplementation::LibraryImplementation::DesignImplementation::TermReference::Reader& termReference) {
  auto design = net->getDesign();
  auto term = design->getTerm(SNLID::DesignObjectID(termReference.getTermID()));
  //LCOV_EXCL_START
  if (not term) {
    std::ostringstream reason;
    reason << "cannot deserialize term reference: no term found with provided reference";
    throw SNLException(reason.str());
  }
  //LCOV_EXCL_STOP
  if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(term)) {
    scalarTerm->setNet(net);
  } else {
    auto busTerm = static_cast<SNLBusTerm*>(term);
    assert(busTerm);
    auto busTermBit = busTerm->getBit(termReference.getBit());
    //LCOV_EXCL_START
    if (not busTermBit) {
      std::ostringstream reason;
      reason << "cannot deserialize term reference: no bus term bit found with provided reference";
      throw SNLException(reason.str());
    }
    //LCOV_EXCL_STOP
    busTermBit->setNet(net);
  }
}

void loadInstTermReference(
  SNLBitNet* net,
  const DBImplementation::LibraryImplementation::DesignImplementation::InstTermReference::Reader& instTermReference) {
  auto instanceID = instTermReference.getInstanceID();
  auto design = net->getDesign();
  auto instance = design->getInstance(SNLID::DesignObjectID(instanceID));
  //LCOV_EXCL_START
  if (not instance) {
    std::ostringstream reason;
    reason << "cannot deserialize instance term reference, no instance found with ID ";
    reason << instanceID << " in design " << design->getDescription();
    throw SNLException(reason.str());
  }
  //LCOV_EXCL_STOP
  auto model = instance->getModel();
  auto termID = instTermReference.getTermID();
  auto term = model->getTerm(SNLID::DesignObjectID(termID));
  //LCOV_EXCL_START
  if (not term) {
    std::ostringstream reason;
    reason << "cannot deserialize instance " << instance->getDescription();
    reason << " term reference: no term found with ID ";
    reason << termID << " in model " << model->getDescription();
    throw SNLException(reason.str());
  }
  //LCOV_EXCL_STOP
  SNLBitTerm* bitTerm = dynamic_cast<SNLScalarTerm*>(term);
  if (not bitTerm) {
    auto busTerm = static_cast<SNLBusTerm*>(term);
    assert(busTerm);
    bitTerm = busTerm->getBit(instTermReference.getBit());
    //LCOV_EXCL_START
    if (not bitTerm) {
      std::ostringstream reason;
      reason << "cannot deserialize instance term reference: no bit found in bus term with provided reference";
      throw SNLException(reason.str());
    }
    //LCOV_EXCL_STOP
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
      auto bit = bitNet.getBit();
      auto busNetBit = busNet->getBit(bit);
      //LCOV_EXCL_START
      if (not busNetBit) {
        std::ostringstream reason;
        reason << "cannot deserialize bus net bit: no bit found in bus term with provided reference";
        throw SNLException(reason.str());
      }
      //LCOV_EXCL_STOP
      busNetBit->setType(CapnPtoSNLNetType(bitNet.getType()));
      if (bitNet.hasComponents()) {
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
  scalarNet->setType(CapnPtoSNLNetType(net.getType()));
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
  //LCOV_EXCL_START
  if (not snlDesign) {
    std::ostringstream reason;
    reason << "cannot deserialize design: no design found in library with provided id";
    throw SNLException(reason.str());
  }
  //LCOV_EXCL_STOP
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
  //LCOV_EXCL_START
  if (not snlLibrary) {
    std::ostringstream reason;
    reason << "cannot load library with id " << libraryID
      << " in db " << db->getDescription();
    if (db->getLibraries().empty()) {
      reason << ", no libraries in this db.";
    } else {
      reason << ", existing libraires are: " << std::endl;
      for (auto lib: db->getLibraries()) {
        reason << lib->getDescription() << std::endl;
      }
    }
    throw SNLException(reason.str());
  }
  //LCOV_EXCL_STOP
  if (libraryImplementation.hasDesignImplementations()) {
    for (auto designImplementation: libraryImplementation.getDesignImplementations()) {
      loadDesignImplementation(snlLibrary, designImplementation);
    } 
  }
}

}

namespace naja { namespace SNL {

void SNLCapnP::dumpImplementation(const SNLDB* snlDB, int fileDescriptor) {
  dumpImplementation(snlDB, fileDescriptor, snlDB->getID());
}

void SNLCapnP::dumpImplementation(const SNLDB* snlDB, int fileDescriptor, SNLID::DBID forceDBID) {
  ::capnp::MallocMessageBuilder message;

  DBImplementation::Builder db = message.initRoot<DBImplementation>();
  db.setId(forceDBID);
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

//LCOV_EXCL_START
void SNLCapnP::sendImplementation(const SNLDB* db, tcp::socket& socket) {
  sendImplementation(db, socket, db->getID());
}

void SNLCapnP::sendImplementation(const SNLDB* db, tcp::socket& socket, SNLID::DBID forceDBID) {
  dumpImplementation(db, socket.native_handle(), forceDBID);
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
//LCOV_EXCL_STOP

SNLDB* SNLCapnP::loadImplementation(int fileDescriptor) {
  ::capnp::PackedFdMessageReader message(fileDescriptor);

  DBImplementation::Reader dbImplementation = message.getRoot<DBImplementation>();
  auto dbID = dbImplementation.getId();
  auto universe = SNLUniverse::get();
  //LCOV_EXCL_START
  if (not universe) {
    std::ostringstream reason;
    reason << "cannot deserialize DB implementation: no existing universe";
    throw SNLException(reason.str());
  }
  //LCOV_EXCL_STOP
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

//LCOV_EXCL_START
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
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja
