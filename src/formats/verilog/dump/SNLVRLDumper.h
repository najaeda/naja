#ifndef __SNL_VRL_DUMPER_H_
#define __SNL_VRL_DUMPER_H_

#include <filesystem>

#include "SNLName.h"

namespace SNL {

class SNLDesign;
class SNLInstance;

class SNLVRLDumper {
  public:
    class Configuration {
    };
    static SNLName createInstanceName(const SNLInstance* instance);
    void dump(const SNLDesign* design, std::ostream& o);
};

}

#endif /* __SNL_VRL_DUMPER_H_ */
