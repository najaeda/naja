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

using namespace std;

namespace naja {
namespace NL {

class PNLDesign;
class NLLibrary;

class LEFDumper {
 public:
  static void dump(const std::vector<PNLDesign*>&, const string& libraryName);
  static int getUnits();
  static PNLBox::Unit getSliceHeight();
  static PNLBox::Unit getPitchWidth();
  LEFDumper(const std::vector<PNLDesign*>&, const string& libraryName, FILE*);
  ~LEFDumper();
  int write();
  static void dump(naja::NL::NLLibrary* library);
  inline int getStatus() const;
  inline const std::vector<PNLDesign*> getPNLDesigns() const;
  inline const string& getNLLibraryName() const;
  int checkStatus(int status);
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