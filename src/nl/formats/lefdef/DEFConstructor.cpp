// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <cstdio>
#include <cstring>
#include <memory>

#include "DEFConstructor.h"
#include "NLDB.h"
#include "NLLibrary.h"
#include "PNLDesign.h"
#include "PNLInstTerm.h"
#include "PNLNet.h"
#include "PNLOrientation.h"
#include "PNLTransform.h"
// for ostringstream
#include <sstream>
#include "NLDB.h"
#include "NLUniverse.h"

#include "defrReader.hpp"
#include "lefrReader.hpp"

using namespace naja::NL;

namespace {

int unitsCbk_(defrCallbackType_e c, double defUnits, lefiUserData ud) {
  DEFConstructor* parser = (DEFConstructor*)ud;
  parser->setUnits(defUnits);
  return 0;
}

int busBitCbk_(defrCallbackType_e c, const char* busbits, lefiUserData ud) {
  DEFConstructor* parser = (DEFConstructor*)ud;

  if (strlen(busbits) == 2) {
    parser->setBusBits(busbits);
  } else {
    // ostringstream message;
    // message << "BUSBITCHARS is not two character long (" << busbits << ")";
    // parser->pushError( message.str() );
    assert(false);
  }

  return 0;
}

int designEndCbk_(defrCallbackType_e c, void*, lefiUserData ud) {
  DEFConstructor* parser = (DEFConstructor*)ud;

  if ((parser->getFlags() & DEFConstructor::FitAbOnDesigns) and
      not parser->getFitOnPNLDesignsDieArea().isEmpty()) {
    parser->getPNLDesign()->setAbutmentBox(parser->getFitOnPNLDesignsDieArea());
  }

  return 0;
}

int dieAreaCbk_(defrCallbackType_e c, defiBox* box, lefiUserData ud) {
  DEFConstructor* parser = (DEFConstructor*)ud;

  parser->getPNLDesign()->setAbutmentBox(
      PNLBox(DEFConstructor::fromDefUnits(box->xl()),
             DEFConstructor::fromDefUnits(box->yl()),
             DEFConstructor::fromDefUnits(box->xh()),
             DEFConstructor::fromDefUnits(box->yh())));

  return 0;
}

int viaCbk_(defrCallbackType_e c, defiVia* via, defiUserData ud) {
  DEFConstructor* parser = (DEFConstructor*)ud;
  string viaName = via->name();

  PNLDesign* viaPNLDesign =
      PNLDesign::create(parser->getPNLDesign()->getLibrary(), NLName(viaName));
  viaPNLDesign->setTerminalNetlist(true);
  if (via->hasViaRule()) {
    char* viaRuleName;
    char* defbotLayer;
    char* defcutLayer;
    char* deftopLayer;
    int defxCutSize = 0;
    int defyCutSize = 0;
    int defxCutSpacing = 0;
    int defyCutSpacing = 0;
    int defxBotEnc = 0;
    int defyBotEnc = 0;
    int defxTopEnc = 0;
    int defyTopEnc = 0;
    int numCutRows = 1;
    int numCutCols = 1;
    via->viaRule(&viaRuleName, &defxCutSize, &defyCutSize, &defbotLayer,
                 &defcutLayer, &deftopLayer, &defxCutSpacing, &defyCutSpacing,
                 &defxBotEnc, &defyBotEnc, &defxTopEnc, &defyTopEnc);
    if (via->hasRowCol())
      via->rowCol(&numCutRows, &numCutCols);
    PNLBox::Unit xCutSize = DEFConstructor::fromDefUnits(defxCutSize);
    PNLBox::Unit yCutSize = DEFConstructor::fromDefUnits(defyCutSize);
    PNLBox::Unit xCutSpacing = DEFConstructor::fromDefUnits(defxCutSpacing);
    PNLBox::Unit yCutSpacing = DEFConstructor::fromDefUnits(defyCutSpacing);
    PNLBox::Unit xBotEnc = DEFConstructor::fromDefUnits(defxBotEnc);
    PNLBox::Unit yBotEnc = DEFConstructor::fromDefUnits(defyBotEnc);
    PNLBox::Unit xTopEnc = DEFConstructor::fromDefUnits(defxTopEnc);
    PNLBox::Unit yTopEnc = DEFConstructor::fromDefUnits(defyTopEnc);
    // Layer*     botLayer    = parser->lookupLayer( defbotLayer );
    // Layer*     cutLayer    = parser->lookupLayer( defcutLayer );
    // Layer*     topLayer    = parser->lookupLayer( deftopLayer );
    PNLNet* net = viaPNLDesign->addNet(NLName("via"));
    PNLBox cellBb;

    PNLBox::Unit halfXSide =
        xTopEnc + (xCutSize * numCutRows + xCutSpacing * (numCutRows - 1)) / 2;
    PNLBox::Unit halfYSide =
        yTopEnc + (xCutSize * numCutCols + xCutSpacing * (numCutRows - 1)) / 2;
    PNLBox padBb = PNLBox(0, 0);
    padBb.increase(halfXSide, halfYSide);
    cellBb.merge(padBb);
    // Pad::create( net, topLayer, padBb );
    halfXSide =
        xBotEnc + (xCutSize * numCutRows + xCutSpacing * (numCutRows - 1)) / 2;
    halfYSide =
        yBotEnc + (xCutSize * numCutCols + xCutSpacing * (numCutRows - 1)) / 2;
    padBb = PNLBox(0, 0);
    padBb.increase(halfXSide, halfYSide);
    cellBb.merge(padBb);
    // Pad::create( net, botLayer, padBb );

    // PNLBox::Unit x = - (xCutSize*numCutRows + xCutSpacing*(numCutRows-1)) /
    // 2; for ( int row=0 ; row<numCutRows ; ++row ) {
    //   PNLBox::Unit y = - (yCutSize*numCutCols + xCutSpacing*(numCutCols-1)) /
    //   2; for ( int col=0 ; col<numCutCols ; ++col ) {
    //     Pad::create( net, cutLayer, PNLBox( x, y, x+xCutSize, y+yCutSize ));
    //     y += yCutSize + yCutSpacing;
    //   }
    //   x += xCutSize + xCutSpacing;
    // }
    viaPNLDesign->setAbutmentBox(cellBb);
  }
  parser->addViaLookup(viaName, viaPNLDesign);

  return 0;
}

int pinCbk_(defrCallbackType_e c, defiPin* pin, lefiUserData ud) {
  return 0;
}

int componentCbk_(defrCallbackType_e c,
                  defiComponent* component,
                  lefiUserData ud) {
  DEFConstructor* parser = (DEFConstructor*)ud;

  string componentName = component->name();
  string componentId = component->id();
  PNLDesign* macro = parser->getLEFMacro(componentName);

  if (macro == NULL) {
    ostringstream message;
    message << "Unknown model/PNLDesign (LEF MACRO) " << componentName
            << " in <%s>.";
    parser->pushError(message.str());
    return 0;
  }

  PNLTransform placement;
  PNLInstance::PlacementStatus state(PNLInstance::PlacementStatus::Unplaced);
  if (component->isPlaced() or component->isFixed()) {
    state = (component->isPlaced()) ? PNLInstance::PlacementStatus::Placed
                                    : PNLInstance::PlacementStatus::Fixed;

    placement = DEFConstructor::getPNLTransform(
        macro->getAbutmentBox(),
        DEFConstructor::fromDefUnits(component->placementX()),
        DEFConstructor::fromDefUnits(component->placementY()),
        DEFConstructor::fromDefOrientation(component->placementOrient()));
  }
  PNLInstance* instance =
      PNLInstance::create(parser->getPNLDesign(), macro, NLName(componentId));
  // , placement
  // , state
  // );
  assert(parser->getPNLDesign()->getInstance(NLName(componentId)));
  instance->setPlacementStatus(state);
  instance->setTransform(placement);
  if (state != PNLInstance::PlacementStatus::Unplaced) {
    parser->mergeToFitOnPNLDesignsDieArea(
        instance->getModel()->getAbutmentBox());
  }
  return 0;
}

int componentEndCbk_(defrCallbackType_e c, void*, lefiUserData ud) {
  DEFConstructor* parser = (DEFConstructor*)ud;
  return parser->flushErrors();
}

int netCbk_(defrCallbackType_e c, defiNet* net, lefiUserData ud) {
  static size_t netCount = 0;

  DEFConstructor* parser = (DEFConstructor*)ud;

  // cerr << "     - PNLNet " << net->name() << endl;

  string name = net->name();
  parser->toHurricaneName(name);

  PNLNetDatas* netDatas = parser->lookupPNLNet(name);
  PNLNet* hnet = NULL;
  if (not netDatas) {
    hnet = parser->getPNLDesign()->addNet(NLName(name));
    parser->addPNLNetLookup(name, hnet);
  } else
    hnet = get<0>(*netDatas);

  // if (parser->getPrebuildPNLNet(false)) {
  //   NLName prebuildAlias = parser->getPrebuildPNLNet()->getName();
  //   hnet->merge( parser->getPrebuildPNLNet() );
  //   hnet->removeAlias( prebuildAlias );
  //   parser->setPrebuildPNLNet( NULL );
  // }

  if (name.size() > 78) {
    name.erase(0, name.size() - 75);
    name.insert(0, 3, '.');
  }
  name.insert(0, "\"");
  name.insert(name.size(), "\"");
  if (name.size() < 80)
    name.insert(name.size(), 80 - name.size(), ' ');

  int numConnections = net->numConnections();
  for (int icon = 0; icon < numConnections; ++icon) {
    string instanceName = net->instance(icon);
    string pinName = net->pin(icon);

    // Connect to an external pin.
    if (instanceName.compare("PIN") == 0)
      continue;
    parser->toHurricaneName(pinName);

    PNLInstance* instance =
        parser->getPNLDesign()->getInstance(NLName(instanceName));
    if (instance == NULL) {
      ostringstream message;
      message << "Unknown instance (DEF COMPONENT) <" << instanceName
              << "> in <%s>.";
      parser->pushError(message.str());
      continue;
    }

    /*PNLNet* masterPNLNet = instance->getModel()->getNet( NLName(pinName) );
    if (not masterPNLNet) {
      ostringstream message;
      message << "Unknown PIN <" << pinName << "> in instance <"
              << instanceName << "> (LEF MACRO) in <%s>.";
      parser->pushError( message.str() );
      continue;
    }*/
    assert(instance->getInstTerm(instance->getModel()->getBitTerm(NLName(pinName))) != NULL);
    instance->getInstTerm(instance->getModel()->getBitTerm(NLName(pinName)))->setNet(hnet);
  }

  return 0;
}

int snetCbk_(defrCallbackType_e c, defiNet* net, lefiUserData ud) {
  static size_t netCount = 0;

  DEFConstructor* parser = (DEFConstructor*)ud;

  string name = net->name();
  parser->toHurricaneName(name);

  PNLNetDatas* netDatas = parser->lookupPNLNet(name);
  PNLNet* hnet = NULL;
  if (not netDatas) {
    hnet = parser->getPNLDesign()->addNet(NLName(name));
    parser->addPNLNetLookup(name, hnet);
  } else
    hnet = get<0>(*netDatas);

  // if (parser->getPrebuildPNLNet(false)) {
  //   const NLName & prebuildAlias = parser->getPrebuildPNLNet()->getName();
  //   hnet->merge( parser->getPrebuildPNLNet() );
  //   hnet->removeAlias( prebuildAlias );
  //   parser->setPrebuildPNLNet( NULL );
  // }

  if (name.size() > 78) {
    name.erase(0, name.size() - 75);
    name.insert(0, 3, '.');
  }
  name.insert(0, "\"");
  name.insert(name.size(), "\"");
  if (name.size() < 80)
    name.insert(name.size(), 80 - name.size(), ' ');

  int numConnections = net->numConnections();
  for (int icon = 0; icon < numConnections; ++icon) {
    string instanceName = net->instance(icon);
    string pinName = net->pin(icon);

    // Connect to an external pin.
    if (instanceName.compare("PIN") == 0)
      continue;
    parser->toHurricaneName(pinName);

    PNLInstance* instance =
        parser->getPNLDesign()->getInstance(NLName(instanceName));
    if (instance == NULL) {
      ostringstream message;
      message << "Unknown instance (DEF COMPONENT) <" << instanceName
              << "> in <%s>.";
      parser->pushError(message.str());
      continue;
    }
    instance->getInstTerm(instance->getModel()->getBitTerm(NLName(pinName)))->setNet(hnet);
  }

  return 0;
}

int netEndCbk_(defrCallbackType_e c, void*, lefiUserData ud) {
  DEFConstructor* parser = (DEFConstructor*)ud;
  return parser->flushErrors();
}

int pathCbk_(defrCallbackType_e c, defiPath* path, lefiUserData ud) {
  return 0;
}

}  // namespace

