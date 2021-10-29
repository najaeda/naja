#ifndef __SNL_VRL_DUMPER_H_
#define __SNL_VRL_DUMPER_H_

#include <filesystem>

namespace SNL {

class SNLDesign;

class SNLVRLDumper {
  public:
    class Configuration {
    };
    void dump(SNLDesign* design); 
};

}

#endif /* __SNL_VRL_DUMPER_H_ */
