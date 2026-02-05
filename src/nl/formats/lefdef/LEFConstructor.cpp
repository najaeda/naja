// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "LEFConstructor.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <memory>
#include <sstream>

#include "NajaLog.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "NLName.h"
#include "NLUniverse.h"
#include "PNLTechnology.h"
#include "PNLBox.h"
#include "PNLDesign.h"
#include "PNLNet.h"
#include "PNLPoint.h"
#include "PNLSite.h"
#include "PNLTerm.h"
#include "lefrReader.hpp"

using namespace naja::NL;

namespace {

void logFunction_(const char* message) {
  if (message) {
    NAJA_LOG_INFO("{}", message);
  }
}

void pinStdPostProcess_() {}

void pinPadPostProcess_() {}

void toUpperInPlace_(std::string& value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
}

int unitsCbk_(lefrCallbackType_e c, lefiUnits* units, lefiUserData ud) {
  LEFConstructor* parser = (LEFConstructor*)ud;

  if (units->hasDatabase()) {
    parser->setUnitsMicrons(1.0 / units->databaseNumber());
    NAJA_LOG_INFO("     - Precision: {} (LEF MICRONS scale factor:{})",
                  parser->getUnitsMicrons(), units->databaseNumber());
  }
  return 0;
}

int layerCbk_(lefrCallbackType_e c, lefiLayer* lefLayer, lefiUserData ud) {
  return 0;
}

int siteCbk_(lefrCallbackType_e c, lefiSite* site, lefiUserData ud) {
  LEFConstructor* parser = (LEFConstructor*)ud;
  PNLSite::ClassType siteClass = PNLSite::ClassType::Unknown;
  if (site->hasClass()) {
    std::string classType = site->siteClass();
    toUpperInPlace_(classType);
    if (classType == "CORE") {
      siteClass = PNLSite::ClassType::Core;
    } else if (classType == "PAD") {
      siteClass = PNLSite::ClassType::Pad;
    } else {
      siteClass = PNLSite::ClassType::Unknown;
    }
  }
  PNLBox::Unit lefSiteWidth =
      site->sizeX();  // PNLBox::fromPhysical( site->sizeX(), PNLBox::Micro
                      // );
  PNLBox::Unit lefSiteHeight =
      site->sizeY();  // PNLBox::fromPhysical( site->sizeY(), PNLBox::Micro
  auto universe = NLUniverse::get();
  auto tech = universe->getTechnology();
  if (not tech) {
    tech = PNLTechnology::create(universe);
  }
  auto pnlSite = PNLSite::create(tech, NLName(site->name()), siteClass, lefSiteWidth,
                                 lefSiteHeight);
  if (site->hasXSymmetry() && site->hasYSymmetry()) {
    pnlSite->setSymmetry(PNLSite::Symmetry::X_Y);
  } else if (site->hasXSymmetry()) {
    pnlSite->setSymmetry(PNLSite::Symmetry::X);
  } else if (site->hasYSymmetry()) {
    pnlSite->setSymmetry(PNLSite::Symmetry::Y);
  } else if (site->has90Symmetry()) {
    pnlSite->setSymmetry(PNLSite::Symmetry::R90);
  }
  return 0;
}

int macroForeignCbk_(lefrCallbackType_e c,
                     const lefiMacroForeign* foreign,
                     lefiUserData ud) {
  LEFConstructor* parser = (LEFConstructor*)ud;

  bool created = false;
  PNLDesign* cell = parser->earlyGetPNLDesign(created, foreign->cellName());
  cell->setClassType(PNLDesign::ClassType::CORE);  // TODO:: Correct?

  cell->setTerminalNetlist(true);
  if (created) {
    if (LEFConstructor::getGdsForeignDirectory().empty()) {
      return 0;
    }

    string gdsPath = LEFConstructor::getGdsForeignDirectory() + "/" +
                     foreign->cellName() + ".gds";
    parser->setForeignPath(gdsPath);
  }

  // parser->setForeignPosition( PNLPoint( parser->fromUnitsMicrons(
  // foreign->px() )
  //                                  , parser->fromUnitsMicrons( foreign->px()
  //                                  )));
  parser->setForeignPosition(PNLPoint(foreign->px(), foreign->py()));

  for (PNLNet* net : cell->getNets()) {
    PNLBitNet* bitNet = static_cast<PNLBitNet*>(net);
    if (bitNet->isVDD())
      parser->setGdsPower(bitNet);
    if (bitNet->isGND())
      parser->setGdsGround(bitNet);
    // if (parser->getForeignPosition() != PNLPoint(0,0)) {
    //   for ( PNLNetComponent* component : bitNet->getComponents() ) {
    //     PNLTerm* term = static_cast<PNLTerm*>(component);
    //     term->translate( parser->getForeignPosition().getX()
    //                         , parser->getForeignPosition().getY() );
    //   }
    // }
  }

  return 0;
}

int obstructionCbk_(lefrCallbackType_e c,
                    lefiObstruction* obstruction,
                    lefiUserData ud) {
  return 0;
}

int macroCbk_(lefrCallbackType_e c, lefiMacro* macro, lefiUserData ud) {
  LEFConstructor* parser = (LEFConstructor*)ud;

  bool created = false;
  string cellName = macro->name();
  PNLBox::Unit width = 0;
  PNLBox::Unit height = 0;
  PNLDesign* cell = parser->earlyGetPNLDesign(created, cellName);

  if (cell->getName() != NLName(cellName)) {
    printf("cell name %s\n", cellName.c_str());
    cell->setName(NLName(cellName));
  }

  if (macro->hasSize()) {
    width = macro->sizeX();   // parser->fromUnitsMicrons( macro->sizeX() );
    height = macro->sizeY();  // parser->fromUnitsMicrons( macro->sizeY() );
    cell->setAbutmentBox(PNLBox(0, 0, width, height));
  }

  // Initialize cell type based on macro->macroClass with switch case
  std::string macroClass = macro->macroClass();
  assert(macro->hasClass());

  std::stringstream ss(macroClass);  // Create a stringstream object
  std::string word;
  std::vector<std::string> substrings;

  // Extract substrings separated by spaces
  while (ss >> word) {
    substrings.push_back(word);
  }

  if (substrings[0] == "CORE") {
    if (substrings.size() > 1) {
      if (substrings[1] == "FEEDTHRU") {
        cell->setClassType(PNLDesign::ClassType::CORE_FEEDTHRU);
      } else if (substrings[1] == "TIEHIGH") {
        cell->setClassType(PNLDesign::ClassType::CORE_TIEHIGH);
      } else if (substrings[1] == "TIELOW") {
        cell->setClassType(PNLDesign::ClassType::CORE_TIELOW);
      } else if (substrings[1] == "SPACER") {
        cell->setClassType(PNLDesign::ClassType::CORE_SPACER);
      } else if (substrings[1] == "ANTENNACELL") {
        cell->setClassType(PNLDesign::ClassType::CORE_ANTENNACELL);
      } else if (substrings[1] == "WELLTAP") {
        cell->setClassType(PNLDesign::ClassType::CORE_WELLTAP);
      } else {
        assert(false);
      }
    } else {
      cell->setClassType(PNLDesign::ClassType::CORE);
    }
  } else if (substrings[0] == "PAD") {
    // PAD, PAD_INPUT, PAD_OUTPUT, PAD_INOUT, PAD_POWER, PAD_SPACER, PAD_AREAIO,
    if (substrings.size() > 1) {
      if (substrings[1] == "INPUT") {
        cell->setClassType(PNLDesign::ClassType::PAD_INPUT);
      } else if (substrings[1] == "OUTPUT") {
        cell->setClassType(PNLDesign::ClassType::PAD_OUTPUT);
      } else if (substrings[1] == "INOUT") {
        cell->setClassType(PNLDesign::ClassType::PAD_INOUT);
      } else if (substrings[1] == "POWER") {
        cell->setClassType(PNLDesign::ClassType::PAD_POWER);
      } else if (substrings[1] == "SPACER") {
        cell->setClassType(PNLDesign::ClassType::PAD_SPACER);
      } else if (substrings[1] == "AREAIO") {
        cell->setClassType(PNLDesign::ClassType::PAD_AREAIO);
      } else {
        assert(false);
      }
    } else {
      cell->setClassType(PNLDesign::ClassType::PAD);
    }
  } else if (substrings[0] == "BLOCK") {
    cell->setClassType(PNLDesign::ClassType::BLOCK);
  } else if (substrings[0] == "BLACKBOX") {
    cell->setClassType(PNLDesign::ClassType::BLACKBOX);
  } else if (substrings[0] == "SOFT MACRO") {
    cell->setClassType(PNLDesign::ClassType::SOFT_MACRO);
  } else if (substrings[0] == "ENDCAP") {
    if (substrings[1] == "PRE") {
      cell->setClassType(PNLDesign::ClassType::ENDCAP_PRE);
    } else if (substrings[1] == "POST") {
      cell->setClassType(PNLDesign::ClassType::ENDCAP_POST);
    } else if (substrings[1] == "TOPRIGHT") {
      cell->setClassType(PNLDesign::ClassType::ENDCAP_TOPRIGHT);
    } else if (substrings[1] == "TOPLEFT") {
      cell->setClassType(PNLDesign::ClassType::ENDCAP_TOPLEFT);
    } else if (substrings[1] == "BOTTOMRIGHT") {
      cell->setClassType(PNLDesign::ClassType::ENDCAP_BOTTOMRIGHT);
    } else if (substrings[1] == "BOTTOMLEFT") {
      cell->setClassType(PNLDesign::ClassType::ENDCAP_BOTTOMLEFT);
    } else {
      assert(false);  // Handle unknown endcap type
    }
  } else if (substrings[0] == "COVER") {
    if (substrings.size() > 1) {
      if (substrings[1] == "BUMP") {
        cell->setClassType(PNLDesign::ClassType::COVER_BUMP);
      } else {
        assert(false);
      }
    } else {
      cell->setClassType(PNLDesign::ClassType::COVER);
    }
  } else if (substrings[0] == "RING") {
    cell->setClassType(PNLDesign::ClassType::RING);
  } else {
    assert(false);  // Handle unknown macro class
  }
  printf("name %s original type %s type %s has class %d\n",
         cell->getName().getString().c_str(), macro->macroClass(),
         cell->getClassType().getString().c_str(), macro->hasClass());

  bool isPad = false;
  string gaugeName = "Unknown SITE";
  if (macro->hasSiteName()) {
    std::string siteName = macro->siteName();
    auto universe = NLUniverse::get();
    auto tech = universe->getTechnology();
    if (not tech) {
      tech = PNLTechnology::create(universe);
    }
    PNLSite* site = tech->getSiteByName(NLName(siteName));
    cell->setSite(site);
    if (site->getClass() == PNLSite::ClassType::Pad) {
      isPad = true;
    }
  }

  if (not isPad)
    pinStdPostProcess_();
  else
    pinPadPostProcess_();
  parser->clearPinComponents();
  //if (isPad) {
  //  cerr << " (PAD)";
  //}
  //cerr << endl;
  cell->setTerminalNetlist(true);
  parser->setPNLDesign(nullptr);
  parser->setGdsPower(nullptr);
  parser->setGdsGround(nullptr);

  return 0;
}

int viaCbk_(lefrCallbackType_e type, lefiVia* via, lefiUserData) {
  return 0;
}
int manufacturingCB_(lefrCallbackType_e /* unused: c */,
                     double num,
                     lefiUserData ud) {
  auto universe = NLUniverse::get();
  auto tech = universe->getTechnology();
  if (not tech) {
    tech = PNLTechnology::create(universe);
  }
  tech->setManufacturingGrid(num);
  return 0;
}

int macroSiteCbk_(lefrCallbackType_e c,
                  const lefiMacroSite* site,
                  lefiUserData ud) {
  return 0;
}

int pinCbk_(lefrCallbackType_e c, lefiPin* pin, lefiUserData ud) {
  LEFConstructor* parser = (LEFConstructor*)ud;

  // cerr << "       @ pinCbk_: " << pin->name() << endl;

  bool created = false;
  parser->earlyGetPNLDesign(created);

  PNLNet* net = nullptr;
  PNLTerm* term = nullptr;
  PNLNet::Type netType = PNLNet::Type::TypeEnum::Undefined;
  if (pin->hasUse()) {
    string lefUse = pin->use();
    toUpperInPlace_(lefUse);

    if (lefUse == "SIGNAL") {
      netType = PNLNet::Type::TypeEnum::Logical;
    } else if (lefUse == "POWER") {
      netType = PNLNet::Type::TypeEnum::VDD;
    } else if (lefUse == "GROUND") {
      netType = PNLNet::Type::TypeEnum::GND;
    } else if (lefUse == "CLOCK") {
      netType = PNLNet::Type::TypeEnum::Clock;
    } else if (lefUse == "ANALOG") {
      netType = PNLNet::Type::TypeEnum::Analog;
    }
  }

  if ((netType == PNLNet::Type::TypeEnum::VDD) and parser->getGdsPower()) {
    net = parser->getGdsPower();
    // cerr << "       - Renaming GDS power net \"" << net->getName() << "\""
    //      << " to LEF name \"" << pin->name() << "\"." << endl;
    net->setName(NLName(pin->name()));
    parser->setGdsPower(nullptr);
  } else {
    if ((netType == PNLNet::Type::TypeEnum::GND) and parser->getGdsGround()) {
      net = parser->getGdsGround();
      // cerr << "       - Renaming GDS ground net \"" << net->getName() << "\""
      //      << " to LEF name \"" << pin->name() << "\"." << endl;
      net->setName(NLName(pin->name()));
      parser->setGdsGround(nullptr);
    } else {
      net = parser->earlygetNet(pin->name());
      term = parser->earlygetTerm(pin->name());
    }
  }
  net->setExternal(true);
  net->setType(netType);

  if (pin->hasDirection()) {
    string lefDir = pin->direction();
    toUpperInPlace_(lefDir);

    if (lefDir == "INPUT")
      term->setDirection(PNLNetComponent::Direction::Input);
    if (lefDir == "OUTPUT")
      term->setDirection(PNLNetComponent::Direction::Output);
    if (lefDir == "OUTPUT TRISTATE")
      term->setDirection(PNLNetComponent::Direction::Tristate);
    if (lefDir == "INOUT")
      term->setDirection(PNLNetComponent::Direction::InOut);
  }
  if (net->isSupply())
    net->setGlobal(true);
  if (pin->name()[strlen(pin->name()) - 1] == '!')
    net->setGlobal(true);
  return 0;
}

}  // namespace