PNLDesign* DEFConstructor::parse(string file,
                                 unsigned int flags,
                                 naja::NL::NLDB* db) {
  size_t iext = file.rfind('.');
  if (file.compare(iext, 4, ".def") != 0) {
    assert(false);
    // throw Error ("DEFConstructor::construct(): DEF files must have  \".def\"
    // extension <%s>.",file.c_str());
  }

  size_t istart = 0;
  size_t length = file.size() - 4;
  size_t islash = file.rfind('/');
  if (islash != string::npos) {
    istart = islash + 1;
    length = file.size() - istart - 4;
  }
  string designName = file.substr(istart, length);
  unique_ptr<DEFConstructor> parser(
      new DEFConstructor(file, /*library,*/ flags, db));

  FILE* defStream = fopen(file.c_str(), "r");
  if (not defStream) {
    assert(false);
    // throw Error ("DEFConstructor::construct(): Cannot open DEF file
    // <%s>.",file.c_str());
  }

  parser->createPNLDesign_(designName.c_str());
  defrRead(defStream, file.c_str(), (defiUserData)parser.get(), 1);

  fclose(defStream);

  return parser->getPNLDesign();
}

void addSupplyPNLNets(PNLDesign* cell) {
  /*PNLNet* vcc = cell->addNet(NLName("VDD") );
  vcc->setExternal ( true );
  vcc->setGlobal   ( true );
  vcc->setType     ( PNLNet::Type::VDD );

  PNLNet* gnd = cell->addNet(NLName("GND") );
  gnd->setExternal ( true );
  gnd->setGlobal   ( true );
  gnd->setType     ( PNLNet::Type::GND );*/
}

