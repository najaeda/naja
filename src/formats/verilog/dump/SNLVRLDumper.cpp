#include "SNLVRLDumper.h"

#include "SNLDesign.h"

namespace SNL {

void SNLVRLDumper::dump(const SNLDesign* design, std::ostream& o) {
  o << "module " << design->getName() << std::endl;
  o << "endmodule;" << std::endl; 
}

}
