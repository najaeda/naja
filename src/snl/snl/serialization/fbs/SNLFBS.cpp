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

using namespace naja::SNL;

TermDirection SNLtoFBSDirection(SNLTerm::Direction direction) {
  switch (direction) {
    case SNLTerm::Direction::Input:
      return TermDirection_Input;
    case SNLTerm::Direction::Output:
      return TermDirection_Output;
    case SNLTerm::Direction::InOut:
      return TermDirection_InOut;
  }
  return TermDirection_Input;
}

flatbuffers::Offset<void> dumpScalarTerm(
  flatbuffers::FlatBufferBuilder& builder,
  const SNLScalarTerm* scalarTerm) {

  flatbuffers::Offset<flatbuffers::String> name; 
  if (not scalarTerm->isAnonymous()) {
    name = builder.CreateSharedString(scalarTerm->getName().getString());
  }

  ScalarTermBuilder termBuilder(builder);
  termBuilder.add_id(scalarTerm->getID());
  if (not scalarTerm->isAnonymous()) {
    termBuilder.add_name(name);
  }
  termBuilder.add_direction(SNLtoFBSDirection(scalarTerm->getDirection()));
  auto fbTerm = termBuilder.Finish();
  return fbTerm.Union();
}

flatbuffers::Offset<void> dumpBusTerm(
  flatbuffers::FlatBufferBuilder& builder,
  const SNLBusTerm* busTerm) {

  flatbuffers::Offset<flatbuffers::String> name; 
  if (not busTerm->isAnonymous()) {
    name = builder.CreateSharedString(busTerm->getName().getString());
  }

  BusTermBuilder termBuilder(builder);
  termBuilder.add_id(busTerm->getID());
  if (not busTerm->isAnonymous()) {
    termBuilder.add_name(name);
  }
  termBuilder.add_direction(SNLtoFBSDirection(busTerm->getDirection()));
  termBuilder.add_msb(busTerm->getMSB());
  termBuilder.add_lsb(busTerm->getLSB());
  auto fbTerm = termBuilder.Finish();
  return fbTerm.Union();
}

DesignType SNLtoFBSDesignType(SNLDesign::Type type) {
  switch (type) {
    case SNLDesign::Type::Standard:
      return DesignType_Standard;
    case SNLDesign::Type::Primitive:
      return DesignType_Primitive;
    case SNLDesign::Type::Blackbox:
      return DesignType_Blackbox;
  }
  return DesignType_Standard;
}

flatbuffers::Offset<SNL::FBS::DesignInterface> dumpDesignInterface(
  flatbuffers::FlatBufferBuilder& builder,
  const SNLDesign* design) {
  std::vector<uint8_t> termTypesVector;
  std::vector<flatbuffers::Offset<void>> termsVector;

  for (auto term: design->getTerms()) {
    if (auto scalarTerm = dynamic_cast<const SNLScalarTerm*>(term)) {
      termTypesVector.push_back(static_cast<uint8_t>(Term_ScalarTerm));
      termsVector.push_back(dumpScalarTerm(builder, scalarTerm));
    } else {
      auto busTerm = static_cast<SNLBusTerm*>(term);
      termTypesVector.push_back(static_cast<uint8_t>(Term_BusTerm));
      termsVector.push_back(dumpBusTerm(builder, busTerm));
    }
  }
  auto termTypes = builder.CreateVector(termTypesVector);
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
  designInterfaceBuilder.add_terms_type(termTypes);
  designInterfaceBuilder.add_terms(terms);
  designInterfaceBuilder.add_type(SNLtoFBSDesignType(design->getType()));
  auto fbDesign = designInterfaceBuilder.Finish();
  return fbDesign;
}

LibraryType SNLtoFBSLibraryType(SNLLibrary::Type type) {
  switch (type) {
    case SNLLibrary::Type::Standard:
      return LibraryType_Standard;
    case SNLLibrary::Type::Primitives:
      return LibraryType_Primitives;
    case SNLLibrary::Type::InDB0:
      //FIXME: ERROR
      return LibraryType_Standard;
  }
  return LibraryType_Standard;
}

SNLLibrary::Type FBStoSNLLibraryType(LibraryType type) {
  switch (type) {
    case LibraryType_Standard:
      return SNLLibrary::Type::Standard;
    case LibraryType_Primitives:
      return  SNLLibrary::Type::Primitives;
  }
  return SNLLibrary::Type::Standard;
}

