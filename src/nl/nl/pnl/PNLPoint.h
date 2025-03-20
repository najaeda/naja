// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace naja { namespace NL {

class PNLPoint {
  public:
    using Unit = long int;
    PNLPoint(Unit x, Unit y): x_(x), y_(y) {}
    Unit getX() const { return x_; }
    Unit getY() const { return y_; }
    
    bool operator==(const PNLPoint& other) const {
      return x_ == other.x_ and y_ == other.y_;
    }
    bool operator!=(const PNLPoint& other) const {
      return !(*this == other);
    }
 
  private:
    Unit  x_;
    Unit  y_;
};

}} // namespace NL // namespace naja