double DEFConstructor::defUnits_ = 0.01;
NLLibrary* DEFConstructor::lefRootNLLibrary_ = NULL;

DEFConstructor::DEFConstructor(string file,
                               unsigned int flags,
                               naja::NL::NLDB* db)
    : flags_(flags),
      file_(file),
      busBits_("()"),
      cell_(NULL),
      pitchs_(0),
      slices_(0),
      fitOnPNLDesignsDieArea_(),
      prebuildPNLNet_(NULL),
      netsLookup_(),
      viasLookup_(),
      errors_(),
      db_(db) {
  defrInit();
  defrSetUnitsCbk(unitsCbk_);
  defrSetBusBitCbk(busBitCbk_);
  defrSetDesignEndCbk(designEndCbk_);
  defrSetDieAreaCbk(dieAreaCbk_);
  defrSetViaCbk(viaCbk_);
  defrSetPinCbk(pinCbk_);
  defrSetComponentCbk(componentCbk_);
  defrSetComponentEndCbk(componentEndCbk_);
  defrSetNetCbk(netCbk_);
  defrSetNetEndCbk(netEndCbk_);
  defrSetSNetCbk(snetCbk_);
  defrSetPathCbk(pathCbk_);
}

DEFConstructor::~DEFConstructor() {
  defrReset();
}
inline void DEFConstructor::setUnits(double units) {
  defUnits_ = 1 / units;
}
inline PNLBox::Unit DEFConstructor::fromDefUnits(int u) {
  return u;
}
inline bool DEFConstructor::isSky130() const {
  return flags_ & Sky130;
}
inline bool DEFConstructor::hasErrors() {
  return not errors_.empty();
}
inline unsigned int DEFConstructor::getFlags() const {
  return flags_;
}
inline string DEFConstructor::getBusBits() const {
  return busBits_;
}
inline PNLDesign* DEFConstructor::getPNLDesign() {
  return cell_;
}
inline size_t DEFConstructor::getPitchs() const {
  return pitchs_;
}
inline size_t DEFConstructor::getSlices() const {
  return slices_;
}
inline const PNLBox& DEFConstructor::getFitOnPNLDesignsDieArea() const {
  return fitOnPNLDesignsDieArea_;
}
inline vector<string>& DEFConstructor::getErrors() {
  return errors_;
}
inline void DEFConstructor::pushError(string error) {
  errors_.push_back(error);
}
inline void DEFConstructor::clearErrors() {
  return errors_.clear();
}
inline void DEFConstructor::setPitchs(size_t pitchs) {
  pitchs_ = pitchs;
}
inline void DEFConstructor::setSlices(size_t slices) {
  slices_ = slices;
}
inline void DEFConstructor::setPrebuildPNLNet(PNLNet* net) {
  prebuildPNLNet_ = net;
}
inline void DEFConstructor::setBusBits(string busbits) {
  busBits_ = busbits;
}
inline void DEFConstructor::mergeToFitOnPNLDesignsDieArea(const PNLBox& box) {
  fitOnPNLDesignsDieArea_.merge(box);
}

