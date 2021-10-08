#ifndef __SNL_DUMP_H_
#define __SNL_DUMP_H_

#include <filesystem>

namespace SNL {

class SNLDesign;

class SNLDump {
  public:
    void dump(const SNLDesign* top, const std::filesystem::path& path);
};

}

#endif /* __SNL_DUMP_H_ */
