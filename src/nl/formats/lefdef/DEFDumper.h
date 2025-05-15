// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PNLBox.h"
#include <string>
#include <cstdint>

namespace naja {
namespace NL {

  class PNLDesign;
  class PNLOrientation;
  class PNLInstance;
  class PNLTransform;

class DEFDumper {
 public:
  static const uint32_t WithLEF = (1 << 0);
  static const uint32_t ExpandDieArea = (1 << 1);
  static const uint32_t ProtectNetNames = (1 << 2);
  static void drive(PNLDesign* PNLDesign, uint32_t flags);
  static int getUnits();
  static int toDefUnits(PNLBox::Unit);
  static int toDefOrient(PNLOrientation);
  static void toDefCoordinates(PNLInstance*,
                               PNLTransform,
                               int& statusX,
                               int& statusY,
                               int& statusOrient);
  static PNLBox::Unit getSliceHeight();
  static PNLBox::Unit getPitchWidth();
  DEFDumper(PNLDesign*, const std::string& designName, FILE*, uint32_t flags);
  ~DEFDumper();
  int write();
  int checkStatus(int status, std::string info);
  inline PNLDesign* getDesign();
  inline const std::string& getDesignName() const;
  inline uint32_t getFlags() const;
  inline int getStatus() const;

 private:
  static int units_;
  static PNLBox::Unit sliceHeight_;
  static PNLBox::Unit pitchWidth_;
  PNLDesign* cell_;
  std::string designName_;
  FILE* defStream_;
  uint32_t flags_;
  int status_;
};
}  // namespace NL
}  // namespace naja
