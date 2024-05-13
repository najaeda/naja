#ifndef __PNL_TRANSFORM_H_
#define __PNL_TRANSFORM_H_

#include "PNLPoint.h"
#include "PNLOrientation.h"

namespace naja { namespace PNL {

class PNLTransform {
  private:
    PNLPoint        offset_;
    PNLOrientation  orientation_;
};


}} // namespace PNL // namespace naja

#endif // __PNL_TRANSFORM_H_