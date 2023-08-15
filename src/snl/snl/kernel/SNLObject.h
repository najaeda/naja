// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_OBJECT_H_
#define __SNL_OBJECT_H_

#include <ostream>
#include <iostream>

#include "NajaObject.h"

namespace naja { namespace SNL {

class SNLObject: public NajaObject {
  public:
    using super = NajaObject;
    virtual void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const = 0;
    
  protected:
    static void preCreate();
    void postCreate();
    void preDestroy() override;
};

}} // namespace SNL // namespace naja

#endif // __SNL_OBJECT_H_
