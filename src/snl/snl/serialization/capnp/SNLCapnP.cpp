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

#include "snl_interface.capnp.h"

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"

namespace {

using namespace naja::SNL;

DBInterface::LibraryInterface::DesignInterface::Direction SNLtoCapnPDirection(SNLTerm::Direction direction) {
  switch (direction) {
    case SNLTerm::Direction::Input:
      return DBInterface::LibraryInterface::DesignInterface::Direction::INPUT;
    case SNLTerm::Direction::Output:
      return DBInterface::LibraryInterface::DesignInterface::Direction::OUTPUT;
    case SNLTerm::Direction::InOut:
      return DBInterface::LibraryInterface::DesignInterface::Direction::INOUT;
  }
  return DBInterface::LibraryInterface::DesignInterface::Direction::INPUT;
}

void dumpScalarTerm(
  DBInterface::LibraryInterface::DesignInterface::Term::Builder& term,
  const SNLScalarTerm* scalarTerm) {
  auto scalarTermBuilder = term.initScalarTerm();
  scalarTermBuilder.setId(scalarTerm->getID());
  if (not scalarTerm->isAnonymous()) {
    scalarTermBuilder.setName(scalarTerm->getName().getString());
  }
  scalarTermBuilder.setDirection(SNLtoCapnPDirection(scalarTerm->getDirection()));
}

void dumpBusTerm(
  DBInterface::LibraryInterface::DesignInterface::Term::Builder& term,
  const SNLBusTerm* busTerm) {
  auto busTermBuilder = term.initBusTerm();
  busTermBuilder.setId(busTerm->getID());
  if (not busTerm->isAnonymous()) {
    busTermBuilder.setName(busTerm->getName().getString());
  }
  busTermBuilder.setMsb(busTerm->getMSB());
  busTermBuilder.setLsb(busTerm->getLSB());
  busTermBuilder.setDirection(SNLtoCapnPDirection(busTerm->getDirection()));
}

DBInterface::LibraryInterface::DesignType SNLtoCapNpDesignType(SNLDesign::Type type) {
  switch (type) {
    case SNLDesign::Type::Standard:
      return DBInterface::LibraryInterface::DesignType::STANDARD;
    case SNLDesign::Type::Primitive:
      return DBInterface::LibraryInterface::DesignType::PRIMITIVE;
    case SNLDesign::Type::Blackbox:
      return DBInterface::LibraryInterface::DesignType::BLACKBOX;
  }
  return DBInterface::LibraryInterface::DesignType::STANDARD;
}

void dumpDesignInterface(
  DBInterface::LibraryInterface::DesignInterface::Builder& designInterface,
  const SNLDesign* snlDesign) {
  designInterface.setId(snlDesign->getID());
  if (not snlDesign->isAnonymous()) {
    designInterface.setName(snlDesign->getName().getString());
  }
  designInterface.setType(SNLtoCapNpDesignType(snlDesign->getType()));
  auto terms = designInterface.initTerms(snlDesign->getTerms().size());

  size_t id = 0;
  for (auto term: snlDesign->getTerms()) {
    auto termBuilder = terms[id++];
    if (auto scalarTerm = dynamic_cast<const SNLScalarTerm*>(term)) {
      dumpScalarTerm(termBuilder, scalarTerm);
    } else {
      auto busTerm = static_cast<SNLBusTerm*>(term);
      dumpBusTerm(termBuilder, busTerm);
    }
  }
}

DBInterface::LibraryType SNLtoCapnPLibraryType(SNLLibrary::Type type) {
  switch (type) {
    case SNLLibrary::Type::Standard:
      return DBInterface::LibraryType::STANDARD;
    case SNLLibrary::Type::Primitives:
      return DBInterface::LibraryType::PRIMITIVES;
    case SNLLibrary::Type::InDB0:
      //FIXME: ERROR
      return DBInterface::LibraryType::STANDARD;
  }
  return DBInterface::LibraryType::STANDARD;
}

SNLLibrary::Type CapnPtoSNLLibraryType(DBInterface::LibraryType type) {
  switch (type) {
    case DBInterface::LibraryType::STANDARD:
      return SNLLibrary::Type::Standard;
    case DBInterface::LibraryType::PRIMITIVES:
      return  SNLLibrary::Type::Primitives;
  }
  return SNLLibrary::Type::Standard;
}

void dumpLibraryInterface(
  DBInterface::LibraryInterface::Builder& libraryInterface,
  const SNLLibrary* snlLibrary) {
  libraryInterface.setId(snlLibrary->getID());
  if (not snlLibrary->isAnonymous()) {
    libraryInterface.setName(snlLibrary->getName().getString());
  }
  libraryInterface.setType(SNLtoCapnPLibraryType(snlLibrary->getType()));
  auto subLibraries = libraryInterface.initLibraryInterfaces(snlLibrary->getLibraries().size());
  size_t id = 0;
  for (auto subLib: snlLibrary->getLibraries()) {
    auto subLibraryBuilder = subLibraries[id++];
    dumpLibraryInterface(subLibraryBuilder, subLib);
  }

  auto designs = libraryInterface.initDesignInterfaces(snlLibrary->getDesigns().size());
  id = 0;
  for (auto snlDesign: snlLibrary->getDesigns()) {
    auto designInterfaceBuilder = designs[id++]; 
    dumpDesignInterface(designInterfaceBuilder, snlDesign);
  }

#if 0
  std::vector<flatbuffers::Offset<LibraryInterface>> librariesVector;
  for (auto lib: library->getLibraries()) {
    librariesVector.push_back(dumpLibraryInterface(builder, lib));
  }
  auto libraries = builder.CreateVector(librariesVector);

  std::vector<flatbuffers::Offset<DesignInterface>> designInterfacesVector;
  for (auto design: library->getDesigns()) {
    designInterfacesVector.push_back(dumpDesignInterface(builder, design));
  }
  auto designInterfaces = builder.CreateVector(designInterfacesVector);
  flatbuffers::Offset<flatbuffers::String> name; 
  if (not library->isAnonymous()) {
    name = builder.CreateString(library->getName().getString());
  }
  LibraryInterfaceBuilder libBuilder(builder);
  libBuilder.add_id(library->getID());
  if (not library->isAnonymous()) {
    libBuilder.add_name(name);
  }
  libBuilder.add_type(SNLtoFBSLibraryType(library->getType()));
  libBuilder.add_library_interfaces(libraries);
  libBuilder.add_design_interfaces(designInterfaces);
  
  //libBuilder.add_type();
  auto fbLib = libBuilder.Finish();
  return fbLib;
#endif

}

SNLDesign::Type CapnPtoSNLDesignType(DBInterface::LibraryInterface::DesignType type) {
  switch (type) {
    case DBInterface::LibraryInterface::DesignType::STANDARD:
      return SNLDesign::Type::Standard;
    case DBInterface::LibraryInterface::DesignType::BLACKBOX: 
      return SNLDesign::Type::Blackbox;
    case DBInterface::LibraryInterface::DesignType::PRIMITIVE: 
      return SNLDesign::Type::Primitive;
  }
  return SNLDesign::Type::Standard;
}

SNLTerm::Direction CapnPtoSNLDirection(DBInterface::LibraryInterface::DesignInterface::Direction direction) {
  switch (direction) {
    case DBInterface::LibraryInterface::DesignInterface::Direction::INPUT:
      return SNLTerm::Direction::Input;
    case DBInterface::LibraryInterface::DesignInterface::Direction::OUTPUT:
      return SNLTerm::Direction::Output;
    case DBInterface::LibraryInterface::DesignInterface::Direction::INOUT:
      return SNLTerm::Direction::InOut;
  }
  return SNLTerm::Direction::Input;
}

void loadDesignBusTerm(
  SNLDesign* design,
  const DBInterface::LibraryInterface::DesignInterface::BusTerm::Reader& term) {
  auto termID = term.getId();
  auto termDirection = term.getDirection();
  auto msb = term.getMsb();
  auto lsb = term.getLsb();
  SNLName snlName;
  if (term.hasName()) {
    snlName = SNLName(term.getName());
  }
  SNLBusTerm::create(design, SNLID::DesignObjectID(termID), CapnPtoSNLDirection(termDirection), msb, lsb, snlName);
}

void loadDesignScalarTerm(
  SNLDesign* design,
  const DBInterface::LibraryInterface::DesignInterface::ScalarTerm::Reader& term) {
  auto termID = term.getId();
  auto termDirection = term.getDirection();
  SNLName snlName;
  if (term.hasName()) {
    snlName = SNLName(term.getName());
  }
  SNLScalarTerm::create(design, SNLID::DesignObjectID(termID), CapnPtoSNLDirection(termDirection), snlName);
}

void loadDesignParameter(
  SNLDesign* design,
  const DBInterface::LibraryInterface::DesignInterface::Parameter::Reader& parameter) {
  auto name = parameter.getName();
  auto value = parameter.getValue();
  SNLParameter::create(design, SNLName(name), value);
}

void loadDesignInterface(
  SNLLibrary* library,
  const DBInterface::LibraryInterface::DesignInterface::Reader& designInterface) {
  auto designID = designInterface.getId();
  auto designType = designInterface.getType();
  SNLDesign* snlDesign = nullptr;
  if (designInterface.hasName()) {
    auto designName = designInterface.getName();
    snlDesign =
      SNLDesign::create(library, SNLID::DesignID(designID), CapnPtoSNLDesignType(designType), SNLName(designName));
  } else {
    snlDesign =
      SNLDesign::create(library, SNLID::DesignID(designID), CapnPtoSNLDesignType(designType));
  }
  if (designInterface.hasParameters()) {
    for (auto parameter: designInterface.getParameters()) {
      loadDesignParameter(snlDesign, parameter);
    }
  }
  if (designInterface.hasTerms()) {
    for (auto term: designInterface.getTerms()) {
      if (term.isScalarTerm()) {
        auto scalarTerm = term.getScalarTerm();
        loadDesignScalarTerm(snlDesign, scalarTerm);
      } else if (term.isBusTerm()) {
        auto busTerm = term.getBusTerm();
        loadDesignBusTerm(snlDesign, busTerm);
      }
    }
  }
}

void loadLibraryInterface(SNLDB* db, const DBInterface::LibraryInterface::Reader& libraryInterface) {
  auto libraryID = libraryInterface.getId();
  auto libraryType = libraryInterface.getType();
  SNLLibrary* snlLibrary = nullptr;
  if (libraryInterface.hasName()) {
    auto libraryName = libraryInterface.getName();
    snlLibrary =
      SNLLibrary::create(db, SNLID::LibraryID(libraryID), CapnPtoSNLLibraryType(libraryType), SNLName(libraryName));
  } else {
    snlLibrary =
      SNLLibrary::create(db, SNLID::LibraryID(libraryID), CapnPtoSNLLibraryType(libraryType));
  }
  if (libraryInterface.hasDesignInterfaces()) {
    for (auto designInterface: libraryInterface.getDesignInterfaces()) {
      loadDesignInterface(snlLibrary, designInterface);
    } 
  }
}

}

