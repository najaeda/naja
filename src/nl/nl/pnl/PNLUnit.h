// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_UNIT_H_
#define __PNL_UNIT_H_

#include <cmath>
#include "PNLBox.h"
#include <stdio.h>

namespace naja {
namespace NL {

class PNLUnit {
 public:
  using Unit = long int;
  enum UnitPower { Pico = 1, Nano, Micro, Milli, Unity, Kilo };
  enum StringMode {
    Db = (1 << 0),
    Grid = (1 << 1),
    Symbolic = (1 << 2),
    Physical = (1 << 3),
    SmartTruncate = (1 << 4)
  };

  // static inline PNLBox::Unit fromPhysical(double value, UnitPower p) {
  //   checkPhysicalBound(value, p);
  //   printf("fromPhysical: %lf * %lf / %lf =  %ld\n", value, getUnitPower(p), _physicalsPerGrid,
  //           fromGrid((value * getUnitPower(p)) / _physicalsPerGrid));
  //   return fromGrid((value * getUnitPower(p)) / _physicalsPerGrid);
  // }
  // static inline Unit fromGrid(double value) {
  //   checkGridBound(value);
  //   printf("fromGrid: %lf / %lf = %ld\n", value, _resolution, (Unit)(value / _resolution));
  //   return (Unit)rint(value / _resolution);
  // }
  // static void checkGridBound(double value) {
  //   if (value < 0)
  //     value = -value;
  //   // if (value >= _gridMax)
  //   //   throw Error( "Grid value %.1f converts to out of range PNLUnit value
  //   //   (maximum is: %.1f)."
  //   //              , value, _gridMax );
  // }
  // static void checkPhysicalBound(double value, UnitPower p) {
  //   if (value < 0)
  //     value = -value;
  //   value *= getUnitPower(p);
  //   // if (value >= _physicalMax)
  //   //   throw Error( "Physical value %.1fnm converts to out of range PNLUnit
  //   //   (maximum is: %.1f)."
  //   //              , (value/1.0e-9), _physicalMax );
  // }
  // static void checkLambdaBound(double value) {
  //   if (value < 0)
  //     value = -value;
  //   // if (value >= _lambdaMax)
  //   //   throw Error( "Lambda value %.1f converts to out of range PNLUnit
  //   //   (maximum is: %.1f)."
  //   //             , value, _lambdaMax );
  // }

  // static void setPhysicalsPerGrid(double physicalsPerGrid, UnitPower p) {
  //   _physicalsPerGrid = physicalsPerGrid * getUnitPower(p);
  //   _updateBounds();
  // }
  // static double getUnitPower(UnitPower p) {
  //   switch (p) {
  //     case Pico:
  //       return 1.0e-12;
  //     case Nano:
  //       return 1.0e-9;
  //     case Micro:
  //       return 1.0e-6;
  //     case Milli:
  //       return 1.0e-3;
  //     case Unity:
  //       return 1.0;
  //     case Kilo:
  //       return 1.0e+3;
  //   }
  //   return 1.0;
  // }
  // static void _updateBounds() {
  //   _gridMax = toGrid(Max);
  //   _lambdaMax = toLambda(Max);
  //   _physicalMax = toPhysical(Max, Unity);
  // }
  // static inline double toGrid(Unit u) { return _resolution * (double)u; }
  // static inline double toGrid(double u) { return _resolution * u; }
  // static inline double toLambda(Unit u) { return toGrid(u) / _gridsPerLambda; }
  // static inline double toLambda(double u) {
  //   return toGrid(u) / _gridsPerLambda;
  // }
  // static inline double toPhysical(Unit u, UnitPower p) {
  //   return (_physicalsPerGrid * _resolution * (double)u) / getUnitPower(p);
  // }
  // inline double toPhysical(double u, UnitPower p) {
  //   return (_physicalsPerGrid * _resolution * u) / getUnitPower(p);
  // }
  // static inline double toMicrons(Unit u) {
  //   return toPhysical(u, UnitPower::Micro);
  // }
  // static inline Unit fromMicrons(double value) {
  //   return fromPhysical(value, UnitPower::Micro);
  // }

  // static inline double toNanos(Unit u) {
  //   return toPhysical(u, UnitPower::Nano);
  // }
  // static const Unit Min;
  // static const Unit Max;
  // static void setStringMode(unsigned int mode, UnitPower p) {
  //   _stringMode = mode;
  //   if (_stringMode == Physical)
  //     _stringModeUnitPower = p;
  // }
  // static inline Unit lambda(double value) { return fromLambda(value); }
  // static inline Unit fromLambda(double value) {
  //   checkLambdaBound(value);
  //   return fromGrid(value * _gridsPerLambda);
  // }

//  private:
//   static const unsigned int _maximalPrecision;
//   static unsigned int _precision;
//   static double _resolution;
//   static double _gridsPerLambda;
//   static double _physicalsPerGrid;
//   static unsigned int _stringMode;
//   static UnitPower _stringModeUnitPower;
//   static Unit _realSnapGridStep;
//   static Unit _symbolicSnapGridStep;
//   static Unit _polygonStep;
//   static double _gridMax;
//   static double _lambdaMax;
//   static double _physicalMax;
};

}  // namespace NL

}  // namespace naja

#endif  // __PNL_UNIT_H_ßßß