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
#include <sstream>
#include <list>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>

#include "snl_interface.capnp.h"

#include "NajaDumpableProperty.h"
#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "SNLException.h"

namespace {

using namespace naja;
using namespace naja::SNL;

void dumpProperty(
  Property::Builder& property,
  const NajaProperty* najaProperty) {
    property.setName(najaProperty->getName());
}

template<typename T> void dumpProperties(
  T& dumpObjectInterface,
  const NajaObject* object,
  auto& initProperties) {
  using NajaProperties = std::list<NajaProperty*>;
  NajaProperties najaProperties(object->getDumpableProperties().begin(), object->getDumpableProperties().end());
  auto properties = initProperties(dumpObjectInterface, najaProperties.size());
  size_t id = 0;
  for (auto najaProperty: najaProperties) {
    auto propertyBuilder = properties[id++];
    dumpProperty(propertyBuilder, najaProperty);
  }
}

DBInterface::LibraryInterface::DesignInterface::Direction SNLtoCapnPDirection(SNLTerm::Direction direction) {
  switch (direction) {
    case SNLTerm::Direction::Input:
      return DBInterface::LibraryInterface::DesignInterface::Direction::INPUT;
    case SNLTerm::Direction::Output:
      return DBInterface::LibraryInterface::DesignInterface::Direction::OUTPUT;
    case SNLTerm::Direction::InOut:
      return DBInterface::LibraryInterface::DesignInterface::Direction::INOUT;
  }
  return DBInterface::LibraryInterface::DesignInterface::Direction::INPUT; //LCOV_EXCL_LINE
}

DBInterface::LibraryInterface::DesignInterface::ParameterType SNLtoCapNpParameterType(SNLParameter::Type type) {
  switch (type) {
    case SNLParameter::Type::Decimal:
      return DBInterface::LibraryInterface::DesignInterface::ParameterType::DECIMAL;
    case SNLParameter::Type::Binary:
      return DBInterface::LibraryInterface::DesignInterface::ParameterType::BINARY;
    case SNLParameter::Type::Boolean:
      return DBInterface::LibraryInterface::DesignInterface::ParameterType::BOOLEAN;
    case SNLParameter::Type::String:
      return DBInterface::LibraryInterface::DesignInterface::ParameterType::STRING;
  }
  return DBInterface::LibraryInterface::DesignInterface::ParameterType::DECIMAL; //LCOV_EXCL_LINE
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

void dumpParameter(
  DBInterface::LibraryInterface::DesignInterface::Parameter::Builder& parameter,
  const SNLParameter* snlParameter) {
  parameter.setName(snlParameter->getName().getString());
  parameter.setType(SNLtoCapNpParameterType(snlParameter->getType()));
  parameter.setValue(snlParameter->getValue());
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
  return DBInterface::LibraryInterface::DesignType::STANDARD; //LCOV_EXCL_LINE
}

void dumpDesignInterface(
  DBInterface::LibraryInterface::DesignInterface::Builder& designInterface,
  const SNLDesign* snlDesign) {
  designInterface.setId(snlDesign->getID());
  if (not snlDesign->isAnonymous()) {
    designInterface.setName(snlDesign->getName().getString());
  }
  designInterface.setType(SNLtoCapNpDesignType(snlDesign->getType()));
  auto lambda = [](DBInterface::LibraryInterface::DesignInterface::Builder& builder, size_t nbProperties) {
    return builder.initProperties(nbProperties);
  };
  dumpProperties(designInterface, snlDesign, lambda);

  size_t id = 0;
  auto parameters = designInterface.initParameters(snlDesign->getParameters().size());
  for (auto parameter: snlDesign->getParameters()) {
    auto parameterBuilder = parameters[id++];
    dumpParameter(parameterBuilder, parameter);
  }
  
  id = 0;
  auto terms = designInterface.initTerms(snlDesign->getTerms().size());
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
    //LCOV_EXCL_START
    case SNLLibrary::Type::InDB0:
      throw SNLException("Unexpected InDB0 Library type while loading Library");
    ////LCOV_EXCL_STOP
  }
  return DBInterface::LibraryType::STANDARD; //LCOV_EXCL_LINE
}

SNLLibrary::Type CapnPtoSNLLibraryType(DBInterface::LibraryType type) {
  switch (type) {
    case DBInterface::LibraryType::STANDARD:
      return SNLLibrary::Type::Standard;
    case DBInterface::LibraryType::PRIMITIVES:
      return  SNLLibrary::Type::Primitives;
  }
  return SNLLibrary::Type::Standard; //LCOV_EXCL_LINE
}

void dumpLibraryInterface(
  DBInterface::LibraryInterface::Builder& libraryInterface,
  const SNLLibrary* snlLibrary) {
  libraryInterface.setId(snlLibrary->getID());
  auto lambda = [](DBInterface::LibraryInterface::Builder& builder, size_t nbProperties) {
    return builder.initProperties(nbProperties);
  };
  dumpProperties(libraryInterface, snlLibrary, lambda);

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
  return SNLDesign::Type::Standard; //LCOV_EXCL_LINE
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
  return SNLTerm::Direction::Input; //LCOV_EXCL_LINE
}

SNLParameter::Type CapnPtoSNLParameterType(DBInterface::LibraryInterface::DesignInterface::ParameterType type) {
  switch (type) {
    case DBInterface::LibraryInterface::DesignInterface::ParameterType::DECIMAL:
      return SNLParameter::Type::Decimal;
    case DBInterface::LibraryInterface::DesignInterface::ParameterType::BINARY:
      return SNLParameter::Type::Binary;
    case DBInterface::LibraryInterface::DesignInterface::ParameterType::BOOLEAN:
      return SNLParameter::Type::Boolean;
    case DBInterface::LibraryInterface::DesignInterface::ParameterType::STRING:
      return SNLParameter::Type::String;
  }
  return SNLParameter::Type::Decimal; //LCOV_EXCL_LINE
}

template<typename T> void loadProperties(
  const T& dumpObjectReader,
  NajaObject* object,
  auto& propertiesGetter) {
  for (auto property: propertiesGetter(dumpObjectReader)) {
    NajaDumpableProperty::create(object, property.getName());
  }
}

void loadScalarTerm(
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

void loadBusTerm(
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

void loadDesignParameter(
  SNLDesign* design,
  const DBInterface::LibraryInterface::DesignInterface::Parameter::Reader& parameter) {
  auto name = parameter.getName();
  auto type = parameter.getType();
  auto value = parameter.getValue();
  SNLParameter::create(design, SNLName(name), CapnPtoSNLParameterType(type), value);
}

void loadDesignInterface(
  SNLLibrary* library,
  const DBInterface::LibraryInterface::DesignInterface::Reader& designInterface) {
  auto designID = designInterface.getId();
  auto designType = designInterface.getType();
  SNLName snlName;
  if (designInterface.hasName()) {
    snlName = SNLName(designInterface.getName());
  }
  SNLDesign* snlDesign = SNLDesign::create(library, SNLID::DesignID(designID), CapnPtoSNLDesignType(designType), snlName);
   if (designInterface.hasProperties()) {
    auto lambda = [](const DBInterface::LibraryInterface::DesignInterface::Reader& reader) {
      return reader.getProperties();
    };
    loadProperties(designInterface, snlDesign, lambda);
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
        loadScalarTerm(snlDesign, scalarTerm);
      } else if (term.isBusTerm()) {
        auto busTerm = term.getBusTerm();
        loadBusTerm(snlDesign, busTerm);
      }
    }
  }
}

void loadLibraryInterface(NajaObject* parent, const DBInterface::LibraryInterface::Reader& libraryInterface) {
  SNLLibrary* parentLibrary = nullptr;
  SNLDB* parentDB = dynamic_cast<SNLDB*>(parent);
  if (not parentDB) {
    parentLibrary = static_cast<SNLLibrary*>(parent);
  }
  auto libraryID = libraryInterface.getId();
  auto libraryType = libraryInterface.getType();
  SNLName snlName;
  if (libraryInterface.hasName()) {
    snlName = SNLName(libraryInterface.getName());
  }
  SNLLibrary* snlLibrary = nullptr;
  if (parentDB) {
    snlLibrary = SNLLibrary::create(parentDB, SNLID::LibraryID(libraryID), CapnPtoSNLLibraryType(libraryType), snlName);
  } else {
    snlLibrary = SNLLibrary::create(parentLibrary, SNLID::LibraryID(libraryID), CapnPtoSNLLibraryType(libraryType), snlName);
  }
  if (libraryInterface.hasProperties()) {
    auto lambda = [](const DBInterface::LibraryInterface::Reader& reader) {
      return reader.getProperties();
    };
    loadProperties(libraryInterface, snlLibrary, lambda);
  }
  if (libraryInterface.hasDesignInterfaces()) {
    for (auto designInterface: libraryInterface.getDesignInterfaces()) {
      loadDesignInterface(snlLibrary, designInterface);
    } 
  }
  if (libraryInterface.hasLibraryInterfaces()) {
    for (auto subLibraryInterface: libraryInterface.getLibraryInterfaces()) {
      loadLibraryInterface(snlLibrary, subLibraryInterface);
    }
  }
}

}

