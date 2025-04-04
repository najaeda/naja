// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_TRANSFORM_H_
#define __PNL_TRANSFORM_H_

#include "PNLPoint.h"
#include "PNLOrientation.h"

namespace naja { namespace NL {

class PNLTransform {
  public:
    PNLTransform(const PNLPoint& offset, const PNLOrientation& orientation):
      offset_(offset), orientation_(orientation)
    {}
    PNLTransform(const PNLPoint& offset, const PNLOrientation::Type& orientation):
      offset_(offset), orientation_(PNLOrientation(orientation))
    {}

    PNLPoint getOffset() const { return offset_; }
    PNLOrientation getOrientation() const { return orientation_; }
  private:
    PNLPoint        offset_{0, 0};
    PNLOrientation  orientation_;
};


}} // namespace PNL // namespace naja

#endif // __PNL_TRANSFORM_H_