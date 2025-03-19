// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_NETLIST0_H_
#define __SNL_NETLIST0_H_

#include <string>

namespace naja::NL {
  class NLDB;
  class NLLibrary;

  class SNLDesign;
  class SNLInstance;
  class SNLBusTerm;
  class SNLScalarNet;
  class SNLBusNet;
}

class SNLNetlist0 {
  //
  //      |    _________________________        _________________________   |
  //      |   |                        |       |                        |   |
  //      |   |   ________     _____   |       |   ________     _____   |   |
  //  i[0]--i0|--|i0      |   |i:INV|  |   |-i0|--|i0      |   |i:INV|  |   |
  //      |   |  |a:AND2 o|---|i   o|--|---|   |  |a:AND2  |---|i   o|--|---- o
  //  i[1]--i1|--|i1______|   |_____|  |   |-i1|--|i1______|   |_____|  |   |
  //      |   |                        |       |                        |   |
  //      |   |________________________|       |________________________|   |
  //      |                                                                 |
  //
  public:
    static constexpr char DesignsLibName[]  { "DESIGNS" };
    static constexpr char TopName[]         { "Top" };
    static constexpr char TopIns0Name[]     { "ins0" };
    static constexpr char TopIns1Name[]     { "ins1" };
    static constexpr char TopOName[]        { "o" };
    static constexpr char TopIName[]        { "i" };
    static constexpr char TopNetName[]      { "n" };
    static constexpr char ModuleI0Name[]    { "i0" };
    static constexpr char ModuleI1Name[]    { "i1" };
    static constexpr char ModuleOName[]     { "o" };
    
    static naja::NL::SNLDesign* create(naja::NL::NLDB* db);
    static naja::NL::NLDB* getDB();
    static naja::NL::NLLibrary* getDesignsLib();
    static naja::NL::SNLDesign* getTop();
    static naja::NL::SNLInstance* getTopIns0();
    static naja::NL::SNLInstance* getTopIns1();
    static naja::NL::SNLBusTerm* getTopITerm();
    static naja::NL::SNLBusTerm* getTopOTerm();
    static naja::NL::SNLBusNet* getTopINet();
    static naja::NL::SNLScalarNet* getTopONet();
};

#endif // __SNL_NETLIST0_H_
