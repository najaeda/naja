#ifndef __PNL_INSTANCE_H_
#define __PNL_INSTANCE_H_

#include "PNLTransform.h"

namespace naja { namespace PNL {

class PNLDesign;

class PNLInstance {
  private:
    PNLDesign*    design_;
    PNLDesign*    model_;
    PNLPoint      origin_;
    PNLTransform  transform_;
};

}} // namespace PNL // namespace naja

#endif // __PNL_INSTANCE_H_