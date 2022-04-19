#ifndef __SNL_NETLIST0_H_
#define __SNL_NETLIST0_H_

#include <string>

namespace naja::SNL {
  class SNLDB;
  class SNLLibrary;
  class SNLDesign;
  class SNLInstance;
  class SNLBusTerm;
  class SNLBusNet;
}

class SNLNetlist0 {
  public:
    static constexpr char DesignsLibName[]  { "DESIGNS" };
    static constexpr char TopName[]         { "Top" };
    static constexpr char TopIns0Name[]     { "ins0" };
    static constexpr char TopIns1Name[]     { "ins1" };
    static constexpr char TopOName[]        { "o" };
    static constexpr char TopIName[]        { "i" };
    
    static naja::SNL::SNLDesign* create(naja::SNL::SNLDB* db);
    static naja::SNL::SNLDB* getDB();
    static naja::SNL::SNLLibrary* getDesignsLib();
    static naja::SNL::SNLDesign* getTop();
    static naja::SNL::SNLInstance* getTopIns0();
    static naja::SNL::SNLInstance* getTopIns1();
    static naja::SNL::SNLBusTerm* getTopITerm();
    static naja::SNL::SNLBusTerm* getTopOTerm();
    static naja::SNL::SNLBusNet* getTopINet();
    static naja::SNL::SNLBusNet* getTopONet();
};

#endif // __SNL_NETLIST0_H_