PNLNet* DEFConstructor::getPrebuildPNLNet(bool create) {
  if (create and not prebuildPNLNet_) {
    prebuildPNLNet_ = cell_->addNet(naja::NL::NLName("__prebuildnet__"));
  }
  return prebuildPNLNet_;
}

NLLibrary* DEFConstructor::createDEFLibrary() {
  // if (_mergeNLLibrary) {
  //   library_ = _mergeNLLibrary;
  //   return library_;
  // }
  NLLibrary* rootNLLibrary = db_->getLibrary(NLName("LIB1"));
  if (rootNLLibrary == nullptr) {
    NLLibrary* rootNLLibrary = NLLibrary::create(db_, NLName("LIB1"));
  }
  NLLibrary* defRootNLLibrary = NLLibrary::create(rootNLLibrary, NLName("DEF"));
  library_ = defRootNLLibrary;
  /*library_ = lefRootNLLibrary->getLibrary( NLName(library_Name) );
  if (library_) {
    assert(false);
    // Error
  } else {
    library_ = NLLibrary::create( lefRootNLLibrary, NLName(library_Name) );
  }*/
  return library_;
}

PNLDesign* DEFConstructor::getLEFMacro(string name) {
  // if (not lefRootNLLibrary_) {
  //   NLLibrary*  rootNLLibrary = db_->getRootNLLibrary();

  //   if (rootNLLibrary) {
  //     lefRootNLLibrary_ = rootNLLibrary->getLibrary( "LEF" );
  //   }
  // }
  NLLibrary* rootNLLibrary = db_->getLibrary(NLName("LIB1"));
  lefRootNLLibrary_ = db_->getLibrary(NLName("LEF"));
  assert(lefRootNLLibrary_ != nullptr);
  PNLDesign* macro = NULL;
  if (lefRootNLLibrary_) {
    for (NLLibrary* library : lefRootNLLibrary_->getLibraries()) {
      macro = library->getPNLDesign(NLName(name));
      if (macro)
        break;
    }
  }
  return macro;
}