inline string LEFConstructor::getLibraryName() const {
  return libraryName_;
}
inline NLLibrary* LEFConstructor::getLibrary(bool create) {
  if (not library_ and create)
    createNLLibrary();
  return library_;
}
inline PNLDesign* LEFConstructor::getPNLDesign() const {
  return cell_;
}
inline void LEFConstructor::setPNLDesign(PNLDesign* cell) {
  cell_ = cell;
}
inline string LEFConstructor::getForeignPath() const {
  return foreignPath_;
}
inline void LEFConstructor::setForeignPath(string path) {
  foreignPath_ = path;
}
inline const PNLPoint& LEFConstructor::getForeignPosition() const {
  return foreignPosition_;
}
inline void LEFConstructor::setForeignPosition(const PNLPoint& position) {
  foreignPosition_ = position;
}
inline PNLNet* LEFConstructor::getGdsPower() const {
  return gdsPower_;
}
inline void LEFConstructor::setGdsPower(PNLNet* net) {
  gdsPower_ = net;
}
inline PNLNet* LEFConstructor::getGdsGround() const {
  return gdsGround_;
}
inline void LEFConstructor::setGdsGround(PNLNet* net) {
  gdsGround_ = net;
}
inline PNLNet* LEFConstructor::getNet() const {
  return net_;
}
inline void LEFConstructor::setPNLNet(PNLNet* net) {
  net_ = net;
}
inline double LEFConstructor::getUnitsMicrons() const {
  return unitsMicrons_;
}
inline void LEFConstructor::setUnitsMicrons(double precision) {
  unitsMicrons_ = precision;
}
inline bool LEFConstructor::hasErrors() const {
  return not errors_.empty();
}
inline const vector<string>& LEFConstructor::getErrors() const {
  return errors_;
}
inline void LEFConstructor::pushError(const string& error) {
  errors_.push_back(error);
}
inline void LEFConstructor::clearErrors() {
  return errors_.clear();
}
inline void LEFConstructor::addPinComponent(string name, PNLTerm* comp) {
  pinComponents_[name].push_back(comp);
}
inline void LEFConstructor::clearPinComponents() {
  pinComponents_.clear();
}

