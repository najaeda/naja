#ifndef __SNL_NETLISTS_H_
#define __SNL_NETLISTS_H_

namespace naja::SNL {
  class SNLDB;
  class SNLDesign;
}

class SNLNetlists {
  public:
    static naja::SNL::SNLDesign* createNetlist0(naja::SNL::SNLDB* db);
};

#endif // __SNL_NETLISTS_H_