// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLTerm.h"

#include "SNLBundleTerm.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"

#include "PyInterface.h"
#include "PySNLBundleTerm.h"
#include "PySNLBusTerm.h"
#include "PySNLScalarTerm.h"
#include "PySNLTermDirection.h"
#include "PySNLBitTerms.h"

namespace PYNAJA {

using namespace naja::NL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(SNLTerm, function)

GetNameMethod(SNLTerm)
GetContainerMethodWithMethodName(Term, BitTerm, getBits)
DirectGetNumericMethod(PySNLTerm_getWidth, getWidth, PySNLTerm, SNLTerm)

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
  } else if (auto scalarTerm = dynamic_cast<SNLScalarTerm*>(object)) {
    return PySNLScalarTerm_Link(scalarTerm);
  } else if (auto bundleTerm = dynamic_cast<SNLBundleTerm*>(object)) {
    return PySNLBundleTerm_Link(bundleTerm);
  }
  Py_RETURN_NONE;
}

PyTypeNLAbstractObjectWithNLIDLinkPyType(SNLTerm)
PyTypeInheritedObjectDefinitions(SNLTerm, SNLNetComponent)

void PySNLTerm_postModuleInit() {
  PySNLTermDirection_postModuleInit();
  PyDict_SetItemString(PyTypeSNLTerm.tp_dict, "Direction", (PyObject*)&PyTypeSNLTermDirection);
}

}