PNLOrientation DEFConstructor::fromDefOrientation(int orient) {
  // Note : the codes between DEF & Hurricane matches.
  // This function is just to be clear.
  switch (orient) {
    default:
    case 0:
      break;  // N, default.
    case 1:
      return PNLOrientation::Type::R90;  // W
    case 2:
      return PNLOrientation::Type::R180;  // S
    case 3:
      return PNLOrientation::Type::R270;  // E
    case 4:
      return PNLOrientation::Type::MX;  // FN
    case 5:
      return PNLOrientation::Type::MXR90;  // FW
    case 6:
      return PNLOrientation::Type::MY;  // FS
    case 7:
      return PNLOrientation::Type::MYR90;  // FE
  }
  return PNLOrientation::Type::R0;
}

void DEFConstructor::toHurricaneName(string& defName) {
  if (busBits_ != "()") {
    if (defName[defName.size() - 1] == busBits_[1]) {
      size_t pos = defName.rfind(busBits_[0]);
      if (pos != string::npos) {
        defName[pos] = '(';
        defName[defName.size() - 1] = ')';
      }
    }
  }
}

PNLTransform DEFConstructor::getPNLTransform(const PNLBox& abox,
                                             PNLBox::Unit x,
                                             PNLBox::Unit y,
                                             const PNLOrientation orientation) {
  switch (orientation.getType().getType()) {
    default:
    case PNLOrientation::Type::R0:
      return PNLTransform(x, y, orientation);
    case PNLOrientation::Type::R90:
      return PNLTransform(x, y + abox.getWidth(), orientation);
    case PNLOrientation::Type::R180:
      return PNLTransform(x + abox.getWidth(), y + abox.getHeight(),
                          orientation);
    case PNLOrientation::Type::R270:
      return PNLTransform(x + abox.getHeight(), y, orientation);
    case PNLOrientation::Type::MX:
      return PNLTransform(x + abox.getWidth(), y, orientation);
    case PNLOrientation::Type::MXR90:
      return PNLTransform(x + abox.getHeight(), y + abox.getWidth(),
                          orientation);
    case PNLOrientation::Type::MY:
      return PNLTransform(x, y + abox.getHeight(), orientation);
    case PNLOrientation::Type::MYR90:
      return PNLTransform(x + abox.getHeight(), y + abox.getWidth(),
                          orientation);
  }
  return PNLTransform();
}