namespace naja { namespace SNL {

void SNLCapnP::dumpInterface(const SNLDB* snlDB, const std::filesystem::path& interfacePath) {
  ::capnp::MallocMessageBuilder message;

  DBInterface::Builder db = message.initRoot<DBInterface>();
  db.setId(snlDB->getID());
  auto libraries = db.initLibraryInterfaces(snlDB->getLibraries().size());
  
  size_t id = 0;
  for (auto snlLibrary: snlDB->getLibraries()) {
    auto libraryInterfaceBuilder = libraries[id++];
    dumpLibraryInterface(libraryInterfaceBuilder, snlLibrary);
  }

  int fd = open(
    interfacePath.c_str(),
    O_CREAT | O_WRONLY,
    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  writePackedMessageToFd(fd, message);
  close(fd);
}

void SNLCapnP::dump(const SNLDB* db, const std::filesystem::path& path) {
  std::filesystem::create_directory(path);
  dumpInterface(db, path/InterfaceName);
}

SNLDB* SNLCapnP::loadInterface(const std::filesystem::path& interfacePath) {
  //FIXME: verify if file can be opened
  int fd = open(interfacePath.c_str(), O_RDONLY);
  ::capnp::PackedFdMessageReader message(fd);

  DBInterface::Reader dbInterface = message.getRoot<DBInterface>();
  auto dbID = dbInterface.getId();
  auto universe = SNLUniverse::get();
  if (not universe) {
    universe = SNLUniverse::create();
  }
  auto snldb = SNLDB::create(universe, dbID);
  if (dbInterface.hasLibraryInterfaces()) {
    for (auto libraryInterface: dbInterface.getLibraryInterfaces()) {
      loadLibraryInterface(snldb, libraryInterface);
    }
  }
  return snldb;
}

SNLDB* SNLCapnP::load(const std::filesystem::path& path) {
  SNLDB* db = loadInterface(path/InterfaceName);
  return db;
}

}} // namespace SNL // namespace naja