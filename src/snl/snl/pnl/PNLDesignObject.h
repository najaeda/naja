// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_DESIGN_OBJECT_H_
#define __PNL_DESIGN_OBJECT_H_

#include "SNLObject.h"

namespace naja { namespace PNL {

class PNLDesign;

class PNLDesignObject: public SNL::SNLObject {
  public:
    virtual PNLDesign* getDesign() const = 0;
  protected:
    PNLDesignObject() = default;
    
    static void preCreate();
    void postCreate();
};

}} // namespace PNL // namespace naja

#endif // __PNL_DESIGN_OBJECT_H_