namespace naja { namespace SNL {

void SNLCapnP::dumpInterface(const SNLDB* snlDB, const std::filesystem::path& interfacePath) {
  ::capnp::MallocMessageBuilder message;

  DBInterface::Builder db = message.initRoot<DBInterface>();
  db.setId(snlDB->getID());
  auto lambda = [](DBInterface::Builder& builder, size_t nbProperties) {
    return builder.initProperties(nbProperties);
  };
  dumpProperties(db, snlDB, lambda);

  auto libraries = db.initLibraryInterfaces(snlDB->getLibraries().size());
  size_t id = 0;
  for (auto snlLibrary: snlDB->getLibraries()) {
    auto libraryInterfaceBuilder = libraries[id++];
    dumpLibraryInterface(libraryInterfaceBuilder, snlLibrary);
  }

  if (auto topDesign = snlDB->getTopDesign()) {
    auto designReference = topDesign->getReference();
    auto designReferenceBuilder = db.initTopDesignReference();
    designReferenceBuilder.setDbID(designReference.dbID_);
    designReferenceBuilder.setLibraryID(designReference.getDBDesignReference().libraryID_);
    designReferenceBuilder.setDesignID(designReference.getDBDesignReference().designID_);
  }

  int fd = open(
    interfacePath.c_str(),
    O_CREAT | O_WRONLY,
    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  writePackedMessageToFd(fd, message);
  close(fd);
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
  if (dbInterface.hasProperties()) {
    auto lambda = [](const DBInterface::Reader& reader) {
      return reader.getProperties();
    };
    loadProperties(dbInterface, snldb, lambda);
  }
  
  if (dbInterface.hasLibraryInterfaces()) {
    for (auto libraryInterface: dbInterface.getLibraryInterfaces()) {
      loadLibraryInterface(snldb, libraryInterface);
    }
  }
  if (dbInterface.hasTopDesignReference()) {
    auto designReference = dbInterface.getTopDesignReference();
    auto snlDesignReference =
      SNLID::DesignReference(
        designReference.getDbID(),
        designReference.getLibraryID(),
        designReference.getDesignID());
    auto topDesign = SNLUniverse::get()->getDesign(snlDesignReference);
    if (not topDesign) {
      //LCOV_EXCL_START
      std::ostringstream reason;
      reason << "cannot deserialize top design: no design found with provided reference";
      throw SNLException(reason.str());
      //LCOV_EXCL_STOP
    }
    snldb->setTopDesign(topDesign);
  }
  return snldb;
}

}} // namespace SNL // namespace naja
