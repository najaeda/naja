// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLTerm.h"

#include "PyInterface.h"
#include "PySNLBusTerm.h"
#include "PySNLScalarTerm.h"
#include "PySNLTermDirection.h"
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
DirectGetIntMethod(PySNLTerm_getWidth, getWidth, PySNLTerm, SNLTerm)

PyMethodDef PySNLTerm_Methods[] = {
  { "getName", (PyCFunction)PySNLTerm_getName, METH_NOARGS,
    "get SNLNet name"},
  { "getWidth", (PyCFunction)PySNLTerm_getWidth, METH_NOARGS,
    "get SNLNet width"},
  { "getBits", (PyCFunction)PySNLTerm_getBits, METH_NOARGS,
    "get a container of SNLBitTerms."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyObject* PySNLTerm_Link(SNLTerm* object) {
  if (not object) {
    Py_RETURN_NONE;   
  }
  if (auto busTerm = dynamic_cast<SNLBusTerm*>(object)) {
    return PySNLBusTerm_Link(busTerm);
  } else {
    auto scalarTerm = static_cast<SNLScalarTerm*>(object);
    return PySNLScalarTerm_Link(scalarTerm);
  }
}

PyTypeNLAbstractObjectWithNLIDLinkPyType(SNLTerm)
PyTypeInheritedObjectDefinitions(SNLTerm, SNLNetComponent)

void PySNLTerm_postModuleInit() {
  PySNLTermDirection_postModuleInit();
  PyDict_SetItemString(PyTypeSNLTerm.tp_dict, "Direction", (PyObject*)&PyTypeSNLTermDirection);
}

}
