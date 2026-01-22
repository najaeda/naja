// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "PNLOrientation.h"
#include "PNLPoint.h"
#include "PNLBox.h"

namespace naja::NL {

static const int A[8] = {1, 0, -1, 0, -1, 0, 1, 0};
static const int B[8] = {0, -1, 0, 1, 0, -1, 0, 1};
static const int C[8] = {0, 1, 0, -1, 0, -1, 0, 1};
static const int D[8] = {1, 0, -1, 0, 1, 0, -1, 0};
static constexpr int DISCREMINENT[8] = {1, 1, 1, 1, -1, -1, -1, -1};
static constexpr PNLOrientation::Type::TypeEnum COMPOSE[64] = {
    PNLOrientation::Type::TypeEnum::R0,
    PNLOrientation::Type::TypeEnum::R90,
    PNLOrientation::Type::TypeEnum::R180,
    PNLOrientation::Type::TypeEnum::R270,
    PNLOrientation::Type::TypeEnum::MX,
    PNLOrientation::Type::TypeEnum::MXR90,
    PNLOrientation::Type::TypeEnum::MY,
    PNLOrientation::Type::TypeEnum::MYR90,
    PNLOrientation::Type::TypeEnum::R90,
    PNLOrientation::Type::TypeEnum::R180,
    PNLOrientation::Type::TypeEnum::R270,
    PNLOrientation::Type::TypeEnum::R0,
    PNLOrientation::Type::TypeEnum::MXR90,
    PNLOrientation::Type::TypeEnum::MY,
    PNLOrientation::Type::TypeEnum::MYR90,
    PNLOrientation::Type::TypeEnum::MX,
    PNLOrientation::Type::TypeEnum::R180,
    PNLOrientation::Type::TypeEnum::R270,
    PNLOrientation::Type::TypeEnum::R0,
    PNLOrientation::Type::TypeEnum::R90,
    PNLOrientation::Type::TypeEnum::MY,
    PNLOrientation::Type::TypeEnum::MYR90,
    PNLOrientation::Type::TypeEnum::MX,
    PNLOrientation::Type::TypeEnum::MXR90,
    PNLOrientation::Type::TypeEnum::R270,
    PNLOrientation::Type::TypeEnum::R0,
    PNLOrientation::Type::TypeEnum::R90,
    PNLOrientation::Type::TypeEnum::R180,
    PNLOrientation::Type::TypeEnum::MYR90,
    PNLOrientation::Type::TypeEnum::MX,
    PNLOrientation::Type::TypeEnum::MXR90,
    PNLOrientation::Type::TypeEnum::MY,
    PNLOrientation::Type::TypeEnum::MX,
    PNLOrientation::Type::TypeEnum::MYR90,
    PNLOrientation::Type::TypeEnum::MY,
    PNLOrientation::Type::TypeEnum::MXR90,
    PNLOrientation::Type::TypeEnum::R0,
    PNLOrientation::Type::TypeEnum::R270,
    PNLOrientation::Type::TypeEnum::R180,
    PNLOrientation::Type::TypeEnum::R90,
    PNLOrientation::Type::TypeEnum::MXR90,
    PNLOrientation::Type::TypeEnum::MX,
    PNLOrientation::Type::TypeEnum::MYR90,
    PNLOrientation::Type::TypeEnum::MY,
    PNLOrientation::Type::TypeEnum::R90,
    PNLOrientation::Type::TypeEnum::R0,
    PNLOrientation::Type::TypeEnum::R270,
    PNLOrientation::Type::TypeEnum::R180,
    PNLOrientation::Type::TypeEnum::MY,
    PNLOrientation::Type::TypeEnum::MXR90,
    PNLOrientation::Type::TypeEnum::MX,
    PNLOrientation::Type::TypeEnum::MYR90,
    PNLOrientation::Type::TypeEnum::R180,
    PNLOrientation::Type::TypeEnum::R90,
    PNLOrientation::Type::TypeEnum::R0,
    PNLOrientation::Type::TypeEnum::R270,
    PNLOrientation::Type::TypeEnum::MYR90,
    PNLOrientation::Type::TypeEnum::MY,
    PNLOrientation::Type::TypeEnum::MXR90,
    PNLOrientation::Type::TypeEnum::MX,
    PNLOrientation::Type::TypeEnum::R270,
    PNLOrientation::Type::TypeEnum::R180,
    PNLOrientation::Type::TypeEnum::R90,
    PNLOrientation::Type::TypeEnum::R0};

class PNLTransform {
 public:
  PNLTransform(const PNLPoint& offset, const PNLOrientation& orientation)
      : offset_(offset), orientation_(orientation) {}
  PNLTransform(const PNLPoint& offset, const PNLOrientation::Type& orientation)
      : offset_(offset), orientation_(PNLOrientation(orientation)) {}
  PNLTransform(PNLBox::Unit x,
               PNLBox::Unit y,
               const PNLOrientation& orientation)
      : offset_(PNLPoint(x, y)), orientation_(orientation) {}

