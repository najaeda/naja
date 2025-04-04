// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_ORIENTATION_H_
#define __PNL_ORIENTATION_H_

#include <string>

namespace naja { namespace NL {

class PNLOrientation {
  public:
    class Type {
      public:
        enum TypeEnum {
          R0,     // Represents no change in orientation
          R90,    // Represents a 90 degree rotation
          R180,   // Represents a 180 degree rotation
          R270,   // Represents a 270 degree rotation
          MY,     // Represents mirroring about the Y axis
          MYR90,  // Represents mirroring about the Y axis then a 90 degree rotation
          MX,     // Represents mirroring about the X axis
          MXR90   // Represents mirroring about the X axis then a 90 degree rotation
        };
        Type(const TypeEnum& typeEnum);
        Type(const Type&) = default;
        Type& operator=(const Type&) = default;
        std::string getString() const;
      private:
        TypeEnum typeEnum_;
    };
    PNLOrientation(Type orientationType) : orientationType_(orientationType) {}
    private:
    Type orientationType_;
};

}} // namespace PNL // namespace naja

#endif // __PNL_POINT_H_