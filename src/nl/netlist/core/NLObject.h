// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <ostream>
#include <iostream>

#include "NajaObject.h"

namespace naja::NL {

/**
 * \brief NLObject is the root object of Naja database objects.
 */
class NLObject: public NajaObject {
  public:
    using super = NajaObject;
    /**
     * \brief Debug method that dumps string describing extensively the object and
     * its sub elements.
     * \param indent number of characters to use for indenting.
     * \param recursive if true, this method will be called recursively on this object elements.
     * Optional parameter with default value: true.
     * \param stream stream to which the debug string will be dumped.
     * Optional parameter with default value: std:cerr.
     */
    virtual void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const = 0;
    
  protected:
    // cppcheck-suppress duplInheritedMember
    static void preCreate();

    void postCreate() override;
    void preDestroy() override;
};

}  // namespace naja::NL