#ifndef __SNL_NETLISTS_H_
#define __SNL_NETLISTS_H_

namespace naja::SNL {
  class SNLLibrary;
  class SNLDesign;
}

class SNLNetlists {
  public:
    static naja::SNL::SNLDesign*
      createNetlist0(naja::SNL::SNLLibrary* primitivesLib, naja::SNL::SNLLibrary* designsLib);
};

#endif // __SNL_NETLISTS_H_