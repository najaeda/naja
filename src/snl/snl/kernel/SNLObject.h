// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_OBJECT_H_
#define __SNL_OBJECT_H_

#include <ostream>
#include <iostream>

#include "NajaObject.h"

namespace naja { namespace SNL {

/**
 * \brief SNLObject is the root object of SNL database objects.
 */
class SNLObject: public NajaObject {
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
    static void preCreate();
    void postCreate();
    void preDestroy() override;
};

}} // namespace SNL // namespace naja

#endif // __SNL_OBJECT_H_
