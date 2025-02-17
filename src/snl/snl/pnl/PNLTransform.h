#ifndef __PNL_TRANSFORM_H_
#define __PNL_TRANSFORM_H_

#include "PNLPoint.h"
#include "PNLOrientation.h"

namespace naja { namespace PNL {

class PNLTransform {
  public:
    PNLTransform() = default;
    PNLTransform(const PNLPoint& offset, const PNLOrientation& orientation):
      offset_(offset), orientation_(orientation)
    {}

    PNLPoint getOffset() const { return offset_; }
    PNLOrientation getOrientation() const { return orientation_; }
  private:
    PNLPoint        offset_{0, 0};
    PNLOrientation  orientation_;
};


}} // namespace PNL // namespace naja

#endif // __PNL_TRANSFORM_H_