  // Default constructor
  PNLTransform() = default;

  PNLPoint getOffset() const { return offset_; }
  PNLOrientation getOrientation() const { return orientation_; }

  // Comperators
  bool operator==(const PNLTransform& other) const {
    return offset_ == other.offset_ && orientation_ == other.orientation_;
  }
  bool operator!=(const PNLTransform& other) const { return !(*this == other); }
  bool operator<(const PNLTransform& other) const {
    return (offset_ < other.offset_) ||
           (offset_ == other.offset_ && orientation_ < other.orientation_);
  }
  bool operator>(const PNLTransform& other) const {
    return (offset_ > other.offset_) ||
           (offset_ == other.offset_ && orientation_ > other.orientation_);
  }
  bool operator<=(const PNLTransform& other) const { return !(*this > other); }
  bool operator>=(const PNLTransform& other) const { return !(*this < other); }
  PNLTransform getTransform(const PNLTransform& transformation) const {
    PNLBox::Unit x = transformation.offset_.getX();
    PNLBox::Unit y = transformation.offset_.getY();

    return PNLTransform(
        (x * A[orientation_.getType().getType()]) +
            (y * B[orientation_.getType().getType()]) + offset_.getX(),
        (x * C[orientation_.getType().getType()]) +
            (y * D[orientation_.getType().getType()]) + offset_.getY(),
        COMPOSE[(orientation_.getType().getType() * 8) +
                transformation.orientation_.getType().getType()]);
  }
  void applyOn(PNLTransform& transformation) const {
    transformation = getTransform(transformation);
  }
  PNLBox::Unit getX(const PNLBox::Unit& x, const PNLBox::Unit& y) const {
    return (x * A[(int)orientation_.getType().getType()]) +
           (y * B[(int)orientation_.getType().getType()]) + offset_.getX();
  }

  PNLBox::Unit getY(const PNLBox::Unit& x, const PNLBox::Unit& y) const {
    return (x * C[(int)orientation_.getType().getType()]) +
           (y * D[(int)orientation_.getType().getType()]) + offset_.getY();
  }
  PNLBox getBox(const PNLBox::Unit& x1,
                const PNLBox::Unit& y1,
                const PNLBox::Unit& x2,
                const PNLBox::Unit& y2) const {
    return PNLBox(getX(x1, y1), getY(x1, y1), getX(x2, y2), getY(x2, y2));
  }
  PNLBox getBox(const PNLBox& box) const {
    if (box.isEmpty())
      return box;
    return getBox(box.getLeft(), box.getBottom(), box.getRight(), box.getTop());
  }

  void applyOn(PNLBox& box) const { box = getBox(box); }

 private:
  PNLPoint offset_{0, 0};
  PNLOrientation orientation_;
};

}  // namespace naja::NL
