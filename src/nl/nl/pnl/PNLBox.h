// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "PNLPoint.h"
#include <limits>

namespace naja { namespace NL {

class PNLBox {
  public:
    using Unit = long int;
    PNLBox() = default;
    PNLBox(Unit left, Unit bottom, Unit right, Unit top):
      left_(left), bottom_(bottom), right_(right), top_(top)
    {}
    Unit getLeft() const { return left_; }
    Unit getBottom() const { return bottom_; }
    Unit getRight() const { return right_; }
    Unit getTop() const { return top_; }
    PNLPoint getUpperRight() const { return PNLPoint(right_, top_); }
    PNLPoint getLowerLeft() const { return PNLPoint(left_, bottom_); }
    private:
      Unit left_    {std::numeric_limits<Unit>::max()};
      Unit bottom_  {std::numeric_limits<Unit>::max()};
      Unit right_   {std::numeric_limits<Unit>::min()};
      Unit top_     {std::numeric_limits<Unit>::min()};
};

}} // namespace NL // namespace naja