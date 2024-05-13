#ifndef __PNL_ORIENTATION_H_
#define __PNL_ORIENTATION_H_

#include <string>

namespace naja { namespace PNL {

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
};

}} // namespace PNL // namespace naja

#endif // __PNL_POINT_H_