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

#include "SNLFBS.h"

#include <fstream>

#include "snl_interface_generated.h"
using namespace SNL::FBS;

#include "SNLUniverse.h"
#include "SNLDB.h"
#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"

namespace {

TermDirection SNLtoFBSDirection(naja::SNL::SNLTerm::Direction direction) {
  switch (direction) {
    case naja::SNL::SNLTerm::Direction::Input:
      return TermDirection_Input;
    case naja::SNL::SNLTerm::Direction::Output:
      return TermDirection_Output;
    case naja::SNL::SNLTerm::Direction::InOut:
      return TermDirection_InOut;
  }
  return TermDirection_Input;
}

flatbuffers::Offset<void> dumpScalarTerm(
  flatbuffers::FlatBufferBuilder& builder,
  const naja::SNL::SNLScalarTerm* scalarTerm) {

  flatbuffers::Offset<flatbuffers::String> name; 
  if (not scalarTerm->isAnonymous()) {
    name = builder.CreateSharedString(scalarTerm->getName().getString());
  }

  ScalarTermBuilder termBuilder(builder);
  if (not scalarTerm->isAnonymous()) {
    termBuilder.add_name(name);
  }
  termBuilder.add_direction(SNLtoFBSDirection(scalarTerm->getDirection()));
  auto fbTerm = termBuilder.Finish();
  return fbTerm.Union();
}

flatbuffers::Offset<void> dumpBusTerm(
  flatbuffers::FlatBufferBuilder& builder,
  const naja::SNL::SNLBusTerm* busTerm) {

  flatbuffers::Offset<flatbuffers::String> name; 
  if (not busTerm->isAnonymous()) {
    name = builder.CreateSharedString(busTerm->getName().getString());
  }

  BusTermBuilder termBuilder(builder);
  if (not busTerm->isAnonymous()) {
    termBuilder.add_name(name);
  }
  termBuilder.add_direction(SNLtoFBSDirection(busTerm->getDirection()));
  termBuilder.add_msb(busTerm->getMSB());
  termBuilder.add_lsb(busTerm->getLSB());
  auto fbTerm = termBuilder.Finish();
  return fbTerm.Union();
}

DesignType SNLtoFBSDesignType(naja::SNL::SNLDesign::Type type) {
  switch (type) {
    case naja::SNL::SNLDesign::Type::Standard:
      return DesignType_Standard;
    case naja::SNL::SNLDesign::Type::Primitive:
      return DesignType_Primitive;
    case naja::SNL::SNLDesign::Type::Blackbox:
      return DesignType_Blackbox;
  }
  return DesignType_Standard;
}

flatbuffers::Offset<SNL::FBS::DesignInterface> dumpDesignInterface(
  flatbuffers::FlatBufferBuilder& builder,
  const naja::SNL::SNLDesign* design) {
  std::vector<flatbuffers::Offset<void>> termsVector;
  for (auto term: design->getTerms()) {
    if (auto scalarTerm = dynamic_cast<const naja::SNL::SNLScalarTerm*>(term)) {
      termsVector.push_back(dumpScalarTerm(builder, scalarTerm));
    } else {
      auto busTerm = static_cast<naja::SNL::SNLBusTerm*>(term);
      termsVector.push_back(dumpBusTerm(builder, busTerm));
    }
  }
  auto terms = builder.CreateVector(termsVector);
  
  flatbuffers::Offset<flatbuffers::String> name; 
  if (not design->isAnonymous()) {
    name = builder.CreateSharedString(design->getName().getString());
  }
  DesignInterfaceBuilder designInterfaceBuilder(builder);
  designInterfaceBuilder.add_id(design->getID());
  if (not design->isAnonymous()) {
    
    designInterfaceBuilder.add_name(name);
  }
  designInterfaceBuilder.add_type(SNLtoFBSDesignType(design->getType()));
  auto fbDesign = designInterfaceBuilder.Finish();
  return fbDesign;
}

LibraryType SNLtoFBSLibraryType(naja::SNL::SNLLibrary::Type type) {
  switch (type) {
    case naja::SNL::SNLLibrary::Type::Standard:
      return LibraryType_Standard;
    case naja::SNL::SNLLibrary::Type::Primitives:
      return LibraryType_Primitives;
    case naja::SNL::SNLLibrary::Type::InDB0:
      //FIXME: ERROR
      return LibraryType_Standard;
  }
  return LibraryType_Standard;
}

naja::SNL::SNLLibrary::Type FBStoSNLLibraryType(LibraryType type) {
  switch (type) {
    case LibraryType_Standard:
      return naja::SNL::SNLLibrary::Type::Standard;
    case LibraryType_Primitives:
      return  naja::SNL::SNLLibrary::Type::Primitives;
  }
  return naja::SNL::SNLLibrary::Type::Standard;
}

flatbuffers::Offset<SNL::FBS::LibraryInterface> dumpLibraryInterface(
  flatbuffers::FlatBufferBuilder& builder,
  const naja::SNL::SNLLibrary* library) {
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
}

void loadLibraryInterface(naja::SNL::SNLDB* db, const SNL::FBS::LibraryInterface* library) {
  auto libraryID = library->id();
  auto libraryName = library->name();
  auto libraryType = library->type();
  naja::SNL::SNLLibrary* snlLibrary = nullptr;
  if (libraryName) {
    snlLibrary =
      naja::SNL::SNLLibrary::create(db, libraryID, FBStoSNLLibraryType(libraryType), naja::SNL::SNLName(libraryName->c_str()));
  } else {
    snlLibrary =
      naja::SNL::SNLLibrary::create(db, libraryID, FBStoSNLLibraryType(libraryType));
  }


}

}

