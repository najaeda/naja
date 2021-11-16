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
    void dumpDesign(const SNLDesign* design, std::ostream& o);
  private:
    static SNLName createInstanceName(const SNLInstance* instance);
    void dumpInstance(const SNLInstance* instance, std::ostream& o);

};

}

#endif /* __SNL_VRL_DUMPER_H_ */
