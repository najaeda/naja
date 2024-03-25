// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLInstTerm.h"

#include "SNLInstTerm.h"

#include "PyInterface.h"
#include "PySNLBitTerm.h"
#include "PySNLInstance.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(SNLInstTerm, function)

DBoDeallocMethod(SNLInstTerm)

DBoLinkCreateMethod(SNLInstTerm)
PyTypeInheritedObjectDefinitions(SNLInstTerm, SNLNetComponent)

GetObjectMethod(InstTerm, Instance, getInstance)
GetObjectMethod(InstTerm, BitTerm, getBitTerm)

PyMethodDef PySNLInstTerm_Methods[] = {
  { "getBitTerm", (PyCFunction)PySNLInstTerm_getBitTerm, METH_NOARGS,
    "get the SNLBitTerm represented by this SNLInstTerm."},
  { "getInstance", (PyCFunction)PySNLInstTerm_getInstance, METH_NOARGS,
    "get the SNLInstance containing this SNLInstTerm."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeSNLObjectWithSNLIDLinkPyType(SNLInstTerm)

}