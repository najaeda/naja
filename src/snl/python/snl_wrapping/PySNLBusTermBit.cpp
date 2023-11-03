// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBusTermBit.h"

#include "PyInterface.h"
#include "PySNLDesign.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(Instance, instance, function)


DBoDeallocMethod(SNLBusTermBit)

DBoLinkCreateMethod(SNLBusTermBit)
PyTypeObjectDefinitions(SNLBusTermBit)

PyMethodDef PySNLBusTermBit_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeSNLObjectWithSNLIDLinkPyType(SNLBusTermBit)

}
