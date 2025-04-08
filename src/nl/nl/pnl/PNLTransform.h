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

    // Default constructor
    PNLTransform() = default;

    PNLPoint getOffset() const { return offset_; }
    PNLOrientation getOrientation() const { return orientation_; }

    // Comperators
    bool operator==(const PNLTransform& other) const {
      return offset_ == other.offset_ && orientation_ == other.orientation_;
    }
    bool operator!=(const PNLTransform& other) const {
      return !(*this == other);
    }
    bool operator<(const PNLTransform& other) const {
      return (offset_ < other.offset_) || (offset_ == other.offset_ && orientation_ < other.orientation_);
    }
    bool operator>(const PNLTransform& other) const {
      return (offset_ > other.offset_) || (offset_ == other.offset_ && orientation_ > other.orientation_);
    }
    bool operator<=(const PNLTransform& other) const {
      return !(*this > other);
    }
    bool operator>=(const PNLTransform& other) const {
      return !(*this < other);
    }
  private:
    PNLPoint        offset_{0, 0};
    PNLOrientation  orientation_;
};


}} // namespace PNL // namespace naja

#endif // __PNL_TRANSFORM_H_