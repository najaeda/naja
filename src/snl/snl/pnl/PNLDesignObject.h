// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_DESIGN_OBJECT_H_
#define __PNL_DESIGN_OBJECT_H_

#include "SNLObject.h"

namespace naja { namespace SNL {

class PNLDesign;

class PNLDesignObject: public SNLObject {
  public:
    virtual PNLDesign* getDesign() const = 0;
  protected:
    PNLDesignObject() = default;
    
    static void preCreate();
    void postCreate();
};

}} // namespace SNL // namespace naja

#endif // __PNL_DESIGN_OBJECT_H_