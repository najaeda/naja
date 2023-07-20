// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLTerm.h"

#include "PyInterface.h"
#include "PySNLTermDirection.h"
#include "PySNLDesign.h"
#include "PySNLBitTerms.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(SNLTerm, function)

GetNameMethod(SNLTerm)
GetContainerMethodWithMethodName(Term, BitTerm, getBits)

PyMethodDef PySNLTerm_Methods[] = {
  { "getName", (PyCFunction)PySNLTerm_getName, METH_NOARGS,
    "get SNLNet name"},
  { "getBits", (PyCFunction)PySNLTerm_getBits, METH_NOARGS,
    "get a container of SNLBitTerms."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLTerm_destroy, PySNLTerm)
DBoDeallocMethod(SNLTerm)

DBoLinkCreateMethod(SNLTerm)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLTerm)
PyTypeInheritedObjectDefinitions(SNLTerm, SNLNetComponent)

void PySNLTerm_postModuleInit() {
  PySNLTermDirection_postModuleInit();
  PyDict_SetItemString(PyTypeSNLTerm.tp_dict, "Direction", (PyObject*)&PyTypeSNLTermDirection);
}

}
