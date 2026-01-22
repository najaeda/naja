// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <string>

namespace naja::NL {

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
        // Default constructor
        Type() = default;
        Type& operator=(const Type&) = default;
        std::string getString() const;
        // Comperators
        bool operator==(const Type& other) const {
          return typeEnum_ == other.typeEnum_;
        }
        bool operator!=(const Type& other) const {
          return !(typeEnum_ == other.typeEnum_);
        }
        bool operator<(const Type& other) const {
          return typeEnum_ < other.typeEnum_;
        }
        bool operator>(const Type& other) const {
          return typeEnum_ > other.typeEnum_;
        }
        bool operator<=(const Type& other) const {
          return !(typeEnum_ > other.typeEnum_);
        }
        bool operator>=(const Type& other) const {
          return !(typeEnum_ < other.typeEnum_);
        }
        const TypeEnum& getType() const { return typeEnum_; }
      private:
        TypeEnum typeEnum_ = R0; // Default to R0
    };
    PNLOrientation(const Type::TypeEnum& orientationType) : orientationType_(orientationType) {}
    PNLOrientation(const Type& orientationType) : orientationType_(orientationType) {}
    // Default constructor
    PNLOrientation() = default;
    const Type& getType() const { return orientationType_; }
    // Comperators
    bool operator==(const PNLOrientation& other) const {
      return orientationType_ == other.orientationType_;
    }
    bool operator!=(const PNLOrientation& other) const {
      return !(*this == other);
    }
    bool operator<(const PNLOrientation& other) const {
      return orientationType_ < other.orientationType_;
    }
    bool operator>(const PNLOrientation& other) const {
      return orientationType_ > other.orientationType_;
    }
    bool operator<=(const PNLOrientation& other) const {
      return !(*this > other);
    }
    bool operator>=(const PNLOrientation& other) const {
      return !(*this < other);
    }
    private:
    Type orientationType_;
};

} // namespace naja::PNL
