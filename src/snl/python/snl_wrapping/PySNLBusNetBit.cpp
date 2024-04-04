// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBusNetBit.h"

#include "PyInterface.h"
#include "PySNLDesign.h"
#include "PySNLBusNet.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject) &(_pyObject->parent_)
#define  METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLBusNetBit, function)

DirectGetIntMethod(PySNLBusNetBit_getBit, getBit, PySNLBusNetBit, SNLBusNetBit)
GetObjectMethod(BusNetBit, BusNet, getBus)

PyMethodDef PySNLBusNetBit_Methods[] = {
  { "getBit", (PyCFunction)PySNLBusNetBit_getBit, METH_NOARGS,
    "get SNLBusNetBit Bit value"},
  { "getBus", (PyCFunction)PySNLBusNetBit_getBus, METH_NOARGS,
    "get SNLBusNetBit Bus net"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLBusNetBit)

DBoLinkCreateMethod(SNLBusNetBit)
PyTypeSNLFinalObjectWithSNLIDLinkPyType(SNLBusNetBit)
PyTypeObjectDefinitions(SNLBusNetBit)

}