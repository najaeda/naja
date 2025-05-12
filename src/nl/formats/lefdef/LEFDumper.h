// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

// For string
#include <string>
// For vector
#include <vector>
#include "PNLBox.h"
#include "PNLDesign.h"
#include "lefwWriter.hpp"
#include "lefwWriterCalls.hpp"

using namespace std;

namespace naja {
namespace NL {

class NLLibrary;

class LEFDumper {
 public:
  static void dump(const std::vector<PNLDesign*>&, const string& libraryName);
  static int getUnits();
  static PNLBox::Unit getSliceHeight();
  static PNLBox::Unit getPitchWidth();
  ~LEFDumper();
  int write();
  static void dump(naja::NL::NLLibrary* library);

 private:
  LEFDumper(const std::vector<PNLDesign*>&, const string& libraryName, FILE*);
  inline const std::vector<PNLDesign*> getPNLDesigns() const;
  inline const string& getNLLibraryName() const;
  inline int getStatus() const;
  int checkStatus(int status);
  static int versionCbk_(lefwCallbackType_e, lefiUserData);
  static int busBitCharsCbk_(lefwCallbackType_e, lefiUserData);
  static int clearanceMeasureCbk_(lefwCallbackType_e, lefiUserData);
  static int dividerCharCbk_(lefwCallbackType_e, lefiUserData);
  static int unitsCbk_(lefwCallbackType_e, lefiUserData);
  static int extCbk_(lefwCallbackType_e, lefiUserData);
  static int propDefCbk_(lefwCallbackType_e, lefiUserData);
  static int endLibCbk_(lefwCallbackType_e, lefiUserData);
  static int layerCbk_(lefwCallbackType_e, lefiUserData);
  static int macroCbk_(lefwCallbackType_e, lefiUserData);
  static int manufacturingGridCbk_(lefwCallbackType_e, lefiUserData);
  static int nonDefaultCbk_(lefwCallbackType_e, lefiUserData);
  static int siteCbk_(lefwCallbackType_e, lefiUserData);
  static int spacingCbk_(lefwCallbackType_e, lefiUserData);
  static int useMinSpacingCbk_(lefwCallbackType_e, lefiUserData);
  static int viaCbk_(lefwCallbackType_e, lefiUserData);
  static int viaRuleCbk_(lefwCallbackType_e, lefiUserData);
  int dumpMacro_(PNLDesign*);

 private:
  static int units_;
  const std::vector<PNLDesign*> cells_;
  string libraryName_;
  FILE* lefStream_;
  int status_;
};

}  // namespace NL
}  // namespace naja