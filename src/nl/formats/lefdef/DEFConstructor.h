
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
#include "PNLBox.h"

using namespace std;

namespace naja {

namespace NL {
class PNLDesign;
class NLDB;
class PNLNet;
class NLLibrary;
class PNLTransform;
class PNLOrientation;
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
  DEFConstructor(std::string file, unsigned int flags,
                 naja::NL::NLDB* db);
  ~DEFConstructor();
  inline bool hasErrors();
  inline bool isSky130() const;
  inline unsigned int getFlags() const;
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
