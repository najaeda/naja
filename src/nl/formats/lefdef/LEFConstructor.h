// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include "NLDB.h"
#include "PNLBox.h"

using namespace std;

namespace naja {
namespace NL {
class NLLibrary;
class PNLDesign;
class PNLNet;
class PNLTerm;
class PNLPoint;

class LEFConstructor {
 public:
  static void setMergeLibrary(NLLibrary*);
  static void setGdsForeignDirectory(string);
  static NLLibrary* parse(string file);
  LEFConstructor(string file, string libraryName);
  ~LEFConstructor();
  inline bool isVH() const;
  bool isUnmatchedLayer(string);
  NLLibrary* createNLLibrary();
  PNLDesign* earlyGetPNLDesign(bool& created, string name = "");
  PNLNet* earlygetNet(string name);
  PNLTerm* earlygetTerm(string name);
  inline string getLibraryName() const;
  inline NLLibrary* getLibrary(bool create = false);
  inline string getForeignPath() const;
  inline void setForeignPath(string);
  inline const PNLPoint& getForeignPosition() const;
  inline void setForeignPosition(const PNLPoint&);
  inline PNLNet* getGdsPower() const;
  inline void setGdsPower(PNLNet*);
  inline PNLNet* getGdsGround() const;
  inline void setGdsGround(PNLNet*);
  inline PNLDesign* getPNLDesign() const;
  inline void setPNLDesign(PNLDesign*);
  inline PNLNet* getNet() const;
  inline void setPNLNet(PNLNet*);
  inline PNLBox::Unit getMinTerminalWidth() const;
  inline double getUnitsMicrons() const;
  inline void setUnitsMicrons(double);
  inline bool hasErrors() const;
  inline const vector<string>& getErrors() const;
  inline void pushError(const string&);
  int flushErrors();
  inline void clearErrors();
  inline void addPinComponent(string name, PNLTerm*);
  inline void clearPinComponents();
  naja::NL::NLDB* getDB() { return db_; }
  static naja::NL::NLLibrary* construct(std::string fileName);
  static string getGdsForeignDirectory() { return gdsForeignDirectory_; }

 private:
  naja::NL::NLDB* db_ = nullptr;
  static string gdsForeignDirectory_;
  static NLLibrary* mergeNLLibrary_;
  string file_;
  string libraryName_;
  NLLibrary* library_;
  string foreignPath_;
  PNLPoint foreignPosition_;
  PNLNet* gdsPower_;
  PNLNet* gdsGround_;
  PNLDesign* cell_;
  PNLNet* net_;
  string busBits_;
  double unitsMicrons_;
  PNLBox::Unit oneGrid_;
  map<string, vector<PNLTerm*> > pinComponents_;
  vector<string> errors_;
};

}  // namespace NL
}  // namespace naja