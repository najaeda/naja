// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBusNetBit.h"

#include "PyInterface.h"
#include "PySNLDesign.h"

#include "SNLBusNetBit.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)   GENERIC_METHOD_HEAD(BusNetBit, net, function)

PyMethodDef PySNLBusNetBit_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLBusNetBit)

DBoLinkCreateMethod(SNLBusNetBit)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLBusNetBit)
PyTypeObjectDefinitions(SNLBusNetBit)

}
