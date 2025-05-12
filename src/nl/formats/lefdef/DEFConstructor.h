
// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include "defrReader.hpp"
#include "lefrReader.hpp"
#include "NLDB.h"

using namespace std;

namespace naja {
namespace PNL {
class PNLDesign;
}
namespace NL {
typedef tuple<PNLNet*, uint32_t> PNLNetDatas;
typedef tuple<PNLDesign*, uint32_t> ViaDatas;
class DEFConstructor {
 public:
  const uint32_t NoPatch = 0;
  const uint32_t Sky130 = (1 << 10);
  enum Flags { FitAbOnDesigns = 0x1 };

 public:
  PNLDesign* createPNLDesign_(const char* name);
  static naja::NL::PNLDesign* construct(std::string design,
                                   unsigned int flags,
                                   naja::NL::NLDB* db);

  // static AllianceFramework* getFramework             ();
  PNLDesign* getLEFMacro(std::string name);
  static void setUnits(double);
  static PNLBox::Unit fromDefUnits(int);
  static PNLOrientation fromDefOrientation(int orient);
  inline NLLibrary* getLibrary(bool create) {
    if (not library_ /*and create*/)
      createDEFLibrary();
    return library_;
  }
  NLLibrary* createDEFLibrary();
  static PNLTransform getPNLTransform(const PNLBox&,
                                      const PNLBox::Unit x,
                                      const PNLBox::Unit y,
                                      const PNLOrientation);
  static PNLDesign* parse(std::string file, unsigned int flags, naja::NL::NLDB* db);
  DEFConstructor(std::string file,
                 /*AllianceNLLibrary*,*/ unsigned int flags,
                 naja::NL::NLDB* db);
  ~DEFConstructor();
  inline bool hasErrors();
  inline bool isSky130() const;
  inline unsigned int getFlags() const;
  // inline AllianceNLLibrary*   getLibrary               ();
  inline PNLDesign* getPNLDesign();
  inline size_t getPitchs() const;
  inline size_t getSlices() const;
  inline const PNLBox& getFitOnPNLDesignsDieArea() const;
  PNLNet* getPrebuildPNLNet(bool create = true);
  inline string getBusBits() const;
  PNLNetDatas* lookupPNLNet(std::string);
  ViaDatas* lookupVia(std::string);
  //  Layer*             lookupLayer              ( string );
  inline vector<string>& getErrors();
  inline void pushError(std::string);
  int flushErrors();
  inline void clearErrors();
  inline void setPitchs(size_t);
  inline void setSlices(size_t);
  inline void setPrebuildPNLNet(PNLNet*);
  inline void setBusBits(std::string);
  PNLNetDatas* addPNLNetLookup(std::string netName, PNLNet*);
  ViaDatas* addViaLookup(std::string viaName, PNLDesign*);
  void toHurricaneName(std::string&);
  inline void mergeToFitOnPNLDesignsDieArea(const PNLBox&);
  // Contact*           createVia                ( string viaName, PNLNet*,
  // PNLBox::Unit x, PNLBox::Unit y );
 private:
  static int unitsCbk_(defrCallbackType_e, double, defiUserData);
  static int busBitCbk_(defrCallbackType_e, const char*, defiUserData);
  static int designEndCbk_(defrCallbackType_e, void*, defiUserData);
  static int dieAreaCbk_(defrCallbackType_e, defiBox*, defiUserData);
  static int pinCbk_(defrCallbackType_e, defiPin*, defiUserData);
  static int viaCbk_(defrCallbackType_e, defiVia*, defiUserData);
  static int componentCbk_(defrCallbackType_e, defiComponent*, defiUserData);
  static int componentEndCbk_(defrCallbackType_e, void*, defiUserData);
  static int netCbk_(defrCallbackType_e, defiNet*, defiUserData);
  static int netEndCbk_(defrCallbackType_e, void*, defiUserData);
  static int snetCbk_(defrCallbackType_e, defiNet*, defiUserData);
  static int pathCbk_(defrCallbackType_e, defiPath*, defiUserData);

 private:
  static double defUnits_;
  static NLLibrary* lefRootNLLibrary_;
  uint32_t flags_;
  string file_;
  string busBits_;
  PNLDesign* cell_;
  size_t pitchs_;
  size_t slices_;
  PNLBox fitOnPNLDesignsDieArea_;
  PNLNet* prebuildPNLNet_;
  map<string, PNLNetDatas> netsLookup_;
  map<string, ViaDatas> viasLookup_;
  vector<string> errors_;
  naja::NL::NLLibrary* library_;
  naja::NL::NLDB* db_ = nullptr;
};
}  // namespace NL
}  // namespace naja