string LEFConstructor::gdsForeignDirectory_ = "";
NLLibrary* LEFConstructor::mergeNLLibrary_ = nullptr;

void LEFConstructor::setMergeLibrary(NLLibrary* library) {
  mergeNLLibrary_ = library;
}

void LEFConstructor::setGdsForeignDirectory(string path) {
  gdsForeignDirectory_ = path;
}

LEFConstructor::LEFConstructor(string file, string libraryName)
    : file_(file),
      libraryName_(libraryName),
      library_(nullptr),
      foreignPath_(),
      foreignPosition_(PNLPoint(0, 0)),
      gdsPower_(nullptr),
      gdsGround_(nullptr),
      cell_(nullptr),
      net_(nullptr),
      busBits_("()"),
      unitsMicrons_(0.01),
      errors_() {
  lefrSetLogFunction(logFunction_);
  lefrInit();
  lefrSetUnitsCbk(unitsCbk_);
  lefrSetLayerCbk(layerCbk_);
  lefrSetSiteCbk(siteCbk_);
  lefrSetObstructionCbk(obstructionCbk_);
  lefrSetMacroCbk(macroCbk_);
  lefrSetMacroSiteCbk(macroSiteCbk_);
  lefrSetMacroForeignCbk(macroForeignCbk_);
  lefrSetPinCbk(pinCbk_);
  lefrSetViaCbk(viaCbk_);
  lefrSetManufacturingCbk(manufacturingCB_);
}

