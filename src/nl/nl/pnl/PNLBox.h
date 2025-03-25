// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <limits>


namespace naja { namespace NL {

class PNLBox {
  public:
    using Unit = long int;
    PNLBox() = default;
    PNLBox(Unit left, Unit right, Unit top, Unit bottom):
      left_(left), right_(right), top_(top), bottom_(bottom)
    {}
    Unit getLeft() const { return left_; }
    Unit getRight() const { return right_; }
    Unit getTop() const { return top_; }
    Unit getBottom() const { return bottom_; }
    private:
      Unit left_    {std::numeric_limits<Unit>::max()};
      Unit right_   {std::numeric_limits<Unit>::min()};
      Unit bottom_  {std::numeric_limits<Unit>::max()};
      Unit top_     {std::numeric_limits<Unit>::min()};
};

}} // namespace NL // namespace naja