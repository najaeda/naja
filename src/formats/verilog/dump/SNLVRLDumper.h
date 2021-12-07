#ifndef __SNL_VRL_DUMPER_H_
#define __SNL_VRL_DUMPER_H_

#include <filesystem>

#include "SNLName.h"

namespace SNL {

class SNLDesign;
class SNLTerm;
class SNLNet;
class SNLInstance;

class SNLVRLDumper {
  public:
    class Configuration {
    };
    void dumpDesign(const SNLDesign* design, std::ostream& o);
  private:
    static std::string createDesignName(const SNLDesign* design);
    static std::string createInstanceName(const SNLInstance* instance);
    void dumpInstance(const SNLInstance* instance, std::ostream& o);
    void dumpNet(const SNLNet* net, std::ostream& o);
    void dumpTerm(const SNLTerm* term, std::ostream& o);
};

}

#endif /* __SNL_VRL_DUMPER_H_ */
