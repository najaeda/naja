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

#include "snl_generated.h"
using namespace SNL::FBS;

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

  auto name = builder.CreateSharedString(scalarTerm->getName().getString());

  ScalarTermBuilder termBuilder(builder);
  termBuilder.add_name(name);
  termBuilder.add_direction(SNLtoFBSDirection(scalarTerm->getDirection()));
  auto fbTerm = termBuilder.Finish();
  return fbTerm.Union();
}

flatbuffers::Offset<void> dumpBusTerm(
  flatbuffers::FlatBufferBuilder& builder,
  const naja::SNL::SNLBusTerm* busTerm) {

  auto name = builder.CreateSharedString(busTerm->getName().getString());

  BusTermBuilder termBuilder(builder);
  termBuilder.add_name(name);
  termBuilder.add_direction(SNLtoFBSDirection(busTerm->getDirection()));
  auto fbTerm = termBuilder.Finish();
  return fbTerm.Union();
}

flatbuffers::Offset<SNL::FBS::Design> dumpDesign(
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
  
  auto name = builder.CreateSharedString(design->getName().getString());
  DesignBuilder designBuilder(builder);
  designBuilder.add_id(design->getID());
  designBuilder.add_name(name);
  auto fbDesign = designBuilder.Finish();
  return fbDesign;
}

flatbuffers::Offset<SNL::FBS::Library> dumpLibrary(
  flatbuffers::FlatBufferBuilder& builder,
  const naja::SNL::SNLLibrary* library) {
  std::vector<flatbuffers::Offset<Library>> librariesVector;
  for (auto lib: library->getLibraries()) {
    librariesVector.push_back(dumpLibrary(builder, lib));
  }
  auto libraries = builder.CreateVector(librariesVector);

  std::vector<flatbuffers::Offset<Design>> designsVector;
  for (auto design: library->getDesigns()) {
    designsVector.push_back(dumpDesign(builder, design));
  }
  auto designs = builder.CreateVector(designsVector);
  auto name = builder.CreateString(library->getName().getString());

  LibraryBuilder libBuilder(builder);
  libBuilder.add_id(library->getID());
  libBuilder.add_name(name);
  libBuilder.add_libraries(libraries);
  libBuilder.add_designs(designs);
  
  //libBuilder.add_type();
  auto fbLib = libBuilder.Finish();
  return fbLib;
}

}

namespace naja { namespace SNL {


void SNLFBS::dump(const SNLDB* db) {
  flatbuffers::FlatBufferBuilder builder(1024);

  std::vector<flatbuffers::Offset<Library>> librariesVector;
  for (auto lib: db->getLibraries()) {
    dumpLibrary(builder, lib);
    librariesVector.push_back(dumpLibrary(builder, lib));
  }
  auto libraries = builder.CreateVector(librariesVector);

  DBBuilder dbBuilder(builder);
  dbBuilder.add_id(db->getID());
  dbBuilder.add_libraries(libraries);

  auto fdb = dbBuilder.Finish();
  builder.Finish(fdb);

  uint8_t* buf = builder.GetBufferPointer();
  std::ofstream wf("snl.fbs", std::ios::out | std::ios::binary);
  wf.write((char*)buf, builder.GetSize());
  wf.close();
}

}} // namespace SNL // namespace naja