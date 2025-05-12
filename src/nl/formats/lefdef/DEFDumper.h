// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PNLBox.h"
#include "PNLDesign.h"
#include "PNLInstance.h"
#include "defwWriter.hpp"
#include "defwWriterCalls.hpp"
#include "lefwWriter.hpp"

namespace naja {
namespace NL {

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
  ~DEFDumper();
  int write();

 private:
  DEFDumper(PNLDesign*, const std::string& designName, FILE*, uint32_t flags);
  inline PNLDesign* getDesign();
  inline const std::string& getDesignName() const;
  inline uint32_t getFlags() const;
  inline int getStatus() const;
  int checkStatus(int status, std::string info);
 private:
  static int designCbk_(defwCallbackType_e, defiUserData);
  static int designEndCbk_(defwCallbackType_e, defiUserData);
  static int historyCbk_(defwCallbackType_e, defiUserData);
  static int versionCbk_(defwCallbackType_e, defiUserData);
  static int dividerCbk_(defwCallbackType_e, defiUserData);
  static int busBitCbk_(defwCallbackType_e, defiUserData);
  static int unitsCbk_(defwCallbackType_e, defiUserData);
  static int technologyCbk_(defwCallbackType_e, defiUserData);
  static int dieAreaCbk_(defwCallbackType_e, defiUserData);
  static int gPNLDesignGridCbk_(defwCallbackType_e, defiUserData);  // TOTO ?
  static int rowCbk_(defwCallbackType_e, defiUserData);
  static int trackCbk_(defwCallbackType_e, defiUserData);
  static int viaCbk_(defwCallbackType_e, defiUserData);
  static int pinCbk_(defwCallbackType_e, defiUserData);
  static int pinPropCbk_(defwCallbackType_e, defiUserData);
  static int componentCbk_(defwCallbackType_e, defiUserData);
  static int netCbk_(defwCallbackType_e, defiUserData);
  static int snetCbk_(defwCallbackType_e, defiUserData);
  static int extensionCbk_(defwCallbackType_e, defiUserData);
  static int groupCbk_(defwCallbackType_e, defiUserData);
  static int propDefCbk_(defwCallbackType_e, defiUserData);
  static int regionCbk_(defwCallbackType_e, defiUserData);
  static int scanchainCbk_(defwCallbackType_e, defiUserData);

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
