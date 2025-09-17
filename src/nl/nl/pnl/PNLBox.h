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
    PNLBox(Unit left, Unit bottom):
      left_(left), bottom_(bottom), right_(left), top_(bottom)
    {}
    Unit getLeft() const { return left_; }
    Unit getBottom() const { return bottom_; }
    Unit getRight() const { return right_; }
    Unit getTop() const { return top_; }
    PNLPoint getUpperRight() const { return PNLPoint(right_, top_); }
    PNLPoint getLowerLeft() const { return PNLPoint(left_, bottom_); }

    Unit getWidth() const {return (right_ - left_);};
    Unit getHalfWidth() const {return (getWidth() / 2);};
    Unit getHeight() const {return (top_ - bottom_);};
    Unit getHalfHeight() const {return (getHeight() / 2);};

    void increase(Unit xLeft, Unit yBottom, Unit xRight, Unit yTop) {
      left_   -= xLeft;
      bottom_ -= yBottom;
      right_  += xRight;
      top_    += yTop;
    }

    void increase(Unit xDelta, Unit yDelta) {
      increase(xDelta, yDelta, xDelta, yDelta);
    }

    // Comperators 
    bool operator==(const PNLBox& other) const {
      return left_ == other.left_ && bottom_ == other.bottom_ &&
             right_ == other.right_ && top_ == other.top_;
    }
    bool operator!=(const PNLBox& other) const {
      return !(*this == other);
    }
    bool operator<(const PNLBox& other) const {
      return (left_ < other.left_) || (left_ == other.left_ && bottom_ < other.bottom_) ||
             (left_ == other.left_ && bottom_ == other.bottom_ && right_ < other.right_) ||
             (left_ == other.left_ && bottom_ == other.bottom_ && right_ == other.right_ && top_ < other.top_);
    }
    bool operator>(const PNLBox& other) const {
      return (left_ > other.left_) || (left_ == other.left_ && bottom_ > other.bottom_) ||
             (left_ == other.left_ && bottom_ == other.bottom_ && right_ > other.right_) ||
             (left_ == other.left_ && bottom_ == other.bottom_ && right_ == other.right_ && top_ > other.top_);
    }
    bool operator<=(const PNLBox& other) const {
      return !(*this > other);
    }
    bool operator>=(const PNLBox& other) const {
      return !(*this < other);
    }

    void merge(const PNLBox& other) {
      if (other.left_ < left_)   left_   = other.left_;
      if (other.bottom_ < bottom_) bottom_ = other.bottom_;
      if (other.right_ > right_) right_  = other.right_;
      if (other.top_ > top_)     top_    = other.top_;
    }

    bool isEmpty() const {
      return (right_ < left_) && (top_ < bottom_);
    }

    private:
      Unit left_    {std::numeric_limits<Unit>::max()};
      Unit bottom_  {std::numeric_limits<Unit>::max()};
      Unit right_   {std::numeric_limits<Unit>::min()};
      Unit top_     {std::numeric_limits<Unit>::min()};
};

}} // namespace NL // namespace naja