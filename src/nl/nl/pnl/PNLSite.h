// SPDX-FileCopyrightText: 2025 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_SITE_H_
#define __PNL_SITE_H_

#include "NLID.h"
#include "NLName.h"
#include "PNLTechnology.h"
#include "PNLBox.h"

namespace naja {
namespace NL {

class PNLSite {
 public:

 // Site simmetry enum x, y, 90
 enum Symmetry {
   NONE = 0,
   X,
   Y,
   X_Y,
   R90
 };
 
 enum ClassType {
   Unknown = 0,
   Core,
   Pad
 };

  PNLSite() = default;
  ~PNLSite();

  static PNLSite* create(const NLName& name,
                         ClassType siteClass,
                         const PNLBox::Unit& width,
                         const PNLBox::Unit& height);

  void setID(NLID::DesignObjectID id) { id_ = id; }
  NLID::DesignObjectID getID() const { return id_; }
  const NLName& getName() const { return name_; }
  void setClass(const ClassType& classType) { class_ = classType; }
  const ClassType& getClass() const { return class_; }

  PNLBox::Unit getWidth() const { return width_; }
  PNLBox::Unit getHeight() const { return height_; }

  void setSymmetry(Symmetry symmetry) { symmetry_ = symmetry; }
  Symmetry getSymmetry() const { return symmetry_; }

 private:
  NLName name_;
  ClassType class_;
  NLID::DesignObjectID id_ = (NLID::DesignObjectID)-1;
  PNLBox::Unit width_ = 0;
  PNLBox::Unit height_ = 0;
  Symmetry symmetry_ = Symmetry::NONE;
};

}  // namespace NL
}  // namespace naja

#endif  // __PNL_SITE_H_