LEFConstructor::~LEFConstructor() {
  lefrReset();
}

NLLibrary* LEFConstructor::createNLLibrary() {
  if (mergeNLLibrary_) {
    library_ = mergeNLLibrary_;
    return library_;
  }
  db_ = NLDB::create(NLUniverse::get());
  NLLibrary* rootNLLibrary = NLLibrary::create(db_, NLName("LIB1"));

  NLLibrary* lefRootNLLibrary = NLLibrary::create(rootNLLibrary, NLName("LEF"));

  library_ = lefRootNLLibrary->getLibrary(NLName(libraryName_));
  if (library_) {
    assert(false);
    // Error
  } else {
    library_ = NLLibrary::create(lefRootNLLibrary, NLName(libraryName_));
  }
  return library_;
}

PNLDesign* LEFConstructor::earlyGetPNLDesign(bool& created, string name) {
  if (not cell_) {
    if (name.empty())
      name = "EarlyLEFPNLDesign";
    cell_ = getLibrary(true)->getPNLDesign(NLName(name));
    if (not cell_) {
      created = true;
      cell_ = PNLDesign::create(getLibrary(true), NLName(name));
    }
  }
  return cell_;
}

PNLNet* LEFConstructor::earlygetNet(string name) {
  bool created = false;
  if (not cell_)
    earlyGetPNLDesign(created);
  PNLNet* net = cell_->getNet(NLName(name));
  if (not net)
    net = cell_->addNet(NLName(name));
  return net;
}

