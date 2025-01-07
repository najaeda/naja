// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBusTermBit.h"

#include "PyInterface.h"
#include "PySNLDesign.h"
#include "PySNLBusTerm.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLBusTermBit, function)

DirectGetIntMethod(PySNLBusTermBit_getBit, getBit, PySNLBusTermBit, SNLBusTermBit)
GetObjectMethod(BusTermBit, BusTerm, getBus)

DirectGetIntMethod(PySNLBusTermBit_getID, getID, PySNLBusTermBit, SNLBusTermBit)
DirectGetIntMethod(PySNLBusTermBit_getFlatID, getFlatID, PySNLBusTermBit, SNLBusTermBit)

PyMethodDef PySNLBusTermBit_Methods[] = {
  { "getBit", (PyCFunction)PySNLBusTermBit_getBit, METH_NOARGS,
    "get SNLBusTermBit Bit value"},
  { "getBus", (PyCFunction)PySNLBusTermBit_getBus, METH_NOARGS,
    "get SNLBusTermBit Bus term"},
  { "getID", (PyCFunction)PySNLBusTermBit_getID, METH_NOARGS,
    "Get the SNLID::DesignObjectID of the instance."},
  { "getFlatID", (PyCFunction)PySNLBusTermBit_getFlatID, METH_NOARGS,
    "Get the flat ID of the instance."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoLinkCreateMethod(SNLBusTermBit)
DBoDeallocMethod(SNLBusTermBit)
PyTypeObjectDefinitions(SNLBusTermBit)

PyTypeSNLFinalObjectWithSNLIDLinkPyType(SNLBusTermBit)

}