PNLDesign* DEFConstructor::createPNLDesign_(const char* name) {
  cell_ = PNLDesign::create(getLibrary(true), NLName(name));
  cell_->setClassType(PNLDesign::ClassType::BLOCK);
  addSupplyPNLNets(cell_);
  return cell_;
}

int DEFConstructor::flushErrors() {
  int code = (hasErrors()) ? 1 : 0;

  for (size_t ierror = 0; ierror < errors_.size(); ++ierror) {
    string message = "DEFConstructor::construct(): " + errors_[ierror];
    cerr << message.c_str() << " " << cell_->getName().getString().c_str()
         << endl;
  }
  clearErrors();

  return code;
}

PNLNetDatas* DEFConstructor::lookupPNLNet(string netName) {
  map<string, PNLNetDatas>::iterator imap = netsLookup_.find(netName);
  if (imap == netsLookup_.end())
    return NULL;

  return &((*imap).second);
}

PNLNetDatas* DEFConstructor::addPNLNetLookup(string netName, PNLNet* net) {
  PNLNetDatas* netDatas = lookupPNLNet(netName);
  if (not netDatas) {
    auto insertIt = netsLookup_.insert(make_pair(netName, make_tuple(net, 0)));
    netDatas = &(((*(insertIt.first)).second));
  }
  return netDatas;
}

ViaDatas* DEFConstructor::lookupVia(string viaName) {
  map<string, ViaDatas>::iterator imap = viasLookup_.find(viaName);
  if (imap == viasLookup_.end())
    return NULL;

  return &((*imap).second);
}

ViaDatas* DEFConstructor::addViaLookup(string viaName, PNLDesign* via) {
  ViaDatas* viaDatas = lookupVia(viaName);
  if (not viaDatas) {
    auto insertIt = viasLookup_.insert(make_pair(viaName, make_tuple(via, 0)));
    viaDatas = &(((*(insertIt.first)).second));
  }
  return viaDatas;
}

PNLDesign* DEFConstructor::construct(string design,
                                     unsigned int flags,
                                     naja::NL::NLDB* db) {
  PNLDesign* cell = NULL;
  if ((design.size() > 4) and (design.substr(design.size() - 4) != ".def"))
    design += ".def";
  cell = DEFConstructor::parse(design, flags, db);
  return cell;
}