flatbuffers::Offset<SNL::FBS::LibraryInterface> dumpLibraryInterface(
  flatbuffers::FlatBufferBuilder& builder,
  const SNLLibrary* library) {
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

SNLDesign::Type FBStoSNLDesignType(DesignType type) {
  switch (type) {
    case DesignType_Standard:
      return SNLDesign::Type::Standard;
    case DesignType_Blackbox:
      return SNLDesign::Type::Blackbox;
    case DesignType_Primitive:
      return SNLDesign::Type::Primitive;
  }
  return SNLDesign::Type::Standard;
}

SNLTerm::Direction FBStoSNLDirection(TermDirection direction) {
  switch (direction) {
    case TermDirection_Input:
      return SNLTerm::Direction::Input;
    case TermDirection_Output:
      return SNLTerm::Direction::Output;
    case TermDirection_InOut:
      return SNLTerm::Direction::InOut;
  }
  return SNLTerm::Direction::Input;
}

void loadDesignBusTerm(SNLDesign* design, const SNL::FBS::BusTerm* term) {
  auto termID = term->id();
  auto termName = term->name();
  auto termDirection = term->direction();
  auto termMSB = term->msb();
  auto termLSB = term->lsb();
  SNLName snlName;
  if (termName) {
    snlName = SNLName(termName->c_str());
  }
  SNLBusTerm::create(design, SNLID::DesignObjectID(termID), FBStoSNLDirection(termDirection), termMSB, termLSB, snlName);
}

void loadDesignScalarTerm(SNLDesign* design, const SNL::FBS::ScalarTerm* term) {
  auto termID = term->id();
  auto termName = term->name();
  auto termDirection = term->direction();
  SNLName snlName;
  if (termName) {
    snlName = SNLName(termName->c_str());
  }
  SNLScalarTerm::create(design, SNLID::DesignObjectID(termID), FBStoSNLDirection(termDirection), snlName);
}

void loadDesignParameter(SNLDesign* design, const SNL::FBS::Parameter* parameter) {
}

void loadDesignInterface(SNLLibrary* library, const SNL::FBS::DesignInterface* design) {
  auto designID = design->id();
  auto designName = design->name();
  auto designType = design->type();
  SNLName snlName;
  if (designName) {
    snlName = SNLName(designName->c_str());
  }
  auto snlDesign = SNLDesign::create(library, SNLID::DesignID(designID), FBStoSNLDesignType(designType), snlName);
  auto parameters = design->parameters();
  if (parameters) {
    for (auto i=0; i<parameters->size(); ++i) {
      auto parameter = parameters->Get(i);
      loadDesignParameter(snlDesign, parameter);
    } 
  }
  auto terms = design->terms();
  if (terms) {
    assert(design->terms_type() and design->terms_type()->size() == terms->size());
    for (auto i=0; i<terms->size(); ++i) {
      auto termsType = design->terms_type()->Get(i);
      auto term = terms->Get(i);
      if (termsType == Term_ScalarTerm) {
        loadDesignScalarTerm(snlDesign, static_cast<const SNL::FBS::ScalarTerm*>(term));
      } else if (termsType == Term_BusTerm) {
        loadDesignBusTerm(snlDesign, static_cast<const SNL::FBS::BusTerm*>(term));
      } else {
        //FIXME ERROR
      }
    } 
  }
}

void loadLibraryInterface(SNLDB* db, const SNL::FBS::LibraryInterface* library) {
  auto libraryID = library->id();
  auto libraryName = library->name();
  auto libraryType = library->type();
  SNLLibrary* snlLibrary = nullptr;
  if (libraryName) {
    snlLibrary =
      SNLLibrary::create(db, SNLID::LibraryID(libraryID), FBStoSNLLibraryType(libraryType), SNLName(libraryName->c_str()));
  } else {
    snlLibrary =
      SNLLibrary::create(db, SNLID::LibraryID(libraryID), FBStoSNLLibraryType(libraryType));
  }
  auto designs = library->design_interfaces();
  if (designs) {
    for (auto i=0; i<designs->size(); ++i) {
      auto design = designs->Get(i);
      loadDesignInterface(snlLibrary, design);
    } 
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