PNLTerm* LEFConstructor::earlygetTerm(string name) {
  bool created = false;
  if (not cell_)
    earlyGetPNLDesign(created);
  PNLTerm* term = cell_->getTerm(NLName(name));
  if (not term)
    term = cell_->addTerm(NLName(name));
  return term;
}

int LEFConstructor::flushErrors() {
  int code = (hasErrors()) ? 1 : 0;

  for (size_t ierror = 0; ierror < errors_.size(); ++ierror) {
    // string message = "LefImport::construct(): " + errors_[ierror];
    // cerr << Error(message.c_str(),getString(library_->getName()).c_str()) <<
    // endl;
    assert(false);
  }
  clearErrors();

  return code;
}

NLLibrary* LEFConstructor::parse(string file) {
  size_t iext = file.rfind('.');
  if (file.compare(iext, 4, ".lef") != 0) {
    // throw Error( "LefImport::construct(): DEF files must have  \".lef\"
    // extension
    // <%s>.", file.c_str() );
    assert(false);
  }

  size_t islash = file.rfind('/');
  islash = (islash == string::npos) ? 0 : islash + 1;

  // string                libraryName = file.substr( islash,
  // file.size()-4-islash );
  string libraryName = "LoadedLibrary";
  unique_ptr<LEFConstructor> parser(new LEFConstructor(file, libraryName));
  // std::ifstream f("file.lef");
  // if (f.is_open())
  //     std::cout << f.rdbuf();

  FILE* lefStream = fopen(file.c_str(), "r");

  if (not lefStream) {
    // throw Error( "LefImport::construct(): Cannot open LEF file \"%s\".",
    // file.c_str() );
    assert(false);
  }
  parser->createNLLibrary();
  lefrRead(lefStream, file.c_str(), (lefiUserData)parser.get());
  fclose(lefStream);
  return parser->getLibrary();
}

NLLibrary* LEFConstructor::construct(string fileName) {
  NLLibrary* library = NULL;
  library = LEFConstructor::parse(fileName);
  return library;
}