namespace naja { namespace SNL {

void SNLFBS::dumpInterface(const SNLDB* db, const std::filesystem::path& interfacePath) {
  flatbuffers::FlatBufferBuilder builder(1024);

  std::vector<flatbuffers::Offset<LibraryInterface>> librariesVector;
  for (auto lib: db->getLibraries()) {
    dumpLibraryInterface(builder, lib);
    librariesVector.push_back(dumpLibraryInterface(builder, lib));
  }
  auto libraries = builder.CreateVector(librariesVector);

  DBInterfaceBuilder dbBuilder(builder);
  dbBuilder.add_id(db->getID());
  dbBuilder.add_library_interfaces(libraries);

  auto fdb = dbBuilder.Finish();
  builder.Finish(fdb);

  uint8_t* buf = builder.GetBufferPointer();
  std::ofstream wf(interfacePath, std::ios::out | std::ios::binary);
  wf.write((char*)buf, builder.GetSize());
  wf.close();
}

void SNLFBS::dump(const SNLDB* db, const std::filesystem::path& path) {
  std::filesystem::create_directory(path);
  dumpInterface(db, path/InterfaceName);
}

SNLDB* SNLFBS::loadInterface(const std::filesystem::path& interfacePath) {
  //FIXME: verify if file can be opened
  std::ifstream rf(interfacePath, std::ios::in | std::ios::binary);
  rf.seekg(0, std::ios::end);
  int length = rf.tellg();
  rf.seekg(0, std::ios::beg);
  char* data = new char[length];
	rf.read(data, length);
	rf.close();

  auto db = GetDBInterface(data);
  auto dbID = db->id();
  auto universe = SNLUniverse::get();
  if (not universe) {
    universe = SNLUniverse::create();
  }
  auto snldb = SNLDB::create(universe, dbID);
  auto libraries = db->library_interfaces();
  if (libraries) {
    for (auto i=0; i<libraries->size(); ++i) {
      auto library = libraries->Get(i);
      loadLibraryInterface(snldb, library);
    }
  }
  return snldb;
}

SNLDB* SNLFBS::load(const std::filesystem::path& path) {
  SNLDB* db = loadInterface(path/InterfaceName);
  return db;
}

}} // namespace SNL // namespace naja