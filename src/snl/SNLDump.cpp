#include "SNLDump.h"

#include "SNLLevelize.h"

namespace SNL {

void SNLDump::dump(const SNLDesign* top, const std::filesystem::path& path) {
  SNLLevelize levelize(top);
  //publish manifest